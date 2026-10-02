import gzip,hashlib,json,subprocess
from pathlib import Path
arm=Path('/tmp/nano-full-coherent-0f53b42d-arm64-reports-20261002')
remote='/home/ubuntu/nano-full-coherent-0f53b42d-amd64-reports-20261002'
def read_remote(name):return subprocess.check_output(['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166','cat '+remote+'/'+name],timeout=30)
reference=json.loads(gzip.decompress(Path('/Users/boris/Dev/blockchain-nano/docs/qualification/snapshot-account-view-20261002/source-manifest.json.gz').read_bytes()))
registries={};digests={}
for arch in ['arm64','amd64']:
 def read(name):return (arm/name).read_bytes() if arch=='arm64' else read_remote(name)
 assert read('build-exit.txt').strip()==b'0'
 assert read('source-revision.txt').strip()==b'0f53b42d3e78da85aa9a2c2f2993472747aabe8a'
 assert json.loads(read('source-before.json'))==reference
 raw=read('registry.json');digests[arch]=hashlib.sha256(raw).hexdigest();tests=json.loads(raw)['tests']
 assert len(tests)==len({t['name'] for t in tests})==427
 actual={}
 for t in tests:
  props={p['name']:p['value'] for p in t['properties']}
  assert props['TIMEOUT']>0
  assert not any(k in props for k in ['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION'])
  assert t['command']
  # Architecture-specific build path is the only permitted normalization.
  command=[arg.replace('full-coherent-0f53b42d-'+arch+'-20261002','full-coherent-0f53b42d-ARCH-20261002') for arg in t['command']]
  actual[t['name']]={'command':command,'timeout':props['TIMEOUT']}
 registries[arch]=actual
assert registries['arm64']==registries['amd64']
print(json.dumps({'registeredPerArchitecture':427,'sameNamesCommandsAndOriginalTimeouts':True,'skipOrDisablePropertiesPresent':False,'sourceFilesBefore':15315,'buildsCompleted':2,'registrySha256':digests,'completeResultsProven':False,'sourcesAfterTestsVerified':False,'scope':'pre-test registry and successful build evidence only'},indent=2))
