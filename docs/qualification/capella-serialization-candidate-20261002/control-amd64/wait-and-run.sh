set -euo pipefail
parent=nano-capella-sha-provider-amd64-test
reports=/home/ubuntu/nano-capella-sha-provider-control-20261002-reports
printf '%s\n' 'Waiting for the confirmed active parent container; no build mutation until terminal manifests.'
while test -n "$(sudo docker ps --filter name="$parent" --format '{{.Names}}' | grep -x "$parent" || true)"; do sleep 15; done
for attempt in $(seq 1 12); do
 if test -f "$reports/container-exit.txt" && test -f "$reports/source-manifest-after-tests.json"; then break; fi
 sleep 5
done
test -f "$reports/container-exit.txt"
bash /home/ubuntu/nano-capella-serialization-amd64-control-20261002/host-driver.sh
