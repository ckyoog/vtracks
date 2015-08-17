#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/times.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <linux/errqueue.h>

//#define USE_SELECT_INSTEAD_OF_POLL 1
#define ENABLE_TOS_AND_FLOWLABEL 1

#ifdef ENABLE_TOS_AND_FLOWLABEL

/* ---------------------------
 * It is just a stripped copy of the kernel header "linux/in6.h"
 *
 * "Flow label" things are still not defined in "netinet/in*.h" headers,
 * but we cannot use "linux/in6.h" immediately because it currently
 * conflicts with "netinet/in.h" .
 */

struct in6_flowlabel_req
{
	struct in6_addr flr_dst;
	__u32   flr_label;
	__u8    flr_action;
	__u8    flr_share;
	__u16   flr_flags;
	__u16   flr_expires;
	__u16   flr_linger;
	__u32   __flr_pad;
	/* Options in format of IPV6_PKTOPTIONS */
};

#define IPV6_FL_A_GET   0
#define IPV6_FL_A_PUT   1
#define IPV6_FL_A_RENEW 2

#define IPV6_FL_F_CREATE        1
#define IPV6_FL_F_EXCL          2

#define IPV6_FL_S_NONE          0
#define IPV6_FL_S_EXCL          1
#define IPV6_FL_S_PROCESS       2
#define IPV6_FL_S_USER          3
#define IPV6_FL_S_ANY           255

#define IPV6_FLOWINFO_FLOWLABEL         0x000fffff
#define IPV6_FLOWINFO_PRIORITY          0x0ff00000

#define IPV6_FLOWLABEL_MGR      32
#define IPV6_FLOWINFO_SEND      33
/* --------------------------- */

static unsigned int const tos = 0;
static unsigned int const flow_label = 0;
#endif

static int const dontfrag = 0;

typedef struct rawsock {
	int raw_sk;
	int tmp_sk;
} rawsock_t;

typedef struct rawsock_buf {
	uint8_t buf[1024];
	struct tcphdr *th;
	size_t csum_len;
} rawsock_buf_t;

typedef struct check_result {
	int err;
	struct sockaddr_storage addr;
	char err_str[32];
} check_result_t;

#define perror_print(str) perror(str)
#define error_print(str) fprintf(stderr, "%s: error\n", str)

#define perror_return(str) do { \
	perror_print(str); \
	return -1; \
} while (0)

#define error_return(str) do { \
	error_print(str); \
	return -1; \
} while (0)

#define perror_close_return(fd, str) do { \
	close(fd); \
	perror_return(str); \
} while (0)

#define error_close_return(fd, str) do { \
	close(fd); \
	error_return(str); \
} while (0)

#define _perror_goto(str, label) do { \
	perror_print(str); \
	goto label; \
} while (0)

#define _error_goto(str, label) do { \
	error_print(str); \
	goto label; \
} while (0)

#define perror_goto(str) _perror_goto(str, err)
#define error_goto(str) _error_goto(str, err)

/* 'addr' is a pointer of struct sockaddr_storage
 * The both statements below are exactly the same
 */
//#define sas_port(addr) ((struct sockaddr_in *)(addr))->sin_port
#define sas_port(addr) ((struct sockaddr_in6 *)(addr))->sin6_port

inline static
const char *inet_sastos(const struct sockaddr_storage *addr, char *buf, size_t len)
{
	return inet_ntop(addr->ss_family,
			addr->ss_family == AF_INET ?
				(void *)&((struct sockaddr_in *)addr)->sin_addr :
				(void *)&((struct sockaddr_in6 *)addr)->sin6_addr,
			buf, len);
}

inline static
void print_addr(const char *str, const struct sockaddr_storage *addr)
{
	char buf[INET6_ADDRSTRLEN];
	printf("%s is %s %d\n", str,
			inet_sastos(addr, buf, sizeof(buf)),
			ntohs(sas_port(addr)));
}

