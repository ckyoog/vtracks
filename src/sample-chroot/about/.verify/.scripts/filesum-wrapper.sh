#!/bin/bash

export LC_ALL=
export LANG=C

for e in $EXCL; do
	e=${e//@/\\@}
	[ ${e%/} = $e ] && ep="[ *]$e/|[ *]$e$" || ep="[ *]$e"
	p="${p:+$p|}$ep"
done
EXCL_PATTERN="$p"

TIME=${TIMESTATS:+time}

if [ "$EXCL_PATTERN" ]; then
	$TIME ./filesum.sh $INCL | sed -r '\@'"$EXCL_PATTERN"'@d' | sort -k2
else
	$TIME ./filesum.sh $INCL | sort -k2
fi
