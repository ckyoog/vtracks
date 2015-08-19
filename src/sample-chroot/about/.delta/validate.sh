#!/bin/bash

DEFBASE=sample-chroot
DIRBASE=${1:-$DEFBASE}
NEWDIR=$DIRBASE.new
OLDDIR=$DIRBASE

FILESUM_SCRIPT=${FILESUM_SCRIPT:-../.verify/.scripts/filesum.sh}

set -o pipefail
diff -rq --no-dereference $NEWDIR $OLDDIR &&
diff <(cd $NEWDIR && $FILESUM_SCRIPT . | sort -k2) <(cd $OLDDIR && $FILESUM_SCRIPT . | sort -k2)
