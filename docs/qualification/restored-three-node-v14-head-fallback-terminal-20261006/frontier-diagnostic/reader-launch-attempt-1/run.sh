#!/bin/bash
set -eu
trap 'status=$?; echo "$status" > /output/controller-exit.txt' EXIT
sha256sum /reader/diagnose > /output/programs.sha256
/reader/diagnose /output/request.json > /output/diagnostic.json 2> /output/error.log
echo 0 > /output/run-exit.txt
