#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#define DBG(format, args...) fprintf(stderr, format, ##args)

/* 'snmp_synch_input' is copied from net-snmp source code */
//#include <net-snmp/library/snmp_debug.h>
static int
snmp_synch_input(int op,
                 netsnmp_session * session,
                 int reqid, netsnmp_pdu *pdu, void *magic)
{
    struct synch_state *state = (struct synch_state *) magic;
    int             rpt_type;

    if (reqid != state->reqid && pdu && pdu->command != SNMP_MSG_REPORT) {
        DEBUGMSGTL(("snmp_synch", "Unexpected response (ReqID: %d,%d - Cmd %d)\n",
                                   reqid, state->reqid, pdu->command ));
        return 0;
    }

    state->waiting = 0;
    DEBUGMSGTL(("snmp_synch", "Response (ReqID: %d - Cmd %d)\n",
                               reqid, (pdu ? pdu->command : -1)));

    if (op == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE && pdu) {
        if (pdu->command == SNMP_MSG_REPORT) {
            rpt_type = snmpv3_get_report_type(pdu);
            if (SNMPV3_IGNORE_UNAUTH_REPORTS ||
                rpt_type == SNMPERR_NOT_IN_TIME_WINDOW) {
                state->waiting = 1;
            }
            state->pdu = NULL;
            state->status = STAT_ERROR;
            session->s_snmp_errno = rpt_type;
            SET_SNMP_ERROR(rpt_type);
        } else if (pdu->command == SNMP_MSG_RESPONSE) {
            /*
             * clone the pdu to return to snmp_synch_response 
             */
            state->pdu = snmp_clone_pdu(pdu);
            state->status = STAT_SUCCESS;
            session->s_snmp_errno = SNMPERR_SUCCESS;
        }
        else {
            char msg_buf[50];
            state->status = STAT_ERROR;
            session->s_snmp_errno = SNMPERR_PROTOCOL;
            SET_SNMP_ERROR(SNMPERR_PROTOCOL);
            snprintf(msg_buf, sizeof(msg_buf), "Expected RESPONSE-PDU but got %s-PDU",
                     snmp_pdu_type(pdu->command));
            snmp_set_detail(msg_buf);
            return 0;
        }
    } else if (op == NETSNMP_CALLBACK_OP_TIMED_OUT) {
        state->pdu = NULL;
        state->status = STAT_TIMEOUT;
        session->s_snmp_errno = SNMPERR_TIMEOUT;
        SET_SNMP_ERROR(SNMPERR_TIMEOUT);
    } else if (op == NETSNMP_CALLBACK_OP_DISCONNECT) {
        state->pdu = NULL;
        state->status = STAT_ERROR;
        session->s_snmp_errno = SNMPERR_ABORT;
        SET_SNMP_ERROR(SNMPERR_ABORT);
    }

    return 1;
}

const char *netsnmp_errstring(netsnmp_session *session, const char *name)
{
	static char errbuf[1024];
	static int errlen = sizeof(errbuf);
	int sys_errno, snmp_errno;
	/* I make the last argument NULL, because I don't want
	 * malloc memory, then free it.
	 */
	snmp_error(session, &sys_errno, &snmp_errno, NULL);
	snprintf(errbuf, errlen, "netsnmp:%s error (%d)%s, (%d)%s", name,
			snmp_errno, snmp_api_errstring(snmp_errno),
			sys_errno, strerror(sys_errno));
	return errbuf;
}

/*
 * The very first routine, then call netsnmp_open().
 * Or call netsnmp_init() instead. (see netsnmp_init() for details)
 */
netsnmp_session *netsnmp_sess_init(netsnmp_session *session, int version, const char *community, const char *dst)
{
	/*  
	 * read in MIB database and initialize the snmp library
	 */ 
	init_snmp("snmpchecker");

	snmp_sess_init(session);

	switch (version) {
	case 1:
		session->version = SNMP_VERSION_1;
		break;
	case 2:
		session->version = SNMP_VERSION_2c;
		break;
	case 3:
		session->version = SNMP_VERSION_3;
		break;
	default:
		DBG("Can't determine a valid SNMP version for the session\n");
		return NULL;
	}
	session->peername = (char *)dst;

	if (session->version == SNMP_VERSION_1 ||
	    session->version == SNMP_VERSION_2c) {
		if (community == NULL) {
			session->community = NULL;
			session->community_len = 0;
		} else {
			session->community = (unsigned char *)community;
			session->community_len = strlen(community);
		}
	}
	return session;
}

/*
 * call netsnmp_sess_init() first, then this one.
 * Or call netsnmp_init() instead. (see netsnmp_init() for details)
 */
