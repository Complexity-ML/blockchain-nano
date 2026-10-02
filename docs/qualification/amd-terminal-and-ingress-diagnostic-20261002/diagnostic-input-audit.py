"""Audit provenance of a terminal diagnostic; never qualifies the runtime."""
import gzip
import hashlib
import json
import sys
from pathlib import Path

original, diagnostic = map(Path, sys.argv[1:3])
def hashes(path):
    result = {}
    for line in path.read_text().splitlines():
        digest, name = line.split(None, 1)
        assert len(digest) == 64
        assert name not in result
        result[name] = digest
    return result

assert (original / 'source-compare-exit.txt').read_text().strip() == '0'
links = hashes(original / 'link-inputs-after.sha256')
assert links == hashes(original / 'link-inputs-before.sha256')
programs = json.loads((original / 'programs-after.json').read_text())
assert programs == json.loads((original / 'programs-before.json').read_text())
raw_inputs = hashes(diagnostic / 'inputs-before.sha256')
assert raw_inputs == hashes(diagnostic / 'inputs-after.sha256')
build = '/build/full-coherent-0f53b42d-amd64-20261002/'
inputs = {name if name.startswith('/') else build + name: digest
          for name, digest in raw_inputs.items()}
assert len(inputs) == len(raw_inputs), 'Ambiguous input paths'
required = [build + p for p in ['lib/libmcl.a', 'libnano_core.a',
    'third_party/ckzg/libnano_blst.a', 'third_party/ckzg/libnano_ckzg.a',
    'third_party/secp256k1/lib/libsecp256k1.a']]
required += ['/build/network/nano-p2p', '/build/network/nano-altair-probe']
for name in required:
    baseline = links.get(name, programs.get(name))
    assert baseline is not None, name
    assert inputs.get(name) == baseline, name
before = json.loads(gzip.decompress((diagnostic / 'source-before.json.gz').read_bytes()))
after = json.loads(gzip.decompress((diagnostic / 'source-after.json.gz').read_bytes()))
assert before == after
baseline = json.loads((original / 'source-after.json').read_text())
assert baseline == json.loads((original / 'source-before.json').read_text())
assert set(before) == set(baseline)
changed = sorted(k for k in before if before[k] != baseline[k])
assert changed == ['tests/test_altair_p2p.cpp'], changed
assert (diagnostic / 'source-revision.txt').read_text().strip() == 'e8efb812af0e0274916cc2d4fecfc8dadf13e080'
assert (diagnostic / 'build-exit.txt').read_text().strip() == '0'
assert (diagnostic / 'input-compare-exit.txt').read_text().strip() == '0'
binary_before = hashes(diagnostic / 'diagnostic-before.sha256')
assert binary_before == hashes(diagnostic / 'diagnostic-after.sha256')
test_exit = int((diagnostic / 'test-exit.txt').read_text())
assert int((diagnostic / 'container-exit.txt').read_text()) == test_exit
print(json.dumps({'sourceFiles': len(before), 'changedSourceFiles': changed,
    'originalCampaignInputsMatched': required, 'diagnosticTestExit': test_exit,
    'additionalInputsStableOnly': sorted(set(inputs) - set(required)),
    'originalFailureResolved': False, 'productionQualified': False}, indent=2))
