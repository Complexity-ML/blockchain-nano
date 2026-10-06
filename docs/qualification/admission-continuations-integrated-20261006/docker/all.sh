#!/bin/bash
set -eu
arch=$1
trap 'status=$?; echo "$status" > /drivers/$arch-controller-exit.txt' EXIT
bash /drivers/build.sh "$arch"
bash /drivers/runtime.sh "$arch"
bash /drivers/regression.sh "$arch"
bash /drivers/sdk.sh "$arch"
