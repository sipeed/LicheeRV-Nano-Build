#!/bin/sh

hub_on() {
	echo "nop"
}

hub_off() {
	echo "nop"
}

inst_mod() {
	echo "nop"
}

case "$1" in
  host)
	hub_on
	;;
  device)
	hub_off
	inst_mod
	echo device > /proc/cviusb/otg_role
	;;
  *)
	echo "Usage: $0 host"
	echo "Usage: $0 device"
	exit 1
esac
exit $?
