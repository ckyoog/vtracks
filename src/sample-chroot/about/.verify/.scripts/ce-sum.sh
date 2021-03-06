#!/bin/bash

# !!!Must be called after chroot!!!

export INCL="/bin /sbin /lib* /usr /var/db/pkg /about/.verify/.scripts /about/.delta /chroot.sh /.chroot*.sh"
#export EXCL="/usr/tmp"	# This will tell filesum-wrapper.sh to remove items in EXCL after hash list is generated
export SKIP=tmp		# This will tell filesum.sh to skip the names in SKIP

# can not output to cwd because cwd will be sum'ed too
./filesum-wrapper.sh > /tmp/ce-sum
mv /tmp/ce-sum .
