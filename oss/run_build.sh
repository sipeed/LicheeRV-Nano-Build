#!/bin/bash
set -e

###############################################################################
# Usage:
#   - when -i is present, build and install into $install_dir
#   - when -t is present, tar the intall dir into $tar_dir
#   - when -e is present, extract from $tar_dir, and install to $install_dir
###############################################################################
usage()
{
   echo ""
   echo "Usage: $0 -n name [-e] [-t tar_dir] [-i install_dir] [-s 64bit|32bit]"
   echo -e "\t-n project name to build"
   echo -e "\t-s arch version, 64bit or 32bit, default 64bit"
   echo -e "\t-t dir store/load tar file to/from"
   echo -e "\t-r sysroot dir"
   echo -e "\t-i dir to install"
   exit 1
}

extract=false
sdk_arch=64bit
while getopts "n:t:i:s:r:eh" opt
do
  case "$opt" in
    n ) name="$OPTARG" ;;
    t ) tar_dir="$OPTARG" ;;
    i ) install_dir="$OPTARG" ;;
    s ) sdk_arch="$OPTARG" ;;
    r ) sysroot_dir="$OPTARG" ;;
    e ) extract=true ;;
    h ) usage ;;
  esac
done

#
# check project name
#
if [ -z $name ] ; then
  echo "name not present"
  exit 1
fi

#
# if extract, extract and then finish
#
if [ "$extract" = true ] ; then
  echo 'Extract'
  if [ -z $install_dir ] || [ -z $tar_dir ] ; then
    echo "$install_dir or $tar_dir not present"
    exit 1
  fi
  mkdir -p ${install_dir}
  tar -xhzf ${tar_dir}/${name}.tar.gz -C ${install_dir}
  exit 0
fi

if [ "$sdk_arch" == "glibc_riscv64" ]; then
  CFLAGS="${CFLAGS} -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d"
elif [ "$sdk_arch" == "musl_riscv64" ]; then
  CFLAGS="${CFLAGS} -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d"
fi

#
# clean and create build dir and temp install_dir if needed
#
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

rm -rf $SCRIPT_DIR/build/${name}
mkdir -p $SCRIPT_DIR/build/${name}
BUILD_DIR=$SCRIPT_DIR/build/${name}

INSTALL_DIR=$install_dir
if [ -z $install_dir ] ; then
  rm -rf $SCRIPT_DIR/install/${name}
  mkdir -p $SCRIPT_DIR/install/${name}
  INSTALL_DIR=$SCRIPT_DIR/install/${name}
fi

# build
BUILD_DIR=$BUILD_DIR \
INSTALL_DIR=$INSTALL_DIR \
SYSROOT_PATH=$sysroot_dir \
CFLAGS=${CFLAGS} \
SDK_ARCH=$sdk_arch \
    $SCRIPT_DIR/build_${name}.sh

if [ "$name" = "opencv4.5" ] ; then
  if [ -d $INSTALL_DIR/include/opencv4 ]; then
    pushd $INSTALL_DIR
    if [ -d include/opencv2 ]; then
      rm -rf include/opencv2
    fi
    mv include/opencv4/opencv2/ include/
    rmdir include/opencv4
    popd
  fi
fi

# tar install dir if $tar_dir is present
if [ ! -z "$tar_dir" ] ; then
  if [ ! -e $tar_dir ] ; then
    echo "$tar_dir not exist"
    exit 1
  fi
  pushd $INSTALL_DIR
  tar -czf $tar_dir/${name}.tar.gz *
  popd
fi
