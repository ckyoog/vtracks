#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "in6_addr_to_64base.h"

int main()
{
	printf("encoded_strlen = %d\n", ENCODED_STRLEN);
	int i;
	void *ret;
	char ip6[ENCODED_STRLEN + 1];
	struct in6_addr addr;
#if 1
	srand(time(NULL));
	for (i = 0; i < 16; i++) {
		addr.s6_addr[i] = rand();
		printf("addr[%d] = 0x%hhX\n", i, addr.s6_addr[i]);
	}
#else
	unsigned char *adr = addr.s6_addr;
	adr[0] = 0x93;
	adr[1] = 0x42;
	adr[2] = 0x55;
	adr[3] = 0x1B;
	adr[4] = 0xDB;
	adr[5] = 0x47;
	adr[6] = 0x92;
	adr[7] = 0xFB;
	adr[8] = 0x89;
	adr[9] = 0x3C;
	adr[10] = 0xD1;
	adr[11] = 0x59;
	adr[12] = 0xCA;
	adr[13] = 0x43;
	adr[14] = 0xFF;
	adr[15] = 0x2D;

	adr[0] = 0x3B;
	adr[1] = 0x34;
	adr[2] = 0x1E;
	adr[3] = 0x6D;
	adr[4] = 0xE5;
	adr[5] = 0xA7;
	adr[6] = 0x36;
	adr[7] = 0xB5;
	adr[8] = 0xBD;
	adr[9] = 0xA9;
	adr[10] = 0x7D;
	adr[11] = 0x8B;
	adr[12] = 0x8;
	adr[13] = 0x38;
	adr[14] = 0x82;
	adr[15] = 0x31;

	for (i = 0; i < 16; i++) {
		printf("addr[%d] = 0x%hhX\n", i, addr.s6_addr[i]);
	}
#endif
	ret = encode64_in6_addr(&addr, ip6, sizeof(ip6));
	if (ret == NULL) {
		printf("encode64_in6_addr failed\n");
		return -1;
	}
	printf("encoded_ip6 = %s\n", ip6);

	char buf[INET6_ADDRSTRLEN];
	if (NULL != inet_ntop(AF_INET6, &addr, buf, sizeof(buf)))
		printf("ip6 address is %s, strlen=%ld\n", buf, strlen(buf));

	struct in6_addr addr1;
	ret = decode64_in6_addr(ip6, &addr1);
	if (ret == NULL) {
		printf("decode64_in6_addr failed\n");
		return -1;
	}
	for (i = 0; i < 16; i++) {
		printf("addr1[%d] = 0x%hhX\n", i, addr1.s6_addr[i]);
	}
	char buf1[INET6_ADDRSTRLEN];
	if (NULL != inet_ntop(AF_INET6, &addr1, buf1, sizeof(buf1)))
		printf("ip6 address is %s, strlen=%ld\n", buf1, strlen(buf1));

	if (memcmp(&addr, &addr1, sizeof(addr)) == 0)
		printf("YES! binary identical\n");
	else
		printf("NO! binary different\n");
	if (strncmp(buf, buf1, sizeof(buf)) == 0)
		printf("YES! text identical\n");
	else
		printf("NO! text different\n");
	return 0;
}