netsnmp_session *netsnmp_open(netsnmp_session *session)
{
	netsnmp_session *ss;
	ss = snmp_open(session);
	if (ss == NULL) {
		DBG("%s\n", netsnmp_errstring(session, "snmp_open"));
		return NULL;
	}
	return ss;
}

/*
 * The combination of netsnmp_sess_init() and netsnmp_open()
 * Can all this one directly, instead of calling those two.
 */
netsnmp_session *netsnmp_init(int version, const char *community, const char *dst)
{
	netsnmp_session sess;
	netsnmp_session *session = &sess;

	if (netsnmp_sess_init(session, version, community, dst) == NULL)
		return NULL;

	return netsnmp_open(session);
}

inline int netsnmp_sess_sock(netsnmp_session *ss, fd_set *fdset)
{
	void *sessp = snmp_sess_pointer(ss);
	int sock = ((struct session_list *)sessp)->transport->sock;
	if (sock >= 0 && fdset != NULL) {
		FD_SET(sock, fdset);
	}
	return sock;
}

int netsnmp_send(netsnmp_session *ss, int isgetnext, const char *oidname, struct synch_state *state)
{
	oid name[MAX_OID_LEN];
	size_t name_length = sizeof(name);
	netsnmp_pdu *pdu;

	memset(state, 0, sizeof(*state));

	pdu = snmp_pdu_create(isgetnext ? SNMP_MSG_GETNEXT : SNMP_MSG_GET);
	if (!snmp_parse_oid(oidname, name, &name_length)) {
		DBG("parsing oidname [%s] error\n", oidname);
		snmp_free_pdu(pdu);
		return -1;
	} else
		snmp_add_null_var(pdu, name, name_length);

	if ((state->reqid = snmp_send(ss, pdu)) == 0) {
		snmp_free_pdu(pdu);
		state->status = STAT_ERROR;
		DBG("%s\n", netsnmp_errstring(ss, "snmp_send"));
		return -1;
	}
	return 0;
}

/* -2: timeout
 * -1: error
 *  0: successful
 *  1: read again
 */
int netsnmp_recv(netsnmp_session *ss, struct synch_state *state, netsnmp_pdu **response)
{
	void *sessp = snmp_sess_pointer(ss);
	fd_set fdset;
	FD_ZERO(&fdset);
	netsnmp_sess_sock(ss, &fdset);

	ss->callback = snmp_synch_input;
	ss->callback_magic = (void *)state;
	*response = NULL;

	if (snmp_sess_read(sessp, &fdset) == -1) {
		DBG("%s\n", netsnmp_errstring(ss, "snmp_sess_read"));
		return -1;
	}

	if (state->waiting) {
		return 1;
	}

	switch (state->status) {
	case STAT_SUCCESS:
		*response = state->pdu;
		if (response == NULL) {
			DBG("snmp_sess_read error, no response\n");
			return -1;
		}
		return 0;
	case STAT_ERROR:
		DBG("snmp_sess_read synch state status error\n");
		return -1;
	case STAT_TIMEOUT:
		DBG("snmp_sess_read timeout\n");
		return -2;
	default:
		DBG("synch state status is unknown\n");
		return -1;
	}
}

/* NULL if successful, other return a static error string */
const char *netsnmp_check_response(netsnmp_pdu *response)
{
	int count;
	netsnmp_variable_list *vars;

	static char errbuf[1024];	/* assume enough. but once it's not, oops... */
	const int errlen = sizeof(errbuf);
	int slen;
	if (response->errstat == SNMP_ERR_NOERROR) {
		return NULL;
	} else {
		slen = snprintf(errbuf, errlen, "Error in packet.\nReason: %s\n",
				snmp_errstring(response->errstat));
		if (response->errindex != 0) {
			slen += snprintf(errbuf + slen, errlen - slen, "Failed object: ");
			for (count = 1, vars = response->variables;
					vars && count != response->errindex;
					vars = vars->next_variable, count++);
			if (vars)
				slen += snprint_objid(errbuf + slen, errlen - slen, vars->name, vars->name_length);
			errbuf[slen] = '\n';
		}
		return errbuf;
	}
}

