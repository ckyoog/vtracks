#!/bin/bash

set -e
#false

svn status |
while read; do
	l=$REPLY
	#echo "\"$l\""
	if [[ $l =~ ^(.)(.)(.)(.)(.)(.)(.)(.)(.*)$ ]]; then
		file=${BASH_REMATCH[9]}
		flag=${BASH_REMATCH[1]}
		svnfile=$file
		[[ $file == *@* ]] && svnfile=$svnfile@
		if [ $flag = '!' ]; then
			#echo svn rm \""$svnfile"\"
			svn rm "$svnfile"
		fi
		if [ $flag = '?' ]; then
			#echo svn add \""$svnfile"\"
			svn add "$svnfile"
		fi
	fi
done

svn commit "$@"
#svn up
