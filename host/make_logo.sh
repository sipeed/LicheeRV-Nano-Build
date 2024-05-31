#!/bin/sh

usage() {
	echo "usage: $0 input output"
	echo "output image must be logo.jpeg"
	exit 1
}

src=${1}
dst=${2}
rm -rf ${dst}

if [ -z ${dst} ]
then
	usage
fi

if [ ! -e ${src} ]
then
	usage
fi


get_picture_w() {
	convert "${1}" -print "%w" /dev/null
}

get_picture_h() {
	convert "${1}" -print "%h" /dev/null
}

W=$(get_picture_w ${src})
H=$(get_picture_h ${src})

echo "picture w : ${W}"
echo "picture h : ${H}"

picturetoyuv420p() {
	ffmpeg -i ${1} -pix_fmt yuv420p ${2}
}

yuv420ptopicture() {
	ffmpeg -pix_fmt yuv420p -s ${3}x${4} -i ${1} ${2}
}

picturetoyuv420p $src $dst
yuv420ptopicture $src $dst $W $H