static void __init_random_seq(void) __attribute__ ((constructor));
static void __init_random_seq(void)
{
	srand(times(NULL) + getpid());
}

inline static
unsigned int random_seq(void)
{
	/*  To not worry about RANDOM_MAX and precision...  */
	return  (rand() << 16) ^ (rand() << 8) ^ rand() ^ (rand() >> 8);
}

inline static
uint16_t in_csum(const void *ptr, size_t len)
{
	const uint16_t *p = (const uint16_t *)ptr;
	size_t nw = len / 2;
	unsigned int sum = 0;
	uint16_t res;

	while (nw--)  sum += *p++;

	if (len & 0x1)
		sum += htons(*((unsigned char *) p) << 8);

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	res = ~sum;
	if (!res)  res = ~0;

	return res;
}

static
void parse_icmp_res(check_result_t *cres, int type, int code, int info)
{
	char *str = NULL;
	char buf[sizeof(cres->err_str)];
	int af = cres->addr.ss_family;

	if (af == AF_INET) {

	    if (type == ICMP_TIME_EXCEEDED) {
		if (code == ICMP_EXC_TTL) {
			str = "!T";
		}
	    }
	    else if (type == ICMP_DEST_UNREACH) {

		switch (code) {
		    case ICMP_UNREACH_NET:
		    case ICMP_UNREACH_NET_UNKNOWN:
		    case ICMP_UNREACH_ISOLATED:
		    case ICMP_UNREACH_TOSNET:
			    str = "!N";
			    break;

		    case ICMP_UNREACH_HOST:
		    case ICMP_UNREACH_HOST_UNKNOWN:
		    case ICMP_UNREACH_TOSHOST:
			    str = "!H";
			    break;

		    case ICMP_UNREACH_NET_PROHIB:
		    case ICMP_UNREACH_HOST_PROHIB:
		    case ICMP_UNREACH_FILTER_PROHIB:
			    str = "!X";
			    break;

		    case ICMP_UNREACH_PORT:
			    /*  dest host is reached   */
			    str = "R";
			    break;

		    case ICMP_UNREACH_PROTOCOL:
			    str = "!P";
			    break;

		    case ICMP_UNREACH_NEEDFRAG:
			    snprintf(buf, sizeof(buf), "!F-%d", info);
			    str = buf;
			    break;

		    case ICMP_UNREACH_SRCFAIL:
			    str = "!S";
			    break;

		    case ICMP_UNREACH_HOST_PRECEDENCE:
			    str = "!V";
			    break;

		    case ICMP_UNREACH_PRECEDENCE_CUTOFF:
			    str = "!C";
			    break;

		    default:
			    snprintf(buf, sizeof(buf), "!<%u>", code);
			    str = buf;
			    break;
		}
	    }

	}
	else if (af == AF_INET6) {

	    if (type == ICMP6_TIME_EXCEEDED) {
		if (code == ICMP6_TIME_EXCEED_TRANSIT) {
			str = "!T";
		}
	    }
	    else if (type == ICMP6_DST_UNREACH) {

		switch (code) {

		    case ICMP6_DST_UNREACH_NOROUTE:
			    str = "!N";
			    break;

		    case ICMP6_DST_UNREACH_BEYONDSCOPE:
		    case ICMP6_DST_UNREACH_ADDR:
			    str = "!H";
			    break;

		    case ICMP6_DST_UNREACH_ADMIN:
			    str = "!X";
			    break;

		    case ICMP6_DST_UNREACH_NOPORT:
			    /*  dest host is reached   */
			    str = "";
			    break;

		    default:
			    snprintf(buf, sizeof(buf), "!<%u>", code);
			    str = buf;
			    break;
		}
	    }
	    else if (type == ICMP6_PACKET_TOO_BIG) {
		snprintf(buf, sizeof(buf), "!F-%d", info);
		str = buf;
	    }
	}


	if (!str) {
	    snprintf(buf, sizeof(buf), "!<%u-%u>", type, code);
	    str = buf;
	}

	if (*str)
	    strcpy(cres->err_str, str);

	return;
}

