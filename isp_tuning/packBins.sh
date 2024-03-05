#!/bin/bash

BIN_FOLDER=${CHIP_ARCH,,}"/src"
BIN_TAR_FILENAME="isp_tuning_bins.tar.gz"

packFolderBins()
{
	echo "Pack isp tuning bins : "${BIN_FOLDER}
	pushd ${BIN_FOLDER}
	BINS_TAR="../../"${BIN_TAR_FILENAME}

	tar -zcf ./${BINS_TAR} *

	popd
}

packFolderBins
