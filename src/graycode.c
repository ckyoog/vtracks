#include <stdlib.h>
#include <stdio.h>


int getbinarydigit(unsigned long n)
{
	int i;
#if 0
	for (i = 1; n >>= 1; ++i);
	fprintf(stderr, "stat = %d\n", i);
#else
	int stat = 0;
	int base = 8, max = 8, min = 0, mid;
	while (n > (1L << max)) {
		max = max + base;
		if (max > sizeof(n) * 8) {
			max = sizeof(n) * 8;
			break;
		}
	}
	//fprintf(stderr, "max = %d\n", max);
	if (n == (1L << max))
		return max + 1;
	if (n == (1L << min))
		return min + 1;
	while (1) {
		++stat;
		mid = min + ((max - min) >> 1); //(max-min)/2
		if (max == min + 1) {
			i = min;
			break;
		}
		if (n < (1L << mid)) {
			max = mid;
			continue;
		} else if (n > (1L << mid)) {
			min = mid;
			continue;
		} else {
			i = mid;
			break;
		}
	}
	i += 1;
	fprintf(stderr, "stat = %d\n", stat);
#endif
	return i;
}

char *getbinary(unsigned long n, int d)
{
	static char str[1024] = {0};
	int i;
	if (d == 0) d = getbinarydigit(n);
	for (i = d - 1; i >= 0; --i) {
		if (n & 1) str[i] = '1';
		else str[i] = '0';
		n >>= 1;
	}
	str[d] = 0;
	return str;
}

int main()
{
	int n = 3, i, v;
	int count = 1 << 3;
	for (i = 0; i < count; ++i) {
		v = (i >> 1)^i;
		printf("#%i: %s (%d)\n", i, getbinary(v, n), v);
	}
	printf("%s\n", getbinary(281474976710656, 0));
	printf("%s\n", getbinary(28147497671065, 0));
	printf("%s\n", getbinary(3, 0));
	printf("%s\n", getbinary(25, 0));
	return 0;
}
