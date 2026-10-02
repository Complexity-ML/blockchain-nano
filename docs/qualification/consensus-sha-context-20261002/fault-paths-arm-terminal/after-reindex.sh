set -euo pipefail
reports=/home/ubuntu/nano-sha-context-fault-reports-20261002
source_dir=/home/ubuntu/nano-consensus-sha-context-candidate-20261002
control=/home/ubuntu/nano-sha-context-fault-control-20261002
while sudo docker inspect nano-sha-context-f7e46437-amd-reindex >/dev/null 2>&1;do sleep 10;done
while ps -p 571030 -o args= | grep -F '/nano-sha-context-control-20261002/reindex-host.sh' >/dev/null;do sleep 5;done
test -f /home/ubuntu/nano-sha-context-reindex-reports-20261002/container-exit.txt
test "$(git -C "$source_dir" rev-parse --short=8 HEAD)" = f7e46437
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$reports";chmod 0777 "$reports"
set +e
sudo docker run --rm --name nano-sha-context-amd-faults --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 2g --tmpfs /tmp:rw,nosuid,size=512m -v "$source_dir:/work:ro" -v "$control:/control:ro" -v nano-linux-sync-build:/scratch sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c sh /control/faults.sh > "$reports/terminal.log" 2>&1
result=$?
set -e
printf '%s\n' "$result" > "$reports/container-exit.txt"
exit "$result"
