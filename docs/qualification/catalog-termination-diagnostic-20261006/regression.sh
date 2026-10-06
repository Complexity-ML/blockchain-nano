#!/bin/bash
set -eu
trap 'status=$?; echo "$status" > /output/regression-controller-exit.txt' EXIT
cp /repo/network/test_capella_catalog_relay.cpp /output/regression.cpp
/usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -pthread -I/output -I/repo/network -I/repo/third_party /output/regression.cpp -o /output/regression -lssl -lcrypto > /output/regression-compile.log 2>&1
/output/regression > /output/regression-run.log 2>&1
cat /output/regression-run.log
