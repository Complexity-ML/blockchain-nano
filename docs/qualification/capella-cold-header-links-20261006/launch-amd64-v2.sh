#!/bin/bash
set -eu
root=/tmp/nano-cold-header-links-20261006
qualified=/tmp/nano-duplicate-fastpath-20261006/amd64
test "$(cat "$qualified/build-controller-exit.txt")" = 0
test "$(cat "$qualified/test-duplicate-exit.txt")" = 0
test ! -e "$root/amd64-v2"
mkdir "$root/amd64-v2"
sudo docker run --rm --network none --ulimit nofile=65536:65536 \
 -v "$root/source-v2:/source:ro" -v "$root/source-v2:/work:ro" \
 -v nano-clean-rpc-build-20261002:/build -v "$root/build-v2.sh:/driver:ro" \
 -v "$root/amd64-v2:/probe" \
 --entrypoint /bin/bash sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14 /driver amd64
