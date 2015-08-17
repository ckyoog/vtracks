#!/bin/sh

if [ $# -lt 3 ]; then
	echo "$0 <username> <password> <proxyport> [<aliveinternal> [<alivecountmax>]]"
	echo aliveinternal and alivecountmax is 15 and 3 by default
	exit 1
fi

# Configuration
USERNAME="$1"
PASSWORD="$2"
PROXYPORT="$3"
ALIVEINTERNAL=${4:-15}
ALIVECOUNTMAX=${5:-3}

# Prepare sshpass
MYSSH_PATH=/tmp/.myssh
echo 'sshpass -p "'"$PASSWORD"'" ssh "$@"' > $MYSSH_PATH
chmod +x $MYSSH_PATH

SSH_OPTS="-N -D $PROXYPORT -o ExitOnForwardFailure=yes -o ServerAliveInterval=$ALIVEINTERNAL -o ServerAliveCountMax=$ALIVECOUNTMAX"
SSH_ADDRESS=ssh.sshcenter.info

# Final command
AUTOSSH_LOGFILE=/tmp/autossh.log AUTOSSH_PATH=$MYSSH_PATH autossh -M 0 $SSH_OPTS $USERNAME@$SSH_ADDRESS