int inet_stosockaddr(char *ip, char *port, struct sockaddr_storage *addr)
{
	void *addr_ip;
	char *cp = ip;

	addr->ss_family = (strchr(ip, ':')) ? AF_INET6 : AF_INET;

	/* remove range and mask stuff */
	if (strstr(ip, "-")) {
		while (*cp != '-' && *cp != '\0')
			cp++;
		if (*cp == '-')
			*cp = 0;
	} else if (strstr(ip, "/")) {
		while (*cp != '/' && *cp != '\0')
			cp++;
		if (*cp == '/')
			*cp = 0;
	}

	if (addr->ss_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;
		if (port)
			addr6->sin6_port = htons(atoi(port));
		addr_ip = &addr6->sin6_addr;
	} else {
		struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
		if (port)
			addr4->sin_port = htons(atoi(port));
		addr_ip = &addr4->sin_addr;
	}

	if (!inet_pton(addr->ss_family, ip, addr_ip))
		return -1;

	return 0;
}

inline static
int bind_socket(int af, int sk, struct sockaddr_storage *src)
{
	if (!src)
		return -1;

	struct sockaddr_storage tmpaddr;
	if (!src->ss_family) {
		memset(&tmpaddr, 0, sizeof(tmpaddr));
		tmpaddr.ss_family = af;
		src = &tmpaddr;
	}
	if (bind(sk, (struct sockaddr *)src, sizeof(*src)) == -1)
		perror_return("bind");
	return 0;
}

inline static
int use_recverr(int af, int sk)
{
	int val = 1;

	if (af == AF_INET) {
		if (setsockopt(sk, SOL_IP, IP_RECVERR, &val, sizeof (val)) < 0)
			perror_return("setsockopt IP_RECVERR");
	}
	else if (af == AF_INET6) {
		if (setsockopt(sk, SOL_IPV6, IPV6_RECVERR, &val, sizeof (val)) < 0)
			perror_return("setsockopt IPV6_RECVERR");
	}
	return 0;
}

inline static
int use_timestamp(int sk)
{
	int n = 1;

	if (setsockopt(sk, SOL_SOCKET, SO_TIMESTAMP, &n, sizeof(n)) < 0)
		perror_return("setsockopt SO_TIMESTAMP");
	/*  foo on errors...  */

	return 0;
}

inline static
int use_recv_ttl(int af, int sk)
{
	int n = 1;

	if (af == AF_INET) {
		if (setsockopt(sk, SOL_IP, IP_RECVTTL, &n, sizeof(n)) < 0)
			perror_return("setsockopt IP_RECVTTL");
	} else if (af == AF_INET6) {
		if (setsockopt(sk, SOL_IPV6, IPV6_RECVHOPLIMIT, &n, sizeof(n)) < 0)
			perror_return("setsockopt IPV6_RECVHOPLIMIT");
	}
	/*  foo on errors   */

	return 0;
}

inline static
int set_ttl(int af, int sk, int ttl)
{
	if (af == AF_INET) {
		if (setsockopt(sk, SOL_IP, IP_TTL, &ttl, sizeof (ttl)) < 0)
			perror_return("setsockopt IP_TTL");
	} else if (af == AF_INET6) {
		if (setsockopt(sk, SOL_IPV6, IPV6_UNICAST_HOPS,
					&ttl, sizeof (ttl)) < 0)
			perror_return("setsockopt IPV6_UNICAST_HOPS");
	}
	return 0;
}

