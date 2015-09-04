/*
 * Yu Kou, ckyoog@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "in6_addr_to_64base.h"

static const char bits_per_byte = BITS_PER_BYTE, bits_per_64base = BITS_PER_64BASE;
static const int encoded_strlen = ENCODED_STRLEN;

static char num_64base[64];
static void init_num_64base() __attribute__((constructor));

void init_num_64base()
{
	char i;
	for (i = 0; i < 10; ++i)
		num_64base[i] = '0' + i;
	for (i = 0; i < 26; ++i)
		num_64base[i + 10] = 'A' + i;
	for (i = 0; i < 26; ++i)
		num_64base[i + 10 + 26] = 'a' + i;
	num_64base[62] = '$';
	num_64base[63] = '=';

#ifdef KOUYU_DEBUG
	for (i = 0; i < sizeof(num_64base); ++i)
		printf("num_64base[%d] = %c\n", i, num_64base[i]);
#endif
}

char *encode64_in6_addr(struct in6_addr *addr, char *encoded_ip6, size_t size)
{
	if (size - 1 < encoded_strlen)
		return NULL;

	unsigned char *a = addr->s6_addr;
	int i, j;
	char n, bit, bitmask;

	char used_bit = 0;
	char left_bit = bits_per_byte - used_bit;

	for (i = 0, j = 0; i < encoded_strlen; ++i) {
		bit = left_bit > bits_per_64base ? bits_per_64base : left_bit;
		bitmask = (1 << bit) - 1;
		n = (a[j] >> used_bit) & bitmask;
		do {
			if (bit < 6) {
				if ((++j) >= 16) break;
				bitmask = (1 << (bits_per_64base - bit)) - 1;
				n += (a[j] & bitmask) << bit;
				used_bit = bits_per_64base - bit;
			} else {
				used_bit += bit;
			}
			left_bit = bits_per_byte - used_bit;
		} while (0);
#ifdef KOUYU_DEBUG
		printf("i=%d, left_bit=%d, used_bit=%d, n=%d, c=%c\n", i, left_bit, used_bit, n, num_64base[n]);
#endif

		encoded_ip6[i] = num_64base[n];
	}
	encoded_ip6[i] = 0;
	return encoded_ip6;
}

struct in6_addr *decode64_in6_addr(char *encoded_ip6, struct in6_addr *addr)
{
	if (strlen(encoded_ip6) < encoded_strlen)
		return NULL;

	unsigned char *a = addr->s6_addr;
	int i, j;
	char n, bit, bitmask;

	char used_bit = 0;
	char need_bit = bits_per_byte - used_bit;

	a[0] = 0;
	for (i = 0, j = 0; i < encoded_strlen; ++i) {
		if (encoded_ip6[i] >= '0' && encoded_ip6[i] < '0' + 10)
			n = encoded_ip6[i] - '0';
		else if (encoded_ip6[i] >= 'A' && encoded_ip6[i] < 'A' + 26)
			n = encoded_ip6[i] - 'A' + 10;
		else if (encoded_ip6[i] >= 'a' && encoded_ip6[i] < 'a' + 26)
			n = encoded_ip6[i] - 'a' + 10 + 26;
		else if (encoded_ip6[i] == '$')
			n = 62;
		else if (encoded_ip6[i] == '=')
			n = 63;
		else
			return NULL;
#ifdef KOUYU_DEBUG
		printf("c=%c c=%c n=%d\n", encoded_ip6[i], num_64base[n], n);
#endif

		bit = need_bit > bits_per_64base ? bits_per_64base : need_bit;
		bitmask = (1 << need_bit) - 1;
		a[j] += (n & bitmask) << used_bit;
		do {
			if (bit < 6) {
				if ((++j) >= 16) break;
				bitmask = (1 << (bits_per_64base - bit)) - 1;
				a[j] = (n >> bit) & bitmask;
				used_bit = bits_per_64base - bit;
			} else {
				used_bit += bit;
			}
			need_bit = bits_per_byte - used_bit;
		} while (0);
	}
	return addr;
}

