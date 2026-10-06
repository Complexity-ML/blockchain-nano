#!/bin/bash
set -eu
trap 'status=$?; echo "$status" > /output/controller-exit.txt' EXIT
for mode in baseline draft; do
 if [ "$mode" = draft ]; then include=/output; else include=/repo/network; fi
 /usr/bin/clang++ -std=c++20 -O1 -DBOOST_ERROR_CODE_HEADER_ONLY -pthread -I"$include" -I/repo/network -I/repo/third_party /output/counterexample.cpp -o "/output/$mode" -lssl -lcrypto > "/output/$mode-compile.log" 2>&1
 set +e
 "/output/$mode" > "/output/$mode-run.log" 2>&1
 status=$?
 set -e
 echo "$status" > "/output/$mode-exit.txt"
 if [ "$mode" = baseline ]; then [ "$status" = 1 ]; grep -F 'MASKED TERMINATION' /output/baseline-run.log; else [ "$status" = 0 ]; cat /output/draft-run.log; fi
 sha256sum "/output/$mode" > "/output/$mode-binary.sha256"
done
