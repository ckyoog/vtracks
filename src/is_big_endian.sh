#!/bin/bash

is_big_endian()
{
	# $(./num-to-mem.sh 1 2 1) is equivalent to '\x00\x01'
	[ $(echo -ne $(./num-to-mem.sh 1 2 1) | od -An -d) = 1 ]
}

if is_big_endian; then
	echo system is big endian
else
	echo system is little endian
fi
