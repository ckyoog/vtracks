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

set -o pipefail
diff -rq --no-dereference $NEWDIR $OLDDIR &&
diff <(cd $NEWDIR && $FILESUM_SCRIPT . | sort -k2) <(cd $OLDDIR && $FILESUM_SCRIPT . | sort -k2)