long netsnmp_parse_var(const netsnmp_variable_list *var, u_char type, ...)
{
	if (var->type == SNMP_NOSUCHOBJECT
	    || var->type == SNMP_NOSUCHINSTANCE
	    || var->type == SNMP_ENDOFMIBVIEW) {
		DBG("Can not get valid snmp return value\n");
		return -1;
	}

	if (type != var->type) {
		DBG("Can not get expected snmp return value\n");
		return -2;
	}

	long ret = 0;
	va_list va;
	va_start(va, type);

	switch (var->type) {
	case ASN_INTEGER:
	{
		long *integer = va_arg(va, long *);
		*integer = *var->val.integer;
		ret = sizeof(*integer);
		break;
	}
	case ASN_OCTET_STR:
	{
		char *string = va_arg(va, char *);
		int len = va_arg(va, int);
		strncpy(string, (char *)var->val.string, MIN(var->val_len, len - 1));
		string[MIN(var->val_len, len - 1)] = '\0';
		ret = var->val_len;
		break;
	}
#if 0
	case ASN_BIT_STR:
		return sprint_realloc_bitstring(buf, buf_len, out_len,
				allow_realloc, var, enums, hint,
				units);
	case ASN_OPAQUE:
		return sprint_realloc_opaque(buf, buf_len, out_len, allow_realloc,
				var, enums, hint, units);
#endif
	case ASN_OBJECT_ID:
	{
		oid *oids = va_arg(va, oid *);
		int len = va_arg(va, int);
		memcpy(oids, var->val.objid, MIN(var->val_len, len * sizeof(oid)));
		ret = var->val_len / sizeof(oid);
		break;
	}
#if 0
	case ASN_TIMETICKS:
		return sprint_realloc_timeticks(buf, buf_len, out_len,
				allow_realloc, var, enums, hint,
				units);
	case ASN_GAUGE:
		return sprint_realloc_gauge(buf, buf_len, out_len, allow_realloc,
				var, enums, hint, units);
#endif
	case ASN_COUNTER:
	{
		unsigned int *counter = va_arg(va, unsigned int *);
		*counter = (*var->val.integer & 0xffffffff);
		ret = sizeof(*counter);
		break;
	}
#if 0
	case ASN_IPADDRESS:
		return sprint_realloc_ipaddress(buf, buf_len, out_len,
				allow_realloc, var, enums, hint,
				units);
	case ASN_NULL:
		return sprint_realloc_null(buf, buf_len, out_len, allow_realloc,
				var, enums, hint, units);
#endif
	case ASN_UINTEGER:
	{
		unsigned long *integer = va_arg(va, unsigned long *);
		*integer = *var->val.integer;
		ret = sizeof(*integer);
		break;
	}
#if 0
	case ASN_COUNTER64:
#ifdef NETSNMP_WITH_OPAQUE_SPECIAL_TYPES
	case ASN_OPAQUE_U64:
	case ASN_OPAQUE_I64:
	case ASN_OPAQUE_COUNTER64:
#endif                          /* NETSNMP_WITH_OPAQUE_SPECIAL_TYPES */
		return sprint_realloc_counter64(buf, buf_len, out_len,
				allow_realloc, var, enums, hint,
				units);
#ifdef NETSNMP_WITH_OPAQUE_SPECIAL_TYPES
	case ASN_OPAQUE_FLOAT:
		return sprint_realloc_float(buf, buf_len, out_len, allow_realloc,
				var, enums, hint, units);
	case ASN_OPAQUE_DOUBLE:
		return sprint_realloc_double(buf, buf_len, out_len, allow_realloc,
				var, enums, hint, units);
#endif                          /* NETSNMP_WITH_OPAQUE_SPECIAL_TYPES */
#endif
	default:
		DBG("Unsupported snmp type\n");
		ret = -3;
		break;

	}
	va_end(va);
	return ret;
}

inline
int netsnmp_close(netsnmp_session *ss)
{
	return snmp_close(ss);
}

inline static
int oidtostr(const oid *objid, size_t objidlen, char *strbuf, size_t *buflen)
{
	int i, slen, ret;
	for (slen = i = *strbuf = 0; i < objidlen; ++i) {
		ret = snprintf(strbuf + slen, *buflen - slen,
				".%" NETSNMP_PRIo "u", objid[i]);
		if (ret < 0)
			return ret;
		slen += ret;
		if (slen >= *buflen)
			return 1; /* space is not enough */
	}
	*buflen = slen;
	return 0;
}

inline static
int strtooid(const char *str, size_t strlen, oid *oidbuf, size_t *oidlen)
{
	int i;
	char *cur, *next, dot = '.';
	for (i = 0, cur = strchr(str, dot); cur && i < *oidlen; cur = next, ++i) {
		++cur;
		next = strchr(cur, dot);
		oidbuf[i] = (oid)atol(cur);
	}
	if (i >= *oidlen && cur)
		return 1; /* need more space */
	*oidlen = i;
	return 0;
}