static
int tune_socket(int af, int sk, struct sockaddr_storage *dst, struct sockaddr_storage *src)
{
	int i;

	if (bind_socket(af, sk, src) != 0) error_return("bind_socket");

	if (af == AF_INET) {
		i = dontfrag ? IP_PMTUDISC_PROBE : IP_PMTUDISC_DONT;
		if (setsockopt(sk, SOL_IP, IP_MTU_DISCOVER, &i, sizeof(i)) < 0 &&
				(!dontfrag || (i = IP_PMTUDISC_DO,
					       setsockopt(sk, SOL_IP, IP_MTU_DISCOVER, &i, sizeof(i)) < 0)))
			perror_return("setsockopt IP_MTU_DISCOVER");

#ifdef ENABLE_TOS_AND_FLOWLABEL
		if (tos) {
			i = tos;
			if (setsockopt(sk, SOL_IP, IP_TOS, &i, sizeof(i)) < 0)
				perror_return("setsockopt IP_TOS");
		}
#endif

	} else if (af == AF_INET6) {

		i = dontfrag ? IPV6_PMTUDISC_PROBE : IPV6_PMTUDISC_DONT;
		if (setsockopt(sk, SOL_IPV6, IPV6_MTU_DISCOVER,&i,sizeof(i)) < 0 &&
				(!dontfrag || (i = IPV6_PMTUDISC_DO,
					       setsockopt(sk, SOL_IPV6, IPV6_MTU_DISCOVER, &i, sizeof(i)) < 0)))
			perror_return("setsockopt IPV6_MTU_DISCOVER");

#ifdef ENABLE_TOS_AND_FLOWLABEL
		if (flow_label) {
			struct in6_flowlabel_req flr;

			memset(&flr, 0, sizeof(flr));
			flr.flr_label = htonl(flow_label & 0x000fffff);
			flr.flr_action = IPV6_FL_A_GET;
			flr.flr_flags = IPV6_FL_F_CREATE;
			flr.flr_share = IPV6_FL_S_ANY;
			memcpy(&flr.flr_dst, &((struct sockaddr_in6 *)dst)->sin6_addr,
					sizeof(flr.flr_dst));

			if (setsockopt(sk, IPPROTO_IPV6, IPV6_FLOWLABEL_MGR,
						&flr, sizeof (flr)) < 0)
				perror_return("setsockopt IPV6_FLOWLABEL_MGR");
		}

		if (tos) {
			i = tos;
			if (setsockopt(sk, IPPROTO_IPV6, IPV6_TCLASS,
						&i, sizeof (i)) < 0)
				perror_return("setsockopt IPV6_TCLASS");
		}

		if (tos || flow_label) {
			i = 1;
			if (setsockopt(sk, IPPROTO_IPV6, IPV6_FLOWINFO_SEND,
						&i, sizeof (i)) < 0)
				perror_return("setsockopt IPV6_FLOWINFO_SEND");
		}
#endif
	}

	if (use_timestamp(sk) != 0) error_return("use_timestamp");

	if (use_recv_ttl(af, sk) != 0) error_return("use_recv_ttl");

	int flags;
	if ((flags = fcntl(sk, F_GETFL)) == -1) perror_return("fcntl get");
	if (fcntl(sk, F_SETFL, flags | O_NONBLOCK) == -1) perror_return("fcntl set nonblock");

	return 0;
}

int equal_addr(const struct sockaddr_storage *a, const struct sockaddr_storage *b)
{

	if (!a->ss_family)
		return 0;

	if (a->ss_family != b->ss_family)
		return 0;

	if (a->ss_family == AF_INET6)
		return  !memcmp(&((struct sockaddr_in6 *)a)->sin6_addr, &((struct sockaddr_in6 *)b)->sin6_addr,
				sizeof(((struct sockaddr_in6 *)a)->sin6_addr));
	else
		return  !memcmp(&((struct sockaddr_in *)a)->sin_addr, &((struct sockaddr_in *)b)->sin_addr,
				sizeof (((struct sockaddr_in *)a)->sin_addr));
	return 0;       /*  not reached   */
}

int do_send(int sk, const void *data, size_t len, const struct sockaddr_storage *addr)
{
	int res;

	if (!addr || 1)
		res = send(sk, data, len, 0);
	else
		res = sendto(sk, data, len, 0, (struct sockaddr *)addr, sizeof(*addr));

	if (res < 0) {
		if (errno == ENOBUFS || errno == EAGAIN)
			return res;
		if (errno == EMSGSIZE)
			return 0;   /*  icmp will say more...  */
		perror_return("send");     /*  not recoverable   */
	}

	return res;
}


