#!/bin/bash
set -eu
trap 'status=$?; echo "$status" > /output/amd-controller-exit.txt' EXIT
for case in capella_catalog_relay capella_head_catalog_relay capella_depth_catchup capella_block_delivery capella_attestation_delivery capella_export_resume; do
 command="/usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -DNANO_TEST_EXPORT_CACHE -pthread -I/output/network -I/work/third_party /output/network/test_$case.cpp -o /output/amd-$case -lssl -lcrypto"
 printf '%s\n' "$command" > /output/amd-$case-command.txt
 eval "$command" > /output/amd-$case-compile.log 2>&1
 /output/amd-$case > /output/amd-$case-run.log 2>&1
 echo 0 > /output/amd-$case-exit.txt
 cat /output/amd-$case-run.log
done
