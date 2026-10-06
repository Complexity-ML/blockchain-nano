#!/bin/bash
set -eu
root=/tmp/nano-cold-runtime-20261006
for mode in cold-automatic cold-bls; do
 test "$(cat "$root/amd64-v3/test-$mode-exit.txt")" = 0
done
test "$(cat "$root/amd64-v3/build-controller-exit.txt")" = 1
test "$(cat "$root/amd64-v3/test-warm-pruning-exit.txt")" = 1
grep -Fq 'attempting to parse an empty input' "$root/amd64-v3/test-warm-pruning.log"
test ! -e "$root/amd64-warm-repair"
mkdir "$root/amd64-warm-repair"
sudo docker run --rm --network none --ulimit nofile=65536:65536 \
 -v "$root/source-v4:/source:ro" -v "$root/source-v4:/work:ro" \
 -v nano-clean-rpc-build-20261002:/build -v "$root/warm-repair.sh:/driver:ro" \
 -v "$root/amd64-v3:/qualified:ro" -v "$root/amd64-warm-repair:/probe" \
 --entrypoint /bin/bash sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14 /driver amd64
