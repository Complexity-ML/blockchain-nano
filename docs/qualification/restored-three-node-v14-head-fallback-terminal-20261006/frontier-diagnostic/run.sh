#!/bin/bash
set -eu
export LD_LIBRARY_PATH=/build/coherent-efbeb9bc-arm64:/build/coherent-efbeb9bc-arm64/third_party/ckzg:/build/coherent-efbeb9bc-arm64/_deps/blst-build:${LD_LIBRARY_PATH:-}
trap 'status=$?; echo "$status" > /output/controller-exit.txt' EXIT
sha256sum /reader/diagnose > /output/programs.sha256
/reader/diagnose /output/request.json > /output/diagnostic.json 2> /output/error.log
echo 0 > /output/run-exit.txt
