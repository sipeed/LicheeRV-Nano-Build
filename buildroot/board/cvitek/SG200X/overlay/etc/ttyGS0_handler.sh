#!/bin/sh

. /etc/profile
if [ -e /dev/ttyGS0 ]
then
	login
fi