struct oid_kv {
	int isgetnext;
	char *oidname;
	u_char val_type;
	size_t unit;
	char val_buf[1024];
} req_oids[] = {
	{ 0, ".1.3.6.1.4.1.2021.4.5.0", ASN_INTEGER },
	{ 0, ".1.3.6.1.4.1.2021.4.6.0", ASN_INTEGER },
	{ 0, ".2.3.6.1.2.1.25.2.3.1.5..1", ASN_INTEGER },
	{ 0, ".1.3.6.1.2.1.25.2.3.1.5.3", ASN_INTEGER },
	{ 0, ".1.3.6.1.2.1.25.2.3.1.5.31", ASN_INTEGER },
	{ 0, ".1.3.6.1.2.1.25.2.3.1.4.31", ASN_INTEGER },
	{ 0, ".1.3.6.1.2.1.25.2.3.1.3.31", ASN_OCTET_STR, 1 },
	{ 0, ".1.3.6.1.2.1.25.2.3.1.2.31", ASN_OBJECT_ID, sizeof(oid) }
};

int main(int argc, char **argv)
{
	int version = 2;
	const char *community = "public";
	const char *dst;

	if (argc < 2) {
		fprintf(stderr, "%s <ip>\n", argv[0]);
		exit(1);
	}

	dst = argv[1];

	int ret;
	const char *print_errstr;
	struct synch_state state;
	netsnmp_session *ss;
	netsnmp_pdu *response = NULL;

	ss = netsnmp_init(version, community, dst);
	if (ss == NULL) {
		fprintf(stderr, "netsnmp_init failed\n");
		exit(1);
	}

	int i, isgetnext, unit, total = 0;
	u_char val_type;
	char *oidname, *val_buf;
	for (i = 0; i < sizeof(req_oids)/sizeof(req_oids[0]); ++i) {

	oidname = req_oids[i].oidname;
	val_type = req_oids[i].val_type;
	isgetnext = req_oids[i].isgetnext;
	unit = req_oids[i].unit;
	val_buf = req_oids[i].val_buf;
	if (val_type != ASN_INTEGER)
		total = sizeof(req_oids[i].val_buf);

	ret = netsnmp_send(ss, isgetnext, oidname, &state);
	if (ret == -1) {
		fprintf(stderr, "netsnmp_send failed\n");
		goto end;
	}

	fd_set fdset;
	int snmp_sock = netsnmp_sess_sock(ss, &fdset);
	//int f;
	//fcntl(snmp_sock, F_GETFL, &f);
	//fcntl(snmp_sock, F_SETFL, f | O_NONBLOCK);
	struct timeval to = { .tv_usec = 0 };
read_again:
	to.tv_sec = 5;
	ret = select(snmp_sock + 1, &fdset, NULL, NULL, &to);
	if (ret == -1) {
		if (errno == EINTR)
			goto read_again;
		perror("select");
		goto end;
	} else if (ret == 0) {
		fprintf(stderr, "select timeout\n");
		goto end;
	}

	ret = netsnmp_recv(ss, &state, &response);
	if (ret < 0) {
		fprintf(stderr, "netsnmp_recv failed\n");
		goto end;
	} else if (ret == 1) {
		fprintf(stderr, "need read again\n");
		goto read_again;
	}

	print_errstr = netsnmp_check_response(response);
	if (print_errstr) {
		fprintf(stderr, print_errstr);
		goto end;
	}

	/*
	long disksize; // .1.3.6.1.2.1.25.2.3.1.5.1
	netsnmp_parse_var(response->variables, ASN_INTEGER, &disksize);
	printf("disksize = %ld\n", disksize);
	*/

	ret = netsnmp_parse_var(response->variables, val_type, val_buf, total);
	if (ret < 0) {
		fprintf(stderr, "netsnmp_parse_var error: %d\n", ret);
		goto end;
	}
	switch (val_type) {
	case ASN_INTEGER:
		printf("%s = %ld\n", oidname, *(long *)val_buf);
		break;
	case ASN_OCTET_STR:
		if (ret >= total)
			fprintf(stderr, "val_buf is not enough for ASN_OCTET_STR\n");
		else
			printf("%s = %s\n", oidname, val_buf);
		break;
	case ASN_OBJECT_ID:
		if (ret > total)
			fprintf(stderr, "val_buf is not enough for ASN_OBJECT_ID\n");
		else {
			printf("%s = ", oidname);
			for (i = 0; i < ret/unit; ++i)
				printf(".%"NETSNMP_PRIo"d", ((oid *)val_buf)[i]);
			printf("\n");
		}
		break;
	default:
		printf("unkown\n");
		break;
	}

end:
	if (response) {
		snmp_free_pdu(response);
		response = NULL;
	}

	} /* for (req_oids) */

	netsnmp_close(ss);
	return 0;
}