#define TH_FLAGS(TH)    (((uint8_t *) (TH))[13])
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PSH  0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80

#define FL_FLAGS        0x0100
#define FL_ECN          0x0200
#define FL_SACK         0x0400
#define FL_TSTAMP       0x0800
#define FL_WSCALE       0x1000

int tcp_half_init(rawsock_t *rawsock, struct sockaddr_storage *dst, struct sockaddr_storage *bindto, rawsock_buf_t *rawbuf)
{
	int raw_sk, mtu, af;
	socklen_t len;
	struct sockaddr_storage src;
	uint8_t *ptr, *buf = rawbuf->buf;
	unsigned int mss = 0;

	memset(rawbuf, 0, sizeof(*rawbuf));
	rawsock->raw_sk = rawsock->tmp_sk = -1;

	af = dst->ss_family;
	raw_sk = socket(af, SOCK_RAW, IPPROTO_TCP);
	if (raw_sk == -1)
		perror_return("socket");

	if (tune_socket(af, raw_sk, dst, bindto) != 0)
		error_goto("tune_socket");

	struct sockaddr_storage dst_port0;
	memcpy(&dst_port0, dst, sizeof(dst_port0));
	sas_port(&dst_port0) = 0;
	if (connect(raw_sk, (struct sockaddr *)&dst_port0, sizeof(dst_port0)) == -1)
		perror_goto("connect");
	//print_addr("connect address", &dst_port0);

	len = sizeof(src);
	if (getsockname(raw_sk, (struct sockaddr *)&src, &len) < 0)
		perror_goto("getsockname");
	//print_addr("raw socket bind-sockname", &src);

	len = sizeof(mtu);
	if (getsockopt(raw_sk, af == AF_INET ? SOL_IP : SOL_IPV6,
				af == AF_INET ? IP_MTU : IPV6_MTU,
				&mtu, &len) < 0 || mtu < 576)
		mtu = 576;

	/*  mss = mtu - headers   */
	mtu -= af == AF_INET ? sizeof(struct iphdr) : sizeof(struct ip6_hdr);
	mtu -= sizeof(struct tcphdr);

	if (use_recverr(af, raw_sk) != 0) error_goto("use_recverr");

	int flags = TH_SYN;
	flags |= FL_SACK;
	flags |= FL_TSTAMP;
	flags |= FL_WSCALE;

	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	ptr = buf;
	if (af == AF_INET) {
		len = sizeof(sin->sin_addr);
		sin = (struct sockaddr_in *)&src;
		memcpy(ptr, &sin->sin_addr, len);
		ptr += len;
		sin = (struct sockaddr_in *)dst;
		memcpy(ptr, &sin->sin_addr, len);
		ptr += len;
	} else {
		len = sizeof(sin6->sin6_addr);
		sin6 = (struct sockaddr_in6 *)&src;
		memcpy(ptr, &sin6->sin6_addr, len);
		ptr += len;
		sin6 = (struct sockaddr_in6 *)dst;
		memcpy(ptr, &sin6->sin6_addr, len);
		ptr += len;
	}

	uint16_t *lenp = (uint16_t *)ptr;
	ptr += sizeof(uint16_t);
	*((uint16_t *)ptr) = htons((uint16_t)IPPROTO_TCP);
	ptr += sizeof(uint16_t);

	/*  Construct TCP header   */

	struct tcphdr *th = (struct tcphdr *)ptr;
	rawbuf->th = th;

	th->source = 0;     /*  temporary   */
	th->dest = sas_port(dst);
	th->seq = 0;        /*  temporary   */
	th->ack_seq = 0;
	th->doff = 0;       /*  later...  */
	TH_FLAGS(th) = flags & 0xff;
	th->window = htons(4 * mtu);
	th->check = 0;
	th->urg_ptr = 0;


	/*  Build TCP options   */

	ptr = (uint8_t *)(th + 1);

	if (flags & TH_SYN) {
		*ptr++ = TCPOPT_MAXSEG;     /*  2   */
		*ptr++ = TCPOLEN_MAXSEG;    /*  4   */
		*((uint16_t *)ptr) = htons(mss ? mss : mtu);
		ptr += sizeof(uint16_t);
	}

	if (flags & FL_TSTAMP) {
		if (flags & FL_SACK) {
			*ptr++ = TCPOPT_SACK_PERMITTED; /*  4   */
			*ptr++ = TCPOLEN_SACK_PERMITTED;/*  2   */
		} else {
			*ptr++ = TCPOPT_NOP;    /*  1   */
			*ptr++ = TCPOPT_NOP;    /*  1   */
		}
		*ptr++ = TCPOPT_TIMESTAMP;  /*  8   */
		*ptr++ = TCPOLEN_TIMESTAMP; /*  10  */

		*((uint32_t *)ptr) = random_seq();       /*  really!  */
		ptr += sizeof(uint32_t);
		*((uint32_t *)ptr) = (flags & TH_ACK) ? random_seq() : 0;
		ptr += sizeof(uint32_t);
	}
	else if (flags & FL_SACK) {
		*ptr++ = TCPOPT_NOP;        /*  1   */
		*ptr++ = TCPOPT_NOP;        /*  1   */
		*ptr++ = TCPOPT_SACK_PERMITTED;     /*  4   */
		*ptr++ = TCPOLEN_SACK_PERMITTED;    /*  2   */
	}

	if (flags & FL_WSCALE) {
		*ptr++ = TCPOPT_NOP;        /*  1   */
		*ptr++ = TCPOPT_WINDOW;     /*  3   */
		*ptr++ = TCPOLEN_WINDOW;    /*  3   */
		*ptr++ = 2; /*  assume some corect value...  */
	}

	size_t csum_len = ptr - buf;
	rawbuf->csum_len = csum_len;

	if (csum_len > sizeof(rawbuf->buf))
		error_goto("impossible 1");   /*  paranoia   */

	len = ptr - (uint8_t *)th;
	if (len & 0x03) error_goto("impossible 2");  /*  as >>2 ...  */

	*lenp = htons(len);
	th->doff = len >> 2;

	rawsock->raw_sk = raw_sk;
	return raw_sk;
err:
	close(raw_sk);
	return -1;
}

