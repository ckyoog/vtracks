#!/bin/bash

CMD=$1
TMPDIR=.tmpdir-pgp

SSLKEY_PREFIX=
PRIDIR=${PRIDIR:-pri-keys}
PUBDIR=${PUBDIR:-pub-keys}
SSLPRI=${SSLPRI:-$PRIDIR/${SSLKEY_PREFIX}ssl-pri.key}
SSLPUB=${SSLPUB:-$PUBDIR/${SSLKEY_PREFIX}ssl-pub.key}

# constant value
SUFX_GPG_SIG=.gpg-sig
SUFX_GPG_DETACH_SIG=.gpg-dsig
SUFX_GPG_ENC=.gpg-enc
SUFX_GPG_PASS_ENC=.gpg-pass-enc
SUFX_SSL_SIG=.ssl-sig
SUFX_SSL_SIG_ENC=.ssl-sig-enc
SUFX_SSL_ENC=.ssl-enc
SUFX_SSL_PASS_ENC=.ssl-pass-enc

usage()
{
	echo "$0 <cmd> [userid|file] [userid]"
	cat <<-EOF
	cmd:
	  new-keys <userid>
	  clean-keys
	  clean
	  sign <file>
	  verify <file>
	  sign-enc <file>
	  verify-dec <file>
	  pass-enc <file>
	  pass-dec <file>
	  enc <file> <userid>
	  dec <file>
	EOF
}

prepare()
{
	mkdir -p $PRIDIR $PUBDIR

	case $CMD in
	new-keys|*enc|*dec|sign|verify)
		#echo 1
		[ $# -lt 2 ] && { usage; exit 1; }
		;;&
	enc)
		#echo 2
		USERID=$3
		[ $# -lt 3 ] && { usage; exit 1; }
		;;&
	new-keys)
		#echo 3
		USERID=$2
		;;
	*enc|*dec|sign|verify)
		#echo 4
		SRC=$2
		mkdir -p $TMPDIR
		cp $SRC $TMPDIR || exit
		SRC=$TMPDIR/`basename $SRC`
		PASS=$SRC-pass
		;;
	esac
}

prepare "$@"

case $CMD in
new-keys)
	echo -------- genkey by gpg --------
	HOME=$PRIDIR gpg --quick-gen-key "$USERID"  #--gen-key
	HOME=$PRIDIR gpg --output $PUBDIR/gpg-pubkey --export "$USERID"
	HOME=$PUBDIR gpg --import $PUBDIR/gpg-pubkey; rm $PUBDIR/gpg-pubkey

	echo -------- genkey by ssl --------
	openssl genrsa -out $SSLPRI 2048 # with -aes128|-camellia128|-des|... to encrypt the private key with specified cipher
	#openssl rsa -outform PEM -out $PRIDIR/prikey-copy -pubout -in $SSLPRI  # prikey-copy is same with $SSLPRI, unless with -aes128|-camellia128|-des|... which will add passphrase to the output private key
	openssl rsa -outform PEM -out $SSLPUB -pubout -in $SSLPRI
	;;

clean-keys)
	rm -rf $PUBDIR $PRIDIR $TMPDIR
	;;

clean)
	rm -rf $TMPDIR
	;;

sign)
	echo -------- signed by gpg --------
	SUFX=$SUFX_GPG_SIG
	HOME=$PRIDIR gpg --yes --sign --output ${SRC}${SUFX} $SRC
	(exit) && echo ok || echo failed

	echo -------- detach signed by gpg --------
	SUFX=$SUFX_GPG_DETACH_SIG
	HOME=$PRIDIR gpg --yes --detach-sign --output ${SRC}${SUFX} $SRC
	(exit) && echo ok || echo failed

	echo -------- signed by ssl --------
	SUFX=$SUFX_SSL_SIG
	openssl dgst -sha256 -sign $SSLPRI -out ${SRC}${SUFX} $SRC  # openssl sha256
	(exit) && echo ok || echo failed
	;;

verify)
	set -o pipefail
	echo ======== verified by gpg ========
	SUFX=$SUFX_GPG_SIG
	HOME=$PUBDIR gpg --decrypt ${SRC}${SUFX} | diff -q - $SRC
	(exit) && r=ok || r=failed; echo ======== $r ========
	set +o pipefail

	echo ======== detach verified by gpg ========
	SUFX=$SUFX_GPG_DETACH_SIG
	HOME=$PUBDIR gpg --verify ${SRC}${SUFX} $SRC
	(exit) && r=ok || r=failed; echo ======== $r ========

	echo ======== verified by ssl ========
	SUFX=$SUFX_SSL_SIG
	openssl dgst -sha256 -verify $SSLPUB -signature ${SRC}${SUFX} $SRC  # openssl sha256
	(exit) && r=ok || r=failed; echo ======== $r ========
	;;

