#!/bin/sh

DEBUG=0

if [ "$#" != "2" ]; then
	echo "usage: wget host url"
	exit -1
fi

URL="$2"
HOST="$1"

SOCK=sock-$$
DATA=sock-data-$$

PORT=80

binder assign /tmp/$SOCK /service/net_endpoint/Connect "$HOST" $PORT > /dev/null
test "$DEBUG" = "1" && printf "** opening connection: $HOST:$PORT\n"
test "$DEBUG" = "1" && printf "** socket: $(binder get /tmp/$SOCK)\n"

if [ "$(binder get /tmp/$SOCK)" = "<undefined>" ]; then
	echo "error: could not open socket"
	exit -1
fi

test "$DEBUG" = "1" && printf "** sending get request\n"
binder get /tmp/$SOCK/SendLine "GET $URL HTTP/1.0" > /dev/null
binder get /tmp/$SOCK/SendLine "" > /dev/null
done=0
while [ "$done" = "0" ]; do
	binder assign /tmp/$DATA /tmp/$SOCK/Receive 2048 > /dev/null
	
	result="$(binder get /tmp/$DATA/result)"

	if [ "$result" = "ok" ]; then
		printf "$(binder get /tmp/$DATA/data)"
	else
		test "$DEBUG" = "1" && printf "\n** read error: $result\n"
		done=1
	fi

	#if [ "$(binder get /tmp/$SOCK/IsOpen)" = "false" ]; then
	#	done=1
	#fi
	binder put /tmp/$DATA undefined undefined
done

test "$DEBUG" = "1" && printf "** closing connection\n"
binder get /tmp/$SOCK/Close > /dev/null
binder put /tmp/$SOCK undefined undefined

printf "\n"