int tcp_half_send(rawsock_t *rawsock, struct sockaddr_storage *dst, struct sockaddr_storage *bindto, rawsock_buf_t *rawbuf)
{
	int sk, raw_sk = rawsock->raw_sk;
	int af = dst->ss_family;
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);

	struct tcphdr *th = rawbuf->th;
	uint8_t *buf = rawbuf->buf;
	size_t csum_len = rawbuf->csum_len;

	/*  To make sure we have chosen a free unused "source port",
	   just create, (auto)bind and hold a socket while the port is needed.
	 */

	sk = socket(af, SOCK_STREAM, 0);
	if (sk < 0)  perror_return("socket");
	rawsock->tmp_sk = sk;

	if (bind_socket(af, sk, bindto) != 0) error_return("bind_socket");

	if (getsockname(sk, (struct sockaddr *)&addr, &len) < 0)
		perror_return("getsockname");
	//print_addr("temp socket bind-sockname", &addr);

	/*  When we reach the target host, it can send us either RST or SYN+ACK.
	  For RST all is OK (we and kernel just answer nothing), but
	  for SYN+ACK we should reply with our RST.
	    It is well-known "half-open technique", used by port scanners etc.
	  This way we do not touch remote applications at all, unlike
	  the ordinary connect(2) call.
	    As the port-holding socket neither connect() nor listen(),
	  it means "no such port yet" for remote ends, and kernel always
	  send RST in such a situation automatically (we have to do nothing).
	 */


	th->source = sas_port(&addr);

	th->seq = random_seq();

	th->check = 0;
	th->check = in_csum(buf, csum_len);

	//if (set_ttl(af, raw_sk, 1) != 0) error_return("set_ttl");

	int ret;
	if ((ret = do_send(raw_sk, th, th->doff << 2, dst)) < 0) {
		close(sk);
		rawsock->tmp_sk = -1;
		return ret;
	}

	return 0;
}

