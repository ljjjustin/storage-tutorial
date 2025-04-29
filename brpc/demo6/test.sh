#!/bin/bash

set -ex

cd $(dirname $(readlink -f ${BASH_SOURCE[0]}))

ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ]; then
    ARCH=arm64
fi

test  -x ./build/linux/$ARCH/release/stream_server || xmake

./build/linux/$ARCH/release/stream_server &
trap "kill $!" EXIT

timeout 5 ./build/linux/$ARCH/release/stream_client
