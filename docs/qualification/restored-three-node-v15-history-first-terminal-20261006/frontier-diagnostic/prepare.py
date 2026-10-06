from pathlib import Path
import json,hashlib,shutil
b=Path('/private/tmp');root=b/'nano-restored-three-node-v15-history-first-20261006';audit=b/'nano-v15-history-first-post-stop-native-diagnostic-20261006';out=b/'nano-v15-frontier-docker-20261006'
assert json.loads((root/'campaign/post-stop-audit-terminal-exits.json').read_text())['nativeExitCode']==0
t=json.loads((root/'campaign/terminal-exit.json').read_text());assert t['originalSession']==29884 and t['exitCode'] in [0,1]
out.mkdir(exist_ok=False)
request={'genesis':'/fixture/genesis.json','maximumSamples':'0x10','maximumSlotsPerSample':'0x80','sources':[]};before={}
for name,mount in [('mac-native','/native-data'),('mac-docker','/docker-data')]:
 data=root/name/'data';actual={str(f.relative_to(data)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(data.rglob('*')) if f.is_file()};expected=json.loads((audit/(name+'-before.json')).read_text());assert actual==expected;before[name]=actual
 c=json.loads((audit/(name+'-config.json')).read_text());c['capella']['profile']['file']='/fixture/profile.json';(out/(name+'-config.json')).write_text(json.dumps(c,indent=2)+'\n')
 request['sources'].append({'name':name,'config':'/output/'+name+'-config.json','database':mount+'/chain','checkpoint':json.loads((audit/(name+'-capture.json')).read_text())['restart']})
(out/'request.json').write_text(json.dumps(request,indent=2)+'\n');(out/'data-before.json').write_text(json.dumps(before,indent=2)+'\n')
reader=b/'nano-v13-frontier-docker-20261006/diagnose'
expected=(Path('/Users/boris/Dev/blockchain-nano/docs/qualification/v13-stopped-frontiers-20261006/programs.sha256').read_text().splitlines()[0].split()[0])
assert hashlib.sha256(reader.read_bytes()).hexdigest()==expected
shutil.copyfile(b/'nano-v13-frontier-docker-20261006/diagnose.cpp',out/'diagnose.cpp')
(out/'reader-program.json').write_text(json.dumps({'sha256':expected,'sourceProof':'docs/qualification/v13-stopped-frontiers-20261006','coreRevision':'5787915e4c62ce3c84dc46e1a9829257cdf628a1','reusedSameReaderBinary':True,'noCompilation':True},indent=2)+'\n')
(out/'run.sh').write_text("#!/bin/bash\nset -eu\ntrap 'status=$?; echo \"$status\" > /output/controller-exit.txt' EXIT\nsha256sum /reader/diagnose > /output/programs.sha256\n/reader/diagnose /output/request.json > /output/diagnostic.json 2> /output/error.log\necho 0 > /output/run-exit.txt\n")
print('Prepared stopped-store inspection using verified existing reader; no compilation or cross-host key/database transfer')
