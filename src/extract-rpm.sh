#!/bin/sh

for rpm in *.rpm; do dir=${rpm%%.rpm}; (mkdir $dir; cd $dir; rpm2cpio ../$rpm | cpio -iv); echo; done
