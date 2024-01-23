#!/bin/sh

while true
do
	# ntp will borken test
	if [ -e `which ntpdate` ]
	then
		mv `which ntpdate` /ntpdate
	fi
	killall udhcpc
	killall wpa_supplicant
	ifconfig wlan0 down
	ifconfig eth0 up
	udhcpc -i eth0 -b -T 1
	# retry 3
	count=3
	while [ $count -gt 0 ] 
	do
		sleep 1
		start_time=$(date +%s)
		wget -O /dev/null http://192.168.0.104:8000/rand10MiB
		if [ $? -ne 0 ]
		then
			count=$((count - 1))
			continue;
		fi
		end_time=$(date +%s)
		a=$((end_time - start_time))
		echo "eth rx 10MiB test (s): $a" | tee -a /tmp/ramdisk/vendor_test.log
		if [ $a -ge 3 ]
		then
			echo "eth rx test failed" | tee -a /tmp/ramdisk/vendor_test.log
		else
			echo "eth rx test ok"
		fi
		break;
	done


	killall crond
	killall wpa_supplicant
	killall udhcpc
	touch /etc/wpa_supplicant.conf
	cat /etc/wpa_supplicant.conf | grep Sipeed_Guest
	if [ $? -ne 0 ]
	then
		echo '
		network={
			ssid="Sipeed_Guest"
			psk=280517f8e2cfd7a605a767d8d9a102f36ac356e68fc777d51f963d20c8817909
		}
		' > /etc/wpa_supplicant.conf
	fi
	ifconfig eth0 down
	ifconfig wlan0 up
	wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf -B
	udhcpc -i wlan0 -b -T 1
	# retry 5
	count=5
	while [ $count -gt 0 ] 
	do
		if [ ! -e /sys/class/net/wlan0 ]
		then
			echo "no wifi hardware"
			break;
		fi
		sleep 1
		start_time=$(date +%s)
		wget -O /dev/null http://192.168.0.104:8000/rand10MiB
		if [ $? -ne 0 ]
		then
			count=$((count - 1))
			continue;
		fi
		end_time=$(date +%s)
		a=$((end_time - start_time))
		echo "wifi rx 10MiB test (s): $a" | tee -a /tmp/ramdisk/vendor_test.log
		if [ $a -ge 10 ]
		then
			echo "wifi rx test failed" | tee -a /tmp/ramdisk/vendor_test.log
		else
			echo "wifi rx test ok" | tee -a /tmp/ramdisk/vendor_test.log
		fi
		break;
	done

done
