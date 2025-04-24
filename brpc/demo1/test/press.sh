#!/bin/bash

if ! type rpc_press  > /dev/null; then
    echo "Install rpc_press first please, exiting..."
    exit 1
fi

TESTDIR=$(dirname $(readlink -f ${BASH_SOURCE[0]}))
PROJDIR=$(cd "$TESTDIR/.." && pwd)

cd "$PROJDIR"
# start server
ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ]; then
    ARCH=arm64
fi
./build/linux/$ARCH/release/echo_server > /dev/null 2>&1 &

# start rpc_press
rpc_press  \
    -duration=180 \
    -server 127.0.0.1:18000 \
    -proto ./src/echo.proto \
    -method=demo1.EchoService.Echo \
    -input '{"message":"hello"} {"message":"world"}' \
    -qps -1 \
    -thread_num $(nproc)

pkill echo_server
wait
