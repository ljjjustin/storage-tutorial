#!/bin/bash

set -ex

cd $(dirname $(readlink -f ${BASH_SOURCE[0]}))

ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ]; then
    ARCH=arm64
fi

test  -x ./build/linux/$ARCH/release/http_server || xmake

./build/linux/$ARCH/release/http_server &> /dev/null &
trap "kill $!" EXIT

for i in $(seq 1 10)
do
    data="req-$i"
    ./build/linux/$ARCH/release/http_client http://127.0.0.1:18000/HttpService/Echo -d "$data"
done
