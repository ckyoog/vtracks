#!/bin/sh

if [ "$1" = save-cookies ]; then
	wget --keep-session-cookies --save-cookies cookies.txt --post-data "formhash=0bbd6af2&referer=http%3A%2F%2F174.127.195.163%2Fbbs%2Findex.php&loginfield=username&username=wantsex1999&password=iwantsex&questionid=0&answer=&cookietime=2592000&loginmode=&styleid=&loginsubmit=true" "http://174.127.195.163/bbs/logging.php?action=login&"
	exit
elif [ "$1" = test-cookies ]; then
	wget --load-cookies=cookies.txt "http://174.127.195.163/bbs/forum-230-1.html" -O 1
	exit
fi

BEGIN=`cat begin.txt`

for ((i=$BEGIN; i<=1795; ++i)); do
	#"http://174.127.195.163/bbs/forumdisplay.php?fid=230&page=$i"
	wget --load-cookies=cookies.txt "http://174.127.195.163/bbs/forum-230-$i.html" -O $i

	if [ -s exclude.txt ]; then
		while read exclude; do
			sed -i '\|'$exclude'|d' $i
		done < exclude.txt
	fi

	found=0
	while read key; do
		if grep $key $i; then
			found=1
			break
		fi
	done < key.txt
	if [ $found = 0 ]; then
		echo 
		rm $i
	fi
done
