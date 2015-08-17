#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


//#define firstbit(type) (type)((type)1<<(sizeof(type)*8-1))
//#define typemax(type) (((type)-1)^firstbit(type))
#define INT_FIRST_BIT_1_ONLY(int_type)         (int_type)((int_type)1 << (sizeof(int_type) * 8 - 1))
#define MIN_SIGNED_NEGATIVE(signed_int_type)   INT_FIRST_BIT_1_ONLY(signed_int_type)
#define MAX_SIGNED_POSITIVE(int_type)          (MIN_SIGNED_NEGATIVE(int_type) - (int_type)1)
//#define MAX_SIGNED_POSITIVE(int_type)		(((int_type)-1) ^ INT_FIRST_BIT_1_ONLY(int_type))

int main()
{
	printf("%hhd\n", 0xFF^(2-1));
	printf("%ld, long max = %ld\n", firstbit(long), typemax(long));
	printf("%ld, int64_t max = %ld\n", firstbit(int64_t), typemax(int64_t));
	printf("%d, int32_t max = %ld\n", firstbit(int32_t), typemax(int32_t));
	printf("%d, char max = %d\n", firstbit(char), typemax(char));
	printf("%d, int max = %d\n", firstbit(int), typemax(int));
	return 0;
}
