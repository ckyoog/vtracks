#!/bin/bash

# The following line is the core of this script.
#	printf '%x' 256 | rev | sed -r 's/(..?)/\1 /g' | rev
# Its meaning is converting a decimal number (represented by text) into hex
# number and grouping the results in bytes (two digits of hex number per
# group), e.g., 256 -> 1 00
# The two following lines are the applications of the converting. The first
# one can be used to examine whether a system is big or little endian. The
# second one can output as same decimal number as what you input. It also
# shows an idea, like when you have a hex number, let's say '\x01\x02\x03'
# (which is actually 0x010203), how to print its decimal value. The main idea
# is to reverse the hex number to, like '\x03\x02\x01' first, and feed the
# reversed result to command 'od --endian=little -l'.
#	echo -ne $(printf '%x' 256 | rev | sed -r 's/(..?)/\1x\\/g' | rev) | od  -An -s
#	echo -ne $(printf '%x' 256 | rev | sed -r 's/(..?)/\1x\\\n/g' | rev | paste -s -d '') | od -An --endian=little -l


#### Functions of dtoh family

# $1: a decimal number
dtoh()
{
	printf '%lx\n' $1
}

# $1: see dtoh()
# $2: delimiter which the result hex number is splitted by. The result is same
#     with dtoh() if $2 is empty. the usually used delimiters are ' ', 'x0',
#     'x\\', 'x\\\n'. Have a try.
dtoh_grouped()
{
	dtoh $1 | rev | sed -r 's/(..?)/\1'"$2"'/g' | rev
}

# $1: see dtoh_grouped()
# $2: see dtoh_grouped()
dtoh_grouped_reverse()
{
	dtoh_grouped $1 "$2"'\n' | paste -s -d ''
}

# Example
dtoh $1
dtoh_grouped $1 "$2"
dtoh_grouped_reverse $1 "$2"
echo


#### Applications of dtoh family functions

# $1: see dtoh_grouped()
output_binary_in_big_endian_order()
{
	echo -ne $(dtoh_grouped $1 'x\\')
}

# $1: see dtoh_grouped_reverse()
output_binary_in_little_endian_order()
{
	echo -ne $(dtoh_grouped_reverse $1 'x\\')
}

big_or_little_endian()
{
	local v1=$(output_binary_in_big_endian_order 256 | od -An -s)
	if [ $v1 = 1 ]; then
		echo "This system is little endian"
	elif [ $v1 = 256 ]; then
		echo "This system is big endian"
	else
		echo "error"
	fi

	local v2=$(output_binary_in_little_endian_order 256 | od -An -s)
	if [ $v2 = 256 ]; then
		echo "This system is little endian"
	elif [ $v2 = 1 ]; then
		echo "This system is big endian"
	else
		echo "error"
	fi
}

verify_dtoh_grouped()
{
	[ $1 = $(output_binary_in_little_endian_order $1 | od -An --endian=little -l) ]
}

echo "big endian output:"
output_binary_in_big_endian_order $1 | od -t x1
output_binary_in_big_endian_order $1 | hexdump -C
echo

echo "little endian output:"
output_binary_in_little_endian_order $1 | od -t x1
output_binary_in_little_endian_order $1 | hexdump -C
echo

big_or_little_endian
echo

verify_dtoh_grouped $1 && echo 'verify: positive' || echo 'verify: negative'
