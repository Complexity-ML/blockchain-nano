#!/bin/sh
# Export local build artifacts only; never mount chain data or validator keys.
set -eu
if [ "$#" -ne 6 ]; then
  echo 'Usage: package-candidate.sh immutable-runtime-base immutable-build-image build-volume compiled-revision nano-sha256 p2p-sha256' >&2
  exit 1
fi
base=$1
builder=$2
volume=$3
revision=$4
nano_hash=$5
p2p_hash=$6
case "$base:$builder" in *[!a-z0-9:]* ) echo 'Invalid image IDs' >&2; exit 1;; esac
for id in "$base" "$builder"; do
  printf '%s\n' "$id" | grep -Eq '^sha256:[0-9a-f]{64}$' || exit 1
done
printf '%s\n' "$volume" | grep -Eq '^[a-zA-Z0-9][a-zA-Z0-9_.-]*$' || exit 1
printf '%s\n' "$revision" | grep -Eq '^[0-9a-f]{40}$' || exit 1
for digest in "$nano_hash" "$p2p_hash"; do
  printf '%s\n' "$digest" | grep -Eq '^[0-9a-f]{64}$' || exit 1
done
# Inspect first: Docker must not create an empty volume or pull an unknown image.
docker volume inspect "$volume" >/dev/null
base_arch=$(docker image inspect "$base" --format '{{.Architecture}}')
build_arch=$(docker image inspect "$builder" --format '{{.Architecture}}')
[ "$base_arch" = "$build_arch" ] || { echo 'Architecture mismatch' >&2; exit 1; }
context=$(mktemp -d)
trap 'rm -rf "$context"' EXIT HUP INT TERM
recipe=$(CDPATH= cd -- "$(dirname -- "$0")/../../deploy/docker" && pwd)/candidate.Dockerfile
# The named volume is read-only; export exactly the two executables and provenance.
docker run --rm --network none --read-only --cap-drop ALL --security-opt no-new-privileges \
  --entrypoint tar -v "$volume:/build:ro" "$builder" \
  -C /build -cf - nano network/nano-p2p production-source-revision.txt > "$context/artifacts.tar"
tar -xf "$context/artifacts.tar" -C "$context"
[ "$(cat "$context/production-source-revision.txt")" = "$revision" ] || { echo 'Build revision mismatch' >&2; exit 1; }
mv "$context/network/nano-p2p" "$context/nano-p2p"
rm -rf "$context/network" "$context/artifacts.tar" "$context/production-source-revision.txt"
# Runtime base is immutable and local. No source compilation or apt step occurs.
docker build --network none --pull=false --progress plain --iidfile "$context/image-id" \
  --build-arg "NANO_RUNTIME_BASE=$base" --build-arg "SOURCE_REVISION=$revision" \
  --build-arg "NANO_SHA256=$nano_hash" --build-arg "P2P_SHA256=$p2p_hash" \
  -f "$recipe" "$context"
cat "$context/image-id"
