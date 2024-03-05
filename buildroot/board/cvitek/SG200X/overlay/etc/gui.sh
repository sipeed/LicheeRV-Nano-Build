#!/bin/sh

lock=$(ps | grep gui.sh | grep -v 'grep' | wc -l)
if [ $lock -gt 2 ]
then
	exit 1
fi
started=$(ps | grep fingerpaint | grep -v 'grep' | wc -l)

if [ "${started}" -eq 0 ]
then
	for i in /etc/init.d/S99*test
	do
		# stop other program use framebuffer
		$i stop
	done
	(/usr/lib/qt/examples/widgets/touch/fingerpaint/fingerpaint) &
else
	killall fingerpaint
	# clean framebuffer
	cat /dev/zero > /dev/fb0
	for i in /etc/init.d/S99*test
	do
		# restore
		$i start
	done
fi
