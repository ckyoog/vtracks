#!/bin/bash

# make sure execute script right within its directory
THISDIR=$(readlink -f $(dirname $0))
if [ $THISDIR != $PWD ]; then
	echo "You must execute this script within its directory" >&2
	exit 1
fi
# Get super user privilege by sudo
[ "$1" != root ] && { exec sudo $0 root "$@" || exit; }
[ `id -u` != 0 ] && { echo need root privilege >&2; exit 1; }


# must use 'sudo' to call 'chroot' in order to 'pgrep' it
CHROOTCMD="sudo chroot `readlink -e .` /bin/bash -l"

if ! pgrep -f "$CHROOTCMD" >/dev/null; then
	echo set up chroot env
	mount -t sysfs sys ./sys
	mount -t proc proc ./proc
	mount -t devpts devpts ./dev/pts
	mountpoint ./tmp >/dev/null || mount -B /tmp ./tmp
	cp /etc/resolv.conf ./etc
	cp /etc/localtime ./etc
fi

$CHROOTCMD

if ! pgrep -f "$CHROOTCMD" >/dev/null; then
	echo clear chroot env
	umount ./sys
	umount ./proc
	umount ./dev/pts
	umount ./tmp
	rm ./etc/resolv.conf
	rm ./etc/localtime
fi
