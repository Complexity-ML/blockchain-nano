#!/bin/bash
set -eu
arch=$1
trap 'status=$?; echo "$status" > /drivers/$arch-controller-exit.txt' EXIT
(cd /overlay/source; sha256sum -c /drivers/source.sha256)
bash /drivers/build.sh "$arch"
bash /drivers/runtime.sh "$arch"
bash /drivers/regression.sh "$arch"
bash /drivers/sdk.sh "$arch"

(cd /overlay/source; sha256sum -c /drivers/source.sha256)
