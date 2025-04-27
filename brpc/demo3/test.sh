#!/bin/bash

set -ex

cd $(dirname $(readlink -f ${BASH_SOURCE[0]}))

ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ]; then
    ARCH=arm64
fi

test  -x ./build/linux/$ARCH/release/echo_server || xmake

./build/linux/$ARCH/release/echo_server &> server.log &
trap "kill $!" EXIT

timeout 60 ./build/linux/$ARCH/release/echo_client
