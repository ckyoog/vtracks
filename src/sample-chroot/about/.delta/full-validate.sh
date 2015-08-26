#!/bin/bash

DEFBASE=sample-chroot
DIRBASE=${1:-$DEFBASE}
NEWDIR=$DIRBASE.new
OLDDIR=$DIRBASE

_FILESUM_RELPATH=../.verify/.scripts/filesum.sh
_FILESUM_DEFPATH=`dirname $0`/$_FILESUM_RELPATH
# If run this script without any dir prefix, that means it is put in PATH,
# and that means I can't use 'dirname' directly to get its dir, I have to
# use command 'which' first to get its absolute path, then use 'dirname',
# as following,
#	[ `basename $0` = $0 ] && _FILESUM_DEFPATH=`dirname $(which $0)`/$_FILESUM_RELPATH
# But actually, if it is put in PATH, the script it calls, filesum.sh,
# has to be put in PATH too, so I actually don't need to use the relative
# path to locate filesum.sh any more. It is actually like this,
#	[ `basename $0` = $0 ] && _FILESUM_DEFPATH=filesum.sh
FILESUM_SCRIPT=$(readlink -m ${FILESUM_SCRIPT:-$_FILESUM_DEFPATH})

fetch_two_filenames()
{
	local l=$1 p1 p2

	p1='^File (.+) is a ((.+ special|regular( empty)*) file|fifo|socket) while file (.+) is a ((.+ special|regular( empty)*) file|fifo|socket)$'
	if [[ "$l" =~ $p1 ]]; then
		IS_SPECIAL=1

		f=${BASH_REMATCH[1]}
		NEW=$f
		
		f=${BASH_REMATCH[5]}
		OLD=$f

		NEW_DEVTYPE="${BASH_REMATCH[2]}"
		OLD_DEVTYPE="${BASH_REMATCH[6]}"
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
		    [ "$NEW_DEVTYPE" = "$OLD_DEVTYPE" ] && dev_num_same; then
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
