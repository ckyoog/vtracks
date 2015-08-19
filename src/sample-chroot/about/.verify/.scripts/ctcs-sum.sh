#!/bin/bash

# !!!Must be called after chroot!!!

export INCL="/fortiddos-dev /fortiddos-root"
#export EXCL="/usr/tmp"	# This will tell filesum-wrapper.sh to remove items in EXCL after hash list is generated
export SKIP=tmp		# This will tell filesum.sh to skip the names in SKIP

# can not output to cwd because cwd will be sum'ed too
./filesum-wrapper.sh > /tmp/ctcs-sum
mv /tmp/ctcs-sum .
