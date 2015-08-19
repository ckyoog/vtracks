#!/bin/bash

DEFBASE=sample-chroot
DIRBASE=${1:-$DEFBASE}
NEWDIR=$DIRBASE.new
OLDDIR=$DIRBASE

FILESUM_SCRIPT=${FILESUM_SCRIPT:-../.verify/.scripts/filesum.sh}

fetch_two_filenames()
{
	local l=$1 p1 p2

	p1='^File (.+) is a (.+ special file|fifo|socket) while file (.+) is a (.+ special file|fifo|socket)$'
	if [[ "$l" =~ $p1 ]]; then
		IS_SPECIAL=1

		f=${BASH_REMATCH[1]}
		NEW=$f
		
		f=${BASH_REMATCH[3]}
		OLD=$f

		NEW_DEVTYPE="${BASH_REMATCH[2]}"
		OLD_DEVTYPE="${BASH_REMATCH[4]}"
	fi
}

fetch_filename()
{
	ONLY=
	INNEW=
	NEW=
	OLD=
	NEW_DEVTYPE=
	OLD_DEVTYPE=
	IS_SPECIAL=

	local l=$1
	if ! [[ "$l" =~ ^Only ]]; then
		fetch_two_filenames "$l"
	fi
}

dev_num_same()
{
	local n=$NEW o=$OLD
	[ `stat -c %t%T $n` = `stat -c %t%T $o` ]
}

validate()
{
	local l
	diff -rq --no-dereference $NEWDIR $OLDDIR | {
	hasdiff=false
	while read -r l; do
		fetch_filename "$l"
		if [ x$IS_SPECIAL = x1 ] &&
		  ([ "$NEW_DEVTYPE" = fifo -o "$NEW_DEVTYPE" = socket ] ||
		    ([ "$NEW_DEVTYPE" = "$OLD_DEVTYPE" ] && dev_num_same)); then
			continue
		fi
		hasdiff=true
		echo "$l"
	done
	! $hasdiff
	} &&
	diff <(cd $NEWDIR && $FILESUM_SCRIPT . | sort -k2) <(cd $OLDDIR && $FILESUM_SCRIPT . | sort -k2)
}

validate