sign-enc)  # encrypt with private key; encrypt with the way you sign, but not sign
	echo -------- encrypted with private key by ssl --------
	SUFX=$SUFX_SSL_SIG_ENC
	openssl rsautl -sign -inkey $SSLPRI -in $SRC -out ${SRC}${SUFX}
	(exit) && echo ok || echo failed

	# signed by ssl if file is too large for key size
	echo -------- encrypted with private key by ssl '(in case file too large)' --------
	#openssl rand -base64 32 > ${PASS}
	#openssl enc -aes-256-cfb -in $SRC -out ${SRC}${SUFX_SSL_PASS_ENC}${SUFX_SSL_SIG_ENC}-part1 -pass file:${PASS}  # openssl aes-256-cfb
	#openssl rsautl -sign -inkey $SSLPRI -in ${PASS} -out ${PASS}${SUFX_SSL_SIG_ENC}${SUFX_SSL_SIG_ENC}-part2
	#rm ${PASS}
	# the above lines do same thing with below ones, except they need to create file $PASS explicitly and don't need to
	# use my public key to decrypt, so can use either of them, above or below.
	openssl rand -base64 32 | openssl rsautl -sign -inkey $SSLPRI -out ${PASS}${SUFX_SSL_SIG_ENC}${SUFX_SSL_SIG_ENC}-part2
	openssl rsautl -verify -inkey $SSLPUB -pubin -in ${PASS}${SUFX_SSL_SIG_ENC}${SUFX_SSL_SIG_ENC}-part2 | openssl aes-256-cfb -in $SRC -out ${SRC}${SUFX_SSL_PASS_ENC}${SUFX_SSL_SIG_ENC}-part1 -pass stdin
	(exit) && echo ok || echo failed
	;;

verify-dec)  # decrypt with public key; decrypt those encrypted with private key, with the way of sign;
	     #decrypt with the way you verify, but not verify
	set -o pipefail
	echo ======== decrypted with public key by ssl ======== >&2
	SUFX=$SUFX_SSL_SIG_ENC
	openssl rsautl -verify -inkey $SSLPUB -pubin -in ${SRC}${SUFX} | tee $SRC.1
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2

	echo ======== decrypted with public key by ssl '(in case file too large)' ======== >&2
	openssl rsautl -verify -inkey $SSLPUB -pubin -in ${PASS}${SUFX_SSL_SIG_ENC}${SUFX_SSL_SIG_ENC}-part2 | openssl aes-256-cfb -d -in ${SRC}${SUFX_SSL_PASS_ENC}${SUFX_SSL_SIG_ENC}-part1 -pass stdin | tee $SRC.2
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2
	set +o pipefail
	;;

enc)
	echo -------- encrypted by gpg --------
	SUFX=$SUFX_GPG_ENC
	HOME=$PUBDIR gpg -r $USERID --output ${SRC}${SUFX} --encrypt $SRC
	(exit) && echo ok || echo failed

	echo -------- encrypted by ssl --------
	SUFX=$SUFX_SSL_ENC
	openssl rsautl -encrypt -inkey $SSLPUB -pubin -in $SRC -out ${SRC}${SUFX}
	(exit) && echo ok || echo failed

	echo -------- encrypted by ssl '(in case file too large)' --------
	openssl rand -base64 32 > ${PASS}
	openssl enc -aes-256-cfb -in $SRC -out ${SRC}${SUFX_SSL_PASS_ENC}${SUFX_SSL_ENC}-part1 -pass file:${PASS}  # openssl aes-256-cfb
	openssl rsautl -encrypt -inkey $SSLPUB -pubin -in ${PASS} -out ${PASS}${SUFX_SSL_ENC}${SUFX_SSL_ENC}-part2
	rm ${PASS}
	# the above lines do same thing with below ones, except they don't need to create file $PASS explicitly, and need to
	# use someone's private key to decrypt, so must not use the below ones, because how can I get someone's private key.
	#openssl rand -base64 32 | openssl rsautl -encrypt -inkey $SSLPUB -pubin -out ${PASS}${SUFX_SSL_ENC}${SUFX_SSL_ENC}-part2
	#openssl rsautl -decrypt -inkey $SSLPRI -in ${PASS}${SUFX_SSL_ENC}${SUFX_SSL_ENC}-part2 | openssl aes-256-cfb -in $SRC -out ${SRC}${SUFX_SSL_PASS_ENC}${SUFX_SSL_ENC}-part1 -pass stdin
	(exit) && echo ok || echo failed
	;;

dec)
	echo ======== decrypted by gpg ======== >&2
	SUFX=$SUFX_GPG_ENC
	HOME=$PRIDIR gpg --decrypt ${SRC}${SUFX}
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2

	echo ======== decrypted by ssl ======== >&2
	SUFX=$SUFX_SSL_ENC
	openssl rsautl -decrypt -inkey $SSLPRI -in ${SRC}${SUFX}
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2

	echo ======== decrypted by ssl '(in case file too large)' ======== >&2
	openssl rsautl -decrypt -inkey $SSLPRI -in ${PASS}${SUFX_SSL_ENC}${SUFX_SSL_ENC}-part2 | openssl aes-256-cfb -d -in ${SRC}${SUFX_SSL_PASS_ENC}${SUFX_SSL_ENC}-part1 -pass stdin | tee $SRC.2
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2
	;;

pass-enc)
	echo -------- passphrase encrypted by gpg --------
	SUFX=$SUFX_GPG_PASS_ENC
	gpg --output ${SRC}${SUFX} --symmetric $SRC
	(exit) && echo ok || echo failed

	echo -------- passphrase encrypted by ssl --------
	SUFX=$SUFX_SSL_PASS_ENC
	openssl enc -aes-256-cfb -in $SRC -out ${SRC}${SUFX}  # openssl aes-256-cfb
	(exit) && echo ok || echo failed
	;;

pass-dec)
	echo ======== passphrase decrypted by gpg ======== >&2
	SUFX=$SUFX_GPG_PASS_ENC
	gpg --decrypt ${SRC}${SUFX}
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2

	echo ======== passphrase decrypted by ssl ======== >&2
	SUFX=$SUFX_SSL_PASS_ENC
	openssl enc -aes-256-cfb -d -in ${SRC}${SUFX}  # openssl aes-256-cfb
	(exit) && r=ok || r=failed; echo ======== $r ======== >&2
	;;

*)
	usage
	exit 1
esac
killall gpg-agent 2>/dev/null
