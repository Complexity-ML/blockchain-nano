#!/bin/bash
set -eu
trap 'status=$?; echo "$status" > /output/arm-controller-exit.txt' EXIT
for case in capella_catalog_relay capella_head_catalog_relay capella_depth_catchup capella_block_delivery capella_attestation_delivery capella_export_resume; do
 command="/usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -DNANO_TEST_EXPORT_CACHE -pthread -I/output/network -I/work/third_party /output/network/test_$case.cpp -o /output/arm-$case -lssl -lcrypto"
 printf '%s\n' "$command" > /output/arm-$case-command.txt
 eval "$command" > /output/arm-$case-compile.log 2>&1
 /output/arm-$case > /output/arm-$case-run.log 2>&1
 echo 0 > /output/arm-$case-exit.txt
 cat /output/arm-$case-run.log
done