void tcp_half_check_reply(int raw_sk, int err, struct sockaddr_storage *dst, check_result_t *cres);
int tcp_half_recv(rawsock_t *rawsock, struct sockaddr_storage *dst, check_result_t *cres)
{
	int ret, recverr;
	int raw_sk = rawsock->raw_sk;
	int to = 10; /* seconds */
	int af = dst->ss_family;

read_again:
	cres->err = 0;
	cres->err_str[0] = 0;

#ifdef USE_SELECT_INSTEAD_OF_POLL
	const char *syscall = "select";
	struct timeval tv;
	fd_set rfds;
	tv.tv_sec = to;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(raw_sk, &rfds);
	ret = select(raw_sk + 1, &rfds, NULL, NULL, &tv);
#else
	const char *syscall = "poll";
	struct pollfd pfd;
	pfd.fd = raw_sk;
	pfd.events = POLLIN | POLLERR;
	ret = poll(&pfd, 1, to * 1000);
#endif
	if (ret == -1)
		perror_return(syscall);
	else if (ret == 0) {
		cres->err = 1;
		memset(&cres->addr, 0, sizeof(cres->addr));
		cres->addr.ss_family = AF_INET;
		snprintf(cres->err_str, sizeof(cres->err_str), "recv(by %s) timeout", syscall);
		return -2;
	}

#ifdef USE_SELECT_INSTEAD_OF_POLL
	socklen_t errlen = sizeof(recverr);
	if (getsockopt(raw_sk, SOL_SOCKET, SO_ERROR, &recverr, &errlen) == -1)
		perror_return("getsockopt");
	//printf("SO_ERROR/IP_RECVERR = %d\n", recverr);
#else
	/* We got something */
	if (!(pfd.revents & (POLLIN | POLLERR)))
		error_return("poll return careless events\n");
	recverr = pfd.revents & POLLERR;
#endif

	tcp_half_check_reply(raw_sk, !!recverr, dst, cres);
	if (cres->err == 2)
		goto read_again;
	return 0;
}

