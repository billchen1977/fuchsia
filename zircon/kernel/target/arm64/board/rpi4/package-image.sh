#!/usr/bin/env bash

# Copyright 2021 The Fuchsia Authors
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT

set -eo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ZIRCON_DIR="${DIR}/../../../../.."
PROJECT_ROOT="${ZIRCON_DIR}/.."

# arguments
BOARD=rpi4
ROOT_BUILD_DIR=
BOOT_IMG=
ZIRCON_BOOTIMAGE=

function HELP {
    echo "help:"
    echo "-B <build-dir>  : path to Fuchsia build directory"
    echo "-o              : output boot img file (defaults to <build-dir>/kernel8.img)"
    echo "-z              : input zircon ZBI file (defaults to <build-dir>/arm64.img)"
    exit 1
}

while getopts "B:o:z:" FLAG; do
    case $FLAG in
        B) ROOT_BUILD_DIR="${OPTARG}";;
        o) BOOT_IMG="${OPTARG}";;
        z) ZIRCON_BOOTIMAGE="${OPTARG}";;
        \?)
            echo unrecognized option
            HELP
            ;;
    esac
done

if [[ -z "${ROOT_BUILD_DIR}" ]]; then
    echo must specify a Zircon build directory
    HELP
fi

# zircon image built by the Zircon build system
if [[ -z "${ZIRCON_BOOTIMAGE}" ]]; then
    ZIRCON_BOOTIMAGE="${ROOT_BUILD_DIR}/arm64.zbi"
fi

# Final packaged boot image
if [[ -z "${BOOT_IMG}" ]]; then
    BOOT_IMG="${ROOT_BUILD_DIR}/kernel8.img"
fi

# boot shim for our board
BOOT_SHIM="${ROOT_BUILD_DIR}/${BOARD}-boot-shim.bin"

# shim and compress the ZBI
cat "${BOOT_SHIM}" "${ZIRCON_BOOTIMAGE}" > "${BOOT_IMG}"

