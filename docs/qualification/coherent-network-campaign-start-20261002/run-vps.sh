#!/bin/bash
set -eu
cd /home/ubuntu/nano-coherent-network-build-candidate-20261002
export PATH=/home/ubuntu/nano-coherent-network-build-control-20261002:$PATH
export NANO_BUILDER_IMAGE=sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c
export NANO_BUILD_VOLUME=nano-linux-sync-build
export NANO_BUILD_SUBDIR=full-network-9e9ebed3-amd64-20261002
set +e
bash scripts/ci/full-docker.sh /home/ubuntu/nano-coherent-network-amd64-reports-20261002
code=$?
printf '%s\n' "$code" > /home/ubuntu/nano-coherent-network-build-control-20261002/campaign-exit.txt
exit "$code"
