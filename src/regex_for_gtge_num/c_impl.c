#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * See comments in perl_impl.pl for details.
 */

#define ntoc(n) ((n) + 48)
#define cton(c) ((c) - 48)

static inline
char *ntos(int num, char *str, size_t *len)
{
	int onum = num, d;
	size_t olen = *len;
	int i = 0, j;
	do {
		d = num - (num / 10) * 10; // d = num % 10
		str[i++] = ntoc(d);
	} while (num /= 10);
	str[i] = 0;
	*len = i;
	for (j = i - 1, i = 0; i < j; ++i, --j) {
		str[i] += str[j];
		str[j] = str[i] - str[j];
		str[i] = str[i] - str[j];
	}
	return str;
}

void test_num2str()
{
	int num = 92185;
	char str[128];
	size_t len = sizeof str;
	printf("num =  %d, str = \"%s\"\n", num, ntos(num, str, &len));
}

#ifndef NEED_PARENTHESIS
#define NEED_PARENTHESIS 1
#endif

#if NEED_PARENTHESIS
#define LEFT_PARENTHESIS	"("
#define RIGHT_PARENTHESIS	")"
#else
#define LEFT_PARENTHESIS	""
#define RIGHT_PARENTHESIS	""
#endif

#ifndef DIGIT_CLASS
//#define DIGIT_CLASS	"\\d"	sed, grep don't support \d
#define DIGIT_CLASS	"[0-9]"
#endif

#define INIT_PREFIX	"|"
#define INIT_RANGE	"[1-9]"
#define INIT_SUFFIX_FMT	DIGIT_CLASS"{%d,}"
#define INIT_REGSTR	INIT_RANGE INIT_SUFFIX_FMT

#define PREFIX_FMT	"%s"
#define RANGE_FMT_EQ_9	"%c"
#define RANGE_FMT_NE_9	"[%c-9]"
#define SUFFIX_FMT	DIGIT_CLASS"{%d}"
#define get_fmt(pfmt, start, sfmt)	((start) != 9 ? pfmt RANGE_FMT_NE_9 sfmt : pfmt RANGE_FMT_EQ_9 sfmt)

#define MAKEREGSTR(sfmt, prefix, start, args...) \
		get_fmt(PREFIX_FMT, start, sfmt), prefix, ntoc(start), ##args
#define MAKEREGSTR_WITH_COND(prefix, start, len) \
		(len) ? get_fmt(PREFIX_FMT, start, SUFFIX_FMT) : get_fmt(PREFIX_FMT, start, RIGHT_PARENTHESIS), \
		prefix, ntoc(start), len
#define MAKEREGSTR_WITH_MORE_COND(prefix, start, len) \
		(len) ? ((len) == 1 ? get_fmt(PREFIX_FMT, start, DIGIT_CLASS) : get_fmt(PREFIX_FMT, start, SUFFIX_FMT)) \
			: get_fmt(PREFIX_FMT, start, RIGHT_PARENTHESIS), \
		prefix, ntoc(start), len

char *level0_0_s(char *num, size_t len, int ge, char *regstr, size_t *reglen)
{
	char prefix[128] = INIT_PREFIX;
	int prefix_len = sizeof(INIT_PREFIX) - 1;	//avoid calling strlen()
	int oreglen = *reglen;
	*reglen = sprintf(regstr, LEFT_PARENTHESIS INIT_REGSTR, len);
	char d, start;
	int i, l;
	for (i = 0; i < len; ++i) {
		l = len - i - 1;
		d = cton(num[i]);
		if (d == 9 && !(l == 0 && ge))
			goto loop_end;
		start = (l == 0 && ge) ? d : d + 1;
		*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR(SUFFIX_FMT, prefix, start, l));
loop_end:
		prefix[prefix_len++] = ntoc(d);
		prefix[prefix_len] = 0;
	}
	if (*reglen + 1 <= oreglen) {
		regstr[(*reglen)++] = RIGHT_PARENTHESIS[0];
		regstr[*reglen] = 0;
	}
	return regstr;
}

char *level1_0_s(char *num, size_t len, int ge, char *regstr, size_t *reglen)
{
	char prefix[128] = INIT_PREFIX;
	int prefix_len = sizeof(INIT_PREFIX) - 1;	//avoid calling strlen()
	int oreglen = *reglen;
	*reglen = sprintf(regstr, LEFT_PARENTHESIS INIT_REGSTR, len);
	char d, start;
	int i, l;
	for (i = 0; i < len; ++i) {
		l = len - i - 1;
		d = cton(num[i]);
		if (d == 9 && !(l == 0 && ge))
			goto loop_end;
		start = (l == 0 && ge) ? d : d + 1;
		*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR_WITH_COND(prefix, start, l));
loop_end:
		prefix[prefix_len++] = ntoc(d);
		prefix[prefix_len] = 0;
	}
	return regstr;
}

