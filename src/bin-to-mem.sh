#!/bin/bash
# Convert binary data to the form like \xHH\xHH..., which can be used
# as arguments for command `echo -ne'. This form of output is also as
# same as the output of script `./num-to-mem.sh'. It reads binary data
# from stdin, and writes output to stdout.
# Usage example,
# 	head -c4 /usr/bin/head | $0
# 	echo -ne `./num-to-mem.sh 2312 4` | $0

od -An -t xC | sed 's/ /\\x/g'
