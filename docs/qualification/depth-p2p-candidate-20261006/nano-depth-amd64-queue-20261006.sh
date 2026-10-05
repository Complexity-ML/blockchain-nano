#!/bin/bash
set -eu
echo 'Waiting for the original v6 VPS container to stop before compilation'
for ((i=0;i<100;i++));do
 state=$(sudo -n docker inspect nano-restored-three-node-vps-muvt00jb --format '{{.State.Running}} {{.State.ExitCode}} {{.State.OOMKilled}}')
 if [ "$state" = 'false 0 false' ];then break;fi
 if [ "${state%% *}" != true ];then echo "Unexpected terminal state: $state";exit 1;fi
 sleep 5
done
[ "$state" = 'false 0 false' ]
[ "$(git -C /home/ubuntu/blockchain-nano-git rev-parse HEAD)" = 81e5ab3105fd21267e864dd11cca694faf0ad0d0 ]
sudo -n docker run --name nano-depth-catchup-amd64-81e5ab31-20261006 --network none --read-only --tmpfs /tmp --memory 4g --cpus 2 --cap-drop ALL --security-opt no-new-privileges -v nano-clean-rpc-build-20261002:/build -v /home/ubuntu/blockchain-nano-git:/work:ro -v /tmp/nano-depth-p2p-build-control-20261006.sh:/control.sh:ro --entrypoint bash sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14 /control.sh /build/coherent-efbeb9bc-amd64/network /build/rejection-scan-8adbabff-amd64-20261005 /build/depth-catchup-amd64-81e5ab31-20261006 amd64