char *level1_1_s(char *num, size_t len, int ge, char *regstr, size_t *reglen)
{
	char prefix[128] = INIT_PREFIX;	//instead of LEFT_PARENTHESIS, keep same with next_prefix
	char next_prefix[128] = INIT_PREFIX;
	int next_prefix_len = sizeof(INIT_PREFIX) - 1;	//avoid calling strlen()
	char d = 0;
	int i, oreglen = *reglen, l = len;
	*reglen = sprintf(regstr, MAKEREGSTR(INIT_SUFFIX_FMT, LEFT_PARENTHESIS, d + 1, l));
	d = 9;	//skip next make-regstr;
	for (i = 0; i < len; ++i) {
		if (d != 9)
			*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR(SUFFIX_FMT, prefix, d + 1, l));
		l = len - i - 1;
		d = cton(num[i]);
		prefix[next_prefix_len-1] = next_prefix[next_prefix_len-1]; prefix[next_prefix_len] = 0; //avoid calling memcpy()/strcpy()
		next_prefix[next_prefix_len++] = ntoc(d);
		next_prefix[next_prefix_len] = 0;
	}
	if (d != 9 || ge)
		*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR(RIGHT_PARENTHESIS, prefix, ge ? d : d + 1));
	return regstr;
}

char *level2_0_s(char *num, size_t len, int ge, char *regstr, size_t *reglen)
{
	char prefix[128] = INIT_PREFIX;
	int prefix_len = sizeof(INIT_PREFIX) - 1;	//avoid calling strlen()
	int oreglen = *reglen;
	*reglen = sprintf(regstr, LEFT_PARENTHESIS INIT_REGSTR, len);
	char d, start;
	int i, l;
	for (i = 0; i < len; ++i) {
		l = len - i - 1;
		d = cton(num[i]);
		if (d == 9 && !(l == 0 && ge))
			goto loop_end;
		start = (l == 0 && ge) ? d : d + 1;
		*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR_WITH_MORE_COND(prefix, start, l));
loop_end:
		prefix[prefix_len++] = ntoc(d);
		prefix[prefix_len] = 0;
	}
	return regstr;
}

char *level2_1_s(char *num, size_t len, int ge, char *regstr, size_t *reglen)
{
	char prefix[128] = INIT_PREFIX;	//instead of LEFT_PARENTHESIS, keep same with next_prefix
	char next_prefix[128] = INIT_PREFIX;
	int next_prefix_len = sizeof(INIT_PREFIX) - 1;	//avoid calling strlen()
	char d = 0;
	int i, oreglen = *reglen, l = len;

	*reglen = sprintf(regstr, MAKEREGSTR(INIT_SUFFIX_FMT, LEFT_PARENTHESIS, d + 1, l));
	d = 9;	//skip next make-regstr;

	for (i = 0; i < len - 1; ++i) {
		if (d != 9)
			*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR(SUFFIX_FMT, prefix, d + 1, l));
		l = len - i - 1;
		d = cton(num[i]);
		prefix[next_prefix_len-1] = next_prefix[next_prefix_len-1]; prefix[next_prefix_len] = 0; //avoid calling memcpy()/strcpy()
		next_prefix[next_prefix_len++] = ntoc(d);
		next_prefix[next_prefix_len] = 0;
	}

	if (d != 9)
		*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR(DIGIT_CLASS, prefix, d + 1));

	d = cton(num[len - 1]);
	prefix[next_prefix_len-1] = next_prefix[next_prefix_len-1]; prefix[next_prefix_len] = 0; //avoid calling memcpy()/strcpy()
	if (d != 9 || ge)
		*reglen += snprintf(regstr + *reglen, oreglen - *reglen, MAKEREGSTR(RIGHT_PARENTHESIS, prefix, ge ? d : d + 1));

	return regstr;
}

#define DEFINE_FUNC_USING_INT_AS_ARG(func) \
static inline \
char *func(int num, int ge, char *regstr, size_t *reglen)\
{\
	char str[128], *s = str, d;\
	size_t len = sizeof str;\
	ntos(num, str, &len);\
	return func##_s(str, len, ge, regstr, reglen);\
}

DEFINE_FUNC_USING_INT_AS_ARG(level0_0)
DEFINE_FUNC_USING_INT_AS_ARG(level1_0)
DEFINE_FUNC_USING_INT_AS_ARG(level1_1)
DEFINE_FUNC_USING_INT_AS_ARG(level2_0)
DEFINE_FUNC_USING_INT_AS_ARG(level2_1)

int main(int argc, char **argv)
{
	int ge = 0;
	char *numstr = argv[1];
	int num = atol(numstr);
	if (argc > 2)
		ge = atoi(argv[2]);
	char regstr[1024];
	size_t reglen = sizeof(regstr);
	printf("%s\n", level0_0_s(numstr, strlen(numstr), ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level0_0(num, ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level1_0_s(numstr, strlen(numstr), ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level1_0(num, ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level1_1_s(numstr, strlen(numstr), ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level1_1(num, ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level2_0_s(numstr, strlen(numstr), ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level2_0(num, ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level2_1_s(numstr, strlen(numstr), ge, regstr, &reglen));

	reglen = sizeof(regstr);
	memset(regstr, 0, reglen);
	printf("%s\n", level2_1(num, ge, regstr, &reglen));
	return 0;
}
