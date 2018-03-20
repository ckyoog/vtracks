#!/bin/sh
# See how I use wget and curl with cookies

#alias wget="/Users/hongyan/workbench/wget-1.13.4/src/wget"


WEBSITE=${WEBSITE:-SISNET}
if [ $WEBSITE = SISNET ]; then
	USE_COOKIES=1
	LOGIN_URL='http://174.127.195.163/bbs/logging.php?action=login&'
	COOKIE_DATA='formhash=0bbd6af2&referer=http%3A%2F%2F174.127.195.163%2Fbbs%2Findex.php&loginfield=username&username=wantsex1999&password=iwantsex&questionid=0&answer=&cookietime=2592000&loginmode=&styleid=&loginsubmit=true'
	TEST_URL='http://174.127.195.163/bbs/forum-230-1.html'
	#"http://174.127.195.163/bbs/forumdisplay.php?fid=${FORUM}&page=$i"
	FORUM_URL_TEMPLATE='http://174.127.195.163/bbs/forum-${FORUM}-$i.html'

	FORUM_yazhou=230
	FORUM_oumei=229
	FORUM_yazhou_wuma=143
	FORUM=$FORUM_yazhou
elif [ $WEBSITE = THZLA ]; then
	USE_COOKIES=
	FORUM_URL_TEMPLATE='http://vipthz.com/forum-${FORUM}-$i.html'

	FORUM=220
fi

[ "${NO_COOKIES}" ] && USE_COOKIES=


if [ "$USE_COOKIES" -a "$1" = save-cookies ]; then
	if [ -z "$X_USE_PROXY" ]; then
		wget --keep-session-cookies --save-cookies cookies.txt --post-data "$COOKIE_DATA" "$LOGIN_URL"
	else
		curl --socks5 127.0.0.1:23456 --cookie-jar cookie.jar --data "$COOKIE_DATA" "$LOGIN_URL" > /dev/null
	fi
	exit
elif [ "$USE_COOKIES" -a "$1" = test-cookies ]; then
	if [ -z "$X_USE_PROXY" ]; then
		wget --load-cookies=cookies.txt "${TEST_URL}" -O 1
	else
		curl --socks5 127.0.0.1:23456 --cookie cookie.jar "${TEST_URL}" -o 1
	fi
	exit
fi

BEGIN=`cat begin.txt`

for ((i=$BEGIN; i<=1795; ++i)); do
	sleep 0.5
	FORUM_URL=`eval echo "$FORUM_URL_TEMPLATE"`
	if [ -z "$X_USE_PROXY" ]; then
		wget ${USE_COOKIES:+--load-cookies=cookies.txt} "$FORUM_URL" -O $i
	else
		curl --socks5 127.0.0.1:23456 ${USE_COOKIES:+--cookie cookie.jar} "$FORUM_URL" -o $i
	fi

	if [ -s exclude.txt ]; then
		while read exclude; do
			sed -e '\|'$exclude'|d' -i '' $i
		done < exclude.txt
	fi

	found=0
	while read -r key; do
		echo grep $key $i
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
