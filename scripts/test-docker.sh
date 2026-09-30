#!/bin/sh
# Bounded smoke test; isolated container and volume, no published ports.
set -eu
image=${1:-blockchain-nano:local}
name="nano-smoke-$$"
volume="$name-data"
cleanup() {
    docker rm -f "$name" >/dev/null 2>&1 || true
    docker volume rm "$volume" >/dev/null 2>&1 || true
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
docker volume create "$volume" >/dev/null
docker run -d --name "$name" --init --network none --read-only \
    --cap-drop ALL --security-opt no-new-privileges:true \
    --ulimit "nofile=${NANO_DOCKER_NOFILE:-65536}:${NANO_DOCKER_NOFILE:-65536}" \
    --tmpfs /tmp:rw,size=64m,mode=1777 \
    -v "$volume:/var/lib/nano" "$image" >/dev/null
ready() {
    attempt=0
    while [ "$attempt" -lt 30 ]; do
        if docker exec "$name" curl --fail --silent --max-time 2 \
            -H 'Content-Type: application/json' \
            --data '{"jsonrpc":"2.0","id":1,"method":"eth_chainId","params":[]}' \
            http://127.0.0.1:8545 | grep -q '"result"'; then
            return 0
        fi
        attempt=$((attempt + 1))
        sleep 1
    done
    docker logs "$name"
    return 1
}
ready
docker top "$name" | grep '[n]ano-p2p' >/dev/null
[ "$(docker exec "$name" id -u)" = 10001 ]
before=$(docker exec "$name" sha256sum /var/lib/nano/peer.key)
docker stop --time 60 "$name" >/dev/null
[ "$(docker inspect --format '{{.State.ExitCode}}' "$name")" = 0 ]
docker start "$name" >/dev/null
ready
after=$(docker exec "$name" sha256sum /var/lib/nano/peer.key)
[ "$before" = "$after" ]
docker top "$name" | grep '[n]ano-p2p' >/dev/null
printf '%s\n' 'PASS: RPC, non-root, P2P child, graceful stop and persistent identity'
