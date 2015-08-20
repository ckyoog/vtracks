#!/bin/bash

XSUM="sha256sum -b"

for n in $SKIP; do
	o="${o:+$o -o }-name $n"
done
ignore_opt="${o:+-not ( ( $o ) -prune )}"

find -L "$@" $ignore_opt -type f -print0 |
xargs -r -0 $XSUM	# using `xargs' to do something to a huge set of
			# files can apparently save very much more time
			# than doing it to each file
