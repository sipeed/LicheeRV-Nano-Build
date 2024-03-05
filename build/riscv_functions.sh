#!/bin/bash
#
# For cv181x_c906
#
function _build_opensbi_env()
{
  export OPENSBI_PATH
}

function clean_opensbi()
{(
  if [[ "$CHIP_ARCH" == CV181X ]] || [[ "$CHIP_ARCH" == CV180X ]]; then
    print_notice "Run ${FUNCNAME[0]}() function"
    _build_opensbi_env

    cd "$BUILD_PATH" || return "$?"
    make opensbi-clean
  fi
)}

function build_opensbi_kernel()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_kernel_env
  _build_opensbi_env

  cd "$BUILD_PATH" || return "$?"
  make riscv-cpio || return "$?"
  make kernel-setconfig SCRIPT_ARG="INITRAMFS_SOURCE="boot.cpio"" || return "$?"
  make kernel || return "$?"
  make kernel-dts || return "$?"
  make kernel-setconfig SCRIPT_ARG="INITRAMFS_SOURCE=""" || return "$?"
  make opensbi-kernel || return "$?"
)}
