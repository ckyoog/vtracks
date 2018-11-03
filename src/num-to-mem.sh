#!/bin/sh
# TODO
# This script can be rewritten in a easier way I implement in dtoh.sh

# num_to_mem_for_echo (num, byte_count, is_bigendian=0)
# the arguments are all string of decimal.
num_to_mem_for_echo ()
{
	[ $# -lt 2 ] && return
	for n in 1 2 4 8; do [ $2 = $n ] && valid_byte_count=1; done
	[ "$valid_byte_count" != 1 ] &&
		{ echo "2nd argument must be one of 1, 2, 4, 8"; return; }

	local MEM= NUM=$1 BYTE_COUNT=$2 IS_BIGENDIAN=${3:-0} i=1 j
	while true; do
		# 0x   ff ff ff ff
		# num:  4  3  2  1
		eval local NUM_BYTE$i

# Separate number by byte.
# Make use of bit-operation of shell-native is much simpler than the deprecated
# one. We even needn't care about whether number is postive or negative.
		eval NUM_BYTE$i=$(printf %02x $((NUM&0xff)))
		NUM=$((NUM>>8&0x00ffffffffffffff))

		# Generate MEM directly, don't need another loop (below).
		if [ $IS_BIGENDIAN -eq 1 ]; then
			eval MEM='${NUM_BYTE'$i':+\\x${NUM_BYTE'$i'}}$MEM'
		elif [ $IS_BIGENDIAN -eq 0 ]; then
			eval MEM='$MEM${NUM_BYTE'$i':+\\x${NUM_BYTE'$i'}}'
		fi

		#(echo $i; eval echo NUM_BYTE$i=\$NUM_BYTE$i) >&2
		[ $i -eq $BYTE_COUNT ] && break
		i=$((i+1))
	done
	echo $MEM
	#echo $MEM >&2
}

[ $# -lt 2 ] && { echo 'Usage: '$0' <decimal number> <output bytes: 1|2|4|8> [bigendian: 1]'; exit 0; }
num_to_mem_for_echo "$@"
