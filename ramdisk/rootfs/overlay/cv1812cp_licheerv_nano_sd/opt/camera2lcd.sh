#!/bin/sh

THISDIR=$(dirname $(realpath $0))

${THISDIR}/camera.sh 1

exec sample_vio 6
