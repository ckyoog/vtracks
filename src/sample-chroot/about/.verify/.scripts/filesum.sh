#!/bin/bash

XSUM="sha256sum -b"

for n in $SKIP; do
	o="${o:+$o -o }-name $n"
done
ignore_opt="${o:+-not ( ( $o ) -prune )}"

for i; do
	if [ -d $i ]; then
		find -L $i $ignore_opt -not -type d -exec $XSUM '{}' ';'
	else
		$XSUM $i
	fi
done
