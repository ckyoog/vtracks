#!/bin/bash
set -e

VER_FROMTO=$1

DEFBASE=sample-chroot
DIRBASE=${2:-$DEFBASE}
NEWDIR=$DIRBASE.new
OLDDIR=$DIRBASE

INTERMDIR=${3:-.} #/tmp

ls -d $NEWDIR $OLDDIR >/dev/null

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

MAGICSTR=`basename $0 .sh`
BF=$INTERMDIR/.${MAGICSTR}.foo
COPY_FILE_LIST=$INTERMDIR/.${MAGICSTR}.copy-file-list
DEL_FILE_LIST=$INTERMDIR/.${MAGICSTR}.del-file-list

fetch_only_filename()
{
	local l=$1 f
	f=${l#Only in }
	f=${f/: //}

	ONLY=$f
	[[ $f =~ "$OLDDIR/" ]] && INNEW=0 || INNEW=1
}

fetch_two_filenames()
{
	local l=$1 p1 p2

	p1='^File (.+) is a ((.+ special|regular( empty)*) file|fifo|socket) while file (.+) is a ((.+ special|regular( empty)*) file|fifo|socket)$'
	p2='^(Files|Symbolic links) (.+) and (.+) differ$'
	if [[ "$l" =~ $p1 ]]; then
		IS_SPECIAL=1

		f=${BASH_REMATCH[1]}
		NEW=$f
		
		f=${BASH_REMATCH[5]}
		OLD=$f

		NEW_DEVTYPE="${BASH_REMATCH[2]}"
		OLD_DEVTYPE="${BASH_REMATCH[6]}"
	elif [[ "$l" =~ $p2 ]]; then
		IS_SPECIAL=0

		f=${BASH_REMATCH[2]}
		NEW=$f

		f=${BASH_REMATCH[3]}
		OLD=$f
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
	if [[ "$l" =~ ^Only ]]; then
		fetch_only_filename "$l"
	else
		fetch_two_filenames "$l"
	fi
}

dev_num_same()
{
	local n=$NEW o=$OLD
	[ `stat -c %t%T $n` = `stat -c %t%T $o` ]
}

make_batch_file()
{
	# Assure the both files exist, but empty.
	echo -n > $COPY_FILE_LIST
	echo -n > $DEL_FILE_LIST

	local l
	# do not use -x(--exclude), which makes diff slow
	diff -rq --no-dereference $NEWDIR $OLDDIR |
	while read -r l; do
		fetch_filename "$l"
		if [ $ONLY ]; then
			if [ $INNEW = 1 ]; then
				echo $ONLY >> $COPY_FILE_LIST
			else
				echo $ONLY >> $DEL_FILE_LIST
			fi
		elif [ $NEW ]; then
			if [ x$IS_SPECIAL = x1 ]; then
				if [ "$NEW_DEVTYPE" = "$OLD_DEVTYPE" ] && dev_num_same; then
					continue
				fi
			fi
			echo $NEW >> $COPY_FILE_LIST
		else
			echo "Line not recognized, \"$l\""
			exit 1
		fi
	done
}

make_delta_script()
{
	if [ ! -s $COPY_FILE_LIST -a ! -s $DEL_FILE_LIST ]; then
		rm $COPY_FILE_LIST $DEL_FILE_LIST
		echo There is no changes, nothing to do.
		exit
	fi

	local DELTA_SCRIPT=`basename $DIRBASE`-delta-update-$VER_FROMTO.sh

	local BIN_BEGIN_FLAG=___ARCHIVE_BELOW___

	if [ $PRESERVE_SOCKET ]; then
		# cpio does not ignore socket file while tar does
		packcmd="eval (cd $NEWDIR && xargs -r find | cpio -ov)"
		unpackcmd='(cd $DESTDIR && cpio -ivu)' # never need '-d'
	else
		packcmd="tar -C $NEWDIR --index-file=/dev/stderr -cvf /dev/stdout -T -"
		unpackcmd='tar -C $DESTDIR -xvf -'
	fi
	sed -r "s@$NEWDIR/@@" $COPY_FILE_LIST | $packcmd | xz > $BF.xz
	cat - $BF.xz >$DELTA_SCRIPT <<-eof
		#!/bin/bash
		set -e
		DESTDIR=\${1:-`basename $DIRBASE`}
		cat <<EOF | xargs rm -rf
		$(sed "s|^$OLDDIR/|\$DESTDIR/|" $DEL_FILE_LIST)
		EOF
		sed '0,/^$BIN_BEGIN_FLAG$/d' \$0 | unxz | $unpackcmd
		exit
		$BIN_BEGIN_FLAG
	eof
	chmod +x $DELTA_SCRIPT
	rm $BF.xz
	rm $COPY_FILE_LIST $DEL_FILE_LIST
}

! [ $1 ] && { usage; exit 1; }

make_batch_file
make_delta_script
