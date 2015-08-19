#!/bin/bash
set -e

VER_FROMTO=$1
BF=${3:-.foo}

DEFBASE=sample-chroot
DIRBASE=${2:-$DEFBASE}
NEWDIR=$DIRBASE.new
OLDDIR=$DIRBASE

usage()
{
	cat <<-eof
	$0 <from-to> [dir]
	   from-to: version from-to string, e.g. 0.6-0.7
	   dir: basename of directory used to compare and sync,
	        e.g. compare dir.new/ to dir/, then sync dir/.
	        default is '$DEFBASE'
	eof
}

make_batch_file()
{
	rsync -i --only-write-batch=$BF -I -a --delete $NEWDIR/ $OLDDIR
}

make_delta_script()
{
	#local RSYNC_CMD=`sed -r 's/(--read-batch=)'"$BF"'/\1-/; s/\\$\{?1.*$/\\$1/' $BF.sh | head -n1`
	local RSYNC_CMD=`sed -r 's/(--read-batch=)'"$BF"'/\1-/' $BF.sh | head -n1`
	local DELTA_SCRIPT=$DIRBASE-delta-update-$VER_FROMTO.sh

	local BIN_BEGIN_FLAG=___ARCHIVE_BELOW___

	xz $BF
	cat - $BF.xz >$DELTA_SCRIPT <<-eof
		#!/bin/bash
		sed '0,/^$BIN_BEGIN_FLAG$/d' \$0 | unxz | $RSYNC_CMD
		exit
		$BIN_BEGIN_FLAG
	eof
	chmod +x $DELTA_SCRIPT
	rm $BF.xz $BF.sh
}

! [ $1 ] && { usage; exit 1; }

make_batch_file
make_delta_script
