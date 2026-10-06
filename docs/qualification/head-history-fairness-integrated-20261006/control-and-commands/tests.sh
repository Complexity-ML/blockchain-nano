#!/bin/bash
set -eu
trap 'status=$?; echo "$status" > /output/arm-controller-exit.txt' EXIT
command="/usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -pthread -I/output/baseline-network -I/work/third_party /output/network/test_capella_head_history_fairness.cpp -o /output/baseline -lssl -lcrypto"
printf '%s\n' "$command" > /output/baseline-command.txt
eval "$command" > /output/baseline-compile.log 2>&1
set +e
/output/baseline > /output/baseline-run.log 2>&1
status=$?
set -e
echo "$status" > /output/baseline-exit.txt
[ "$status" = 1 ]
grep -F 'HISTORY STARVATION: unconfirmed head consumed both historical opportunities' /output/baseline-run.log
for case in capella_head_catalog_relay capella_head_history_fairness capella_relay_schedule; do
 command="/usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -DNANO_HEAD_HISTORY_FIRST -pthread -I/output/network -I/work/third_party /output/network/test_$case.cpp -o /output/arm-$case -lssl -lcrypto"
 printf '%s\n' "$command" > /output/arm-$case-command.txt
 eval "$command" > /output/arm-$case-compile.log 2>&1
 /output/arm-$case > /output/arm-$case-run.log 2>&1
 echo 0 > /output/arm-$case-exit.txt
 cat /output/arm-$case-run.log
done
