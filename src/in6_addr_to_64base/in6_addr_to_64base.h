/*
 * Yu Kou, ckyoog@gmail.com
 */

#include <stdlib.h>
#include <netinet/in.h>

#define BITS_PER_IPV6	128
#define BITS_PER_BYTE	8
#define BITS_PER_64BASE	6
#define ENCODED_STRLEN	(BITS_PER_IPV6/BITS_PER_64BASE + 1)

char *encode64_in6_addr(struct in6_addr *addr, char *encoded_ip6, size_t size);
struct in6_addr *decode64_in6_addr(char *encoded_ip6, struct in6_addr *addr);
