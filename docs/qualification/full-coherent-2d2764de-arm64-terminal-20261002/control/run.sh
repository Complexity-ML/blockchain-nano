set -euo pipefail
source_dir=/tmp/nano-production-hex-candidate-20261002
selected=/tmp/nano-coherent-2d2764de-arm64-test-reports-20261002
for file in ctest-exit.txt container-exit.txt input-compare-exit.txt; do test "$(cat "$selected/$file")" = 0; done
python3 - "$selected" <<'VERIFY'
import sys,json
from pathlib import Path
p=Path(sys.argv[1]);assert json.loads((p/'result-audit.json').read_text())=={'cases':15,'skipped':0,'failed':0,'timeoutsPreserved':True}
for a,b in [('source-manifest-before.json','source-manifest-after.json'),('programs-before.sha256','programs-after.sha256'),('inputs-before.sha256','inputs-after.sha256')]:assert (p/a).read_bytes()==(p/b).read_bytes()
VERIFY
cd "$source_dir"
test "$(git rev-parse --short=8 HEAD)" = 2d2764de
test -z "$(git status --porcelain)"
export NANO_BUILDER_IMAGE=sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4
export NANO_BUILD_VOLUME=nano-linux-sync-build
export NANO_BUILD_SUBDIR=coherent-2d2764de-arm64-20261002
bash scripts/ci/full-docker.sh /tmp/nano-full-2d2764de-arm64-reports-20261002