void tcp_half_check_reply(int raw_sk, int recverr, struct sockaddr_storage *dst, check_result_t *cres)
{
	int sk = raw_sk;
	int af = dst->ss_family;

	struct msghdr msg;
	struct sockaddr_storage from;
	struct iovec iov;
	int n;
	char buf[1280];		/*  min mtu for ipv6 ( >= 576 for ipv4)  */
	char *bufp = buf;
	char control[1024];
	struct cmsghdr *cm;
	struct sock_extended_err *ee = NULL;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &from;
	msg.msg_namelen = sizeof(from);
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(sk, &msg, recverr ? MSG_ERRQUEUE : 0);
	if (n < 0)  return;

	/*  when not MSG_ERRQUEUE, AF_INET returns full ipv4 header
	    on raw sockets...
	*/

	if (!recverr &&
	    af == AF_INET
	    /*  XXX: Assume that the presence of an extra header means
		that it is not a raw socket...
	    */
	) {
	    struct iphdr *ip = (struct iphdr *)bufp;
	    int hlen;

	    if (n < sizeof (struct iphdr))  return;

	    hlen = ip->ihl << 2;
	    if (n < hlen)  return;

	    bufp += hlen;
	    n -= hlen;
	}

	//check_reply(sk, recverr, &from, bufp, n);
	uint16_t dport;
	char buf1[INET6_ADDRSTRLEN], buf2[INET6_ADDRSTRLEN];
	struct tcphdr *th = (struct tcphdr *)bufp;
	if (recverr)
		dport = th->dest;
	else
		dport =th->source;
	if (!equal_addr(&from, dst)) {
		printf("address differ, read again. [%s]:[%s]\n",
				inet_sastos(&from, buf1, sizeof(buf1)),
				inet_sastos(dst, buf2, sizeof(buf2)));
		cres->err = 2;
		return;
	}
	if (dport != sas_port(dst)) {
		printf("port differ, read again. %d:%d\n",
				ntohs(dport),
				ntohs(sas_port(dst)));
		cres->err = 2;
		return;
	}
	int flags = TH_FLAGS(th);
	if (flags & TH_RST) {
		cres->err = 1;
		memcpy(&cres->addr, &from, sizeof(cres->addr));
		sas_port(&cres->addr) = th->source;
		snprintf(cres->err_str, sizeof(cres->err_str), "TCP RST");
		return;
	}

	/*  Parse CMSG stuff   */

	for (cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
		void *ptr = CMSG_DATA(cm);

		if (cm->cmsg_level == SOL_IP) {
			if (cm->cmsg_type == IP_RECVERR) {

				ee = (struct sock_extended_err *)ptr;

				if (ee->ee_origin != SO_EE_ORIGIN_ICMP)
					ee = NULL;

				/*  dgram icmp sockets might return extra things...  */
				else if (ee->ee_type == ICMP_SOURCE_QUENCH ||
						ee->ee_type == ICMP_REDIRECT)
					ee = NULL;
			}
		}
		else if (cm->cmsg_level == SOL_IPV6) {
			if (cm->cmsg_type == IPV6_RECVERR) {

				ee = (struct sock_extended_err *)ptr;

				if (ee->ee_origin != SO_EE_ORIGIN_ICMP6)
					ee = NULL;
			}
		}
	}

	if (ee) {
		cres->err = 1;
		memcpy(&cres->addr, SO_EE_OFFENDER (ee), sizeof(cres->addr));
		parse_icmp_res(cres, ee->ee_type, ee->ee_code, ee->ee_info);
		return;
	}
}

void tcp_half_close(rawsock_t *rawsock)
{
	if (rawsock->raw_sk >= 0)
		close(rawsock->raw_sk);
	if (rawsock->tmp_sk >= 0)
		close(rawsock->tmp_sk);
	rawsock->raw_sk = rawsock->tmp_sk = -1;
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <ipv4/ipv6 address> <port>\n", argv[0]);
		exit(1);
	}

	int ret;
	struct sockaddr_storage dst;
	struct sockaddr_storage bindto = { .ss_family = 0 };
	rawsock_t rawsock;
	rawsock_buf_t rawbuf;
	check_result_t cres;

	if (inet_stosockaddr(argv[1], argv[2], &dst) == -1)
		error_return("inet_stosockaddr");
#ifdef ENABLE_TOS_AND_FLOWLABEL
	if (dst.ss_family == AF_INET6 && (tos || flow_label))
		((struct sockaddr_in6 *)&dst)->sin6_flowinfo =
			htonl (((tos & 0xff) << 20) | (flow_label & 0x000fffff));
#endif
	//print_addr("destination", &dst);

	ret = tcp_half_init(&rawsock, &dst, &bindto, &rawbuf);
	if (ret < 0)
		error_return("tcp_half_init");

	if (tcp_half_send(&rawsock, &dst, &bindto, &rawbuf) != 0) {
		tcp_half_close(&rawsock);
		error_return("tcp_half_send");
	}

	tcp_half_recv(&rawsock, &dst, &cres);
	tcp_half_close(&rawsock);

	char addrstr[INET6_ADDRSTRLEN];
	if (cres.err) {
		if (inet_sastos(&cres.addr, addrstr, sizeof(addrstr)) == NULL)
			perror_return("inet_ntop");
		printf("check failed: [%s]:%d: %s.\n", addrstr,
				ntohs(sas_port(&cres.addr)), cres.err_str);
	} else
		printf("check successful\n");

	return 0;
}
