#!/bin/bash
set -e
shopt -s expand_aliases
alias rm_intermediate_files='rm -rf ce-sum ce-sum.xz* ctcs-sum ctcs-sum.xz*'
trap 'rm_intermediate_files' EXIT

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

NEWROOT=../../..
VERSION=$NEWROOT/about/VERSION
LASTVER=$NEWROOT/about/.LASTVER
VERIFY_FILE_DIR=$NEWROOT/about/.verify

CHROOT_SCRIPTDIR=/about/.verify/.scripts

# The below two variables, GPGUSER and TTYUSER, must be same, otherwise
# it'll make pinentry doesn't work. I.e. you must login as GPGUSER, the
# user who got GPG directory in his HOME, or put GPG directory into the
# TTYUSER's HOME.
TTYUSER=$(stat -c'%U' `tty`)
GPGUSER=${TTYUSER}

# $1:	newroot
# $2:	cmd(abs/rel/no path)
# $3:	dir path after chroot which you want to go in to run cmd
#	mostly it's where the cmd is, and can be omitted when cmd is in PATH
chroot-runcmd()
{
	chroot $1 /bin/bash -l -c "cd $3 && $2"
}

prepare()
{
	rm_intermediate_files

	# launch my gpg-agent manually, otherwise subsequent 'gpg' will
	# launch root's gpg-agent automatically, and root's gpg-agent
	# will launch root's pinentry, and root's pinentry is not able
	# to work if root privilege is obtained by `su' or `sudo'. Because
	# the owner of TTY or DISPLAY is still the user calling `su' or
	# `sudo', rather than root. (There's another way besides launching
	# my gpg-agent manually to let root's pinentry work. That's like
	# this,
	# (chown root `tty`; unset DISPLAY; gpg ...; chown <original> `tty`)
	sudo -u $TTYUSER gpgconf --launch gpg-agent
}

verify()
{
	local what=$1
	HOME=/home/$GPGUSER gpg --verify $VERIFY_FILE_DIR/$what-sum.xz.enc.gpg-dsig $VERIFY_FILE_DIR/$what-sum.xz.enc
	sha256sum $VERSION | cut -d' ' -f1 | cat - $VERSION | openssl enc -aes-256-cfb -d -in $VERIFY_FILE_DIR/$what-sum.xz.enc -out $what-sum.xz -pass stdin -nosalt # -nosalt should not have been used, nevertheless, my purpose is not encrypt, so it is not a big deal. using it can keep encrypt result same as long as password is same.
	unxz $what-sum.xz
	mv $what-sum /tmp/$what-sum.ref

	chroot-runcmd $NEWROOT ./$what-sum.sh $CHROOT_SCRIPTDIR
	mv $what-sum /tmp/$what-sum.cur
	diff /tmp/$what-sum.ref /tmp/$what-sum.cur -q
	echo "=== ${what^^} VERSION Verification OK!"
}

case $1 in
ce|ctcs|all)
	prepare
	;;&
ce|ctcs)
	verify $1
	;;
all)
	verify ce
	verify ctcs
	;;
esac
