#!/bin/bash
set -eu
for case in capella_head_history_fairness; do
 command="/usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -DNANO_HEAD_HISTORY_FIRST -pthread -I/output/network -I/work/third_party /output/network/test_$case.cpp -o /output/arm-final-$case -lssl -lcrypto"
 printf '%s\n' "$command" > /output/arm-final-$case-command.txt
 eval "$command" > /output/arm-final-$case-compile.log 2>&1
 /output/arm-final-$case > /output/arm-final-$case-run.log 2>&1
 echo 0 > /output/arm-final-$case-exit.txt
 cat /output/arm-final-$case-run.log
done
