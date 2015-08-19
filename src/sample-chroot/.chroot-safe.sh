#!/bin/bash
set -e

# make sure execute script right within its directory
THISDIR=$(readlink -f $(dirname $0))
if [ $THISDIR != $PWD ]; then
	echo "You must execute this script right within its directory" >&2
	exit 1
fi
# Get super user privilege by sudo
[ `id -u` = 0 -a "$1" != RoOt ] || {
[ "$1" != RoOt ] && { exec sudo $0 RoOt "$@" || exit; }
[ `id -u` != 0 ] && { echo need root privilege >&2; exit 1; }
shift
}


CHROOTCMD="chroot . /bin/bash -l"

CHROOT_LOCK_FD=100
CHROOT_LOCK_FILE=./.chroot.lock

chroot_lock()
{
	exec 100<>$CHROOT_LOCK_FILE
	flock -x 100
}

chroot_unlock()
{
	exec 100>&-
}

chrootref()
{
	read l <$CHROOT_LOCK_FILE || true
	CHROOTNUM=${l:-0}
	[ $1 = get ] && { echo $CHROOTNUM; return; }
	if [ $1 = add ]; then
		echo $((++CHROOTNUM)) >$CHROOT_LOCK_FILE
	elif [ $1 = sub ]; then
		echo $((--CHROOTNUM)) >$CHROOT_LOCK_FILE
	fi
}


chroot_lock
if [ `chrootref get` = 0 ]; then
	echo set up chroot env
	mount -t sysfs sys ./sys
	mount -t proc proc ./proc
	mount -t devpts devpts ./dev/pts
	mountpoint ./tmp >/dev/null || mount -B /tmp ./tmp
	cp /etc/resolv.conf ./etc
	cp /etc/localtime ./etc
fi
chrootref add
chroot_unlock

$CHROOTCMD

chroot_lock
if [ `chrootref get` = 1 ]; then
	echo clear chroot env
	umount ./sys
	umount ./proc
	umount ./dev/pts
	umount ./tmp
	rm ./etc/resolv.conf
	rm ./etc/localtime
fi
chrootref sub
chroot_unlock
