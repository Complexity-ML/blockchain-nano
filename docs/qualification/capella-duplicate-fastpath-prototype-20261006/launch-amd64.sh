#!/bin/bash
set -eu
runtime=/tmp/nano-cold-runtime-20261006
prototype=/tmp/nano-duplicate-fastpath-20261006
for mode in cold-automatic cold-bls; do
 test "$(cat "$runtime/amd64-v3/test-$mode-exit.txt")" = 0
done
test "$(cat "$runtime/amd64-warm-repair/controller-exit.txt")" = 0
test "$(cat "$runtime/amd64-warm-repair/test-warm-pruning-exit.txt")" = 0
test ! -e "$prototype/amd64"
mkdir "$prototype/amd64"
sudo docker run --rm --network none --ulimit nofile=65536:65536 \
 -v "$prototype/source-v2:/source:ro" -v "$prototype/source-v2:/work:ro" \
 -v nano-clean-rpc-build-20261002:/build \
 -v "$prototype/build.sh:/driver:ro" -v "$prototype/amd64:/probe" \
 --entrypoint /bin/bash sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14 /driver amd64
