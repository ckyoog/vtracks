#!/bin/bash

export LC_ALL=
export LANG=C

for e in $EXCL; do
	e=${e//@/\\@}
	[ ${e%/} = $e ] && ep="[ *]$e/|[ *]$e$" || ep="[ *]$e"
	p="${p:+$p|}$ep"
done
EXCL_PATTERN="$p"

# `time' is a keyword, so it doesn't work if expanded from a variable.
# so I use `time' always, but use TIMEFORMAT to control it display time
# info or not.
#TIME=${TIMESTATS:+time}
[ x$TIMESTATS != x1 ] && TIMEFORMAT=

if [ "$EXCL_PATTERN" ]; then
	time ./filesum.sh $INCL | sed -r '\@'"$EXCL_PATTERN"'@d' | sort -k2
else
	time ./filesum.sh $INCL | sort -k2
fi
