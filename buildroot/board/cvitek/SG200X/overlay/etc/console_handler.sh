#!/bin/sh

. /etc/profile
if [ -e /dev/console ]
then
	login
fi
