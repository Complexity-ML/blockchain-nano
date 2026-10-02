set -euo pipefail
build=snapshot-cache-row-2db1840c-arm64-20261002
prepare=nano-full-$build-prepare
tests=nano-full-$build-test
reports=/tmp/nano-full-candidate-3030b799-arm-reports-20261002
log=/tmp/nano-full-candidate-3030b799-arm-control-20261002/queue.log
while ! rg -q 'Built target nano-finality-crash-fixture$' "$log"; do
 test -n "$(docker ps --filter name="$prepare" --format '{{.Names}}' | grep -x "$prepare" || true)"
 sleep 5
done
rg -q 'Built target nano$' "$log"
test -z "$(docker ps --filter name="$tests" --format '{{.Names}}' | grep -x "$tests" || true)"
date -u '+%Y-%m-%dT%H:%M:%SZ' > "$reports/external-helper-before-observation-time.txt"
docker exec "$prepare" sha256sum /build/$build/nano /build/$build/nano-finality-crash-fixture /build/network/nano-altair-probe > "$reports/external-helpers-before-tests.txt"
printf '%s\n' 'Three external helpers hashed after build and before CTest starts.'
while ! test -f "$reports/ctest-exit.txt"; do
 if ! test -n "$(docker ps --filter name=nano-full-$build --format '{{.Names}}')"; then
  printf '%s\n' 'No full-run container active and no terminal CTest result.' >&2;exit 1
 fi
 sleep 15
done
docker run --rm --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 1 --memory 128m --mount type=volume,src=nano-linux-sync-build,dst=/build,readonly --entrypoint sha256sum sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 /build/$build/nano /build/$build/nano-finality-crash-fixture /build/network/nano-altair-probe > "$reports/external-helpers-after-tests.txt"
cmp "$reports/external-helpers-before-tests.txt" "$reports/external-helpers-after-tests.txt"
printf '%s\n' 0 > "$reports/external-helper-manifest-exit.txt"
