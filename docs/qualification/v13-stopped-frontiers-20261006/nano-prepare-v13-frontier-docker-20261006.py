from pathlib import Path
import json,hashlib,shutil
b=Path('/private/tmp');root=b/'nano-restored-three-node-v13-parent-cache-20261006';audit=b/'nano-v13-parent-cache-post-stop-native-diagnostic-20261006';out=b/'nano-v13-frontier-docker-20261006';out.mkdir(exist_ok=False)
assert json.loads((root/'campaign/terminal-exit.json').read_text())==json.loads((root/'campaign/terminal-exit.json').read_text())
t=json.loads((root/'campaign/terminal-exit.json').read_text());assert t['originalSession']==80316 and t['exitCode']==1
shutil.copyfile(b/'nano-v12-block-frontier-diagnostic-20261006/diagnose.cpp',out/'diagnose.cpp')
request={'genesis':'/fixture/genesis.json','maximumSamples':'0x10','maximumSlotsPerSample':'0x80','sources':[]};before={}
for name,mount in [('mac-native','/native-data'),('mac-docker','/docker-data')]:
 data=root/name/'data';actual={str(f.relative_to(data)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(data.rglob('*')) if f.is_file()};expected=json.loads((audit/(name+'-before.json')).read_text());assert actual==expected;before[name]=actual
 c=json.loads((audit/(name+'-config.json')).read_text());c['capella']['profile']['file']='/fixture/profile.json';(out/(name+'-config.json')).write_text(json.dumps(c,indent=2)+'\n')
 request['sources'].append({'name':name,'config':'/output/'+name+'-config.json','database':mount+'/chain','checkpoint':json.loads((audit/(name+'-capture.json')).read_text())['restart']})
(out/'request.json').write_text(json.dumps(request,indent=2)+'\n');(out/'data-before.json').write_text(json.dumps(before,indent=2)+'\n')
(out/'build-run.sh').write_text('''#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64
core=/build/parent-cache-arm64-20261006/libnano_core.a
trap 'status=$?; echo "$status" > /output/controller-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\\(DEFINES\\|INCLUDES\\|FLAGS\\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\\n' ' ')
command="/usr/bin/clang++ -I/work/include -I/work/third_party/ckzg/blst/bindings $flags -c /output/diagnose.cpp -o /output/diagnose.o"
printf '%s\\n' "$command" > /output/compile-command.txt
eval "$command" > /output/compile.log 2>&1
echo 0 > /output/compile-exit.txt
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\\/nano-capella-transition-tests.dir\\/tests\\/test_capella_transition.cpp.o/\\/output\\/diagnose.o}
command=${command//-o nano-capella-transition-tests /-o \\/output\\/diagnose }
command=${command// libnano_core.a / $core }
printf '%s\\n' "$command" > /output/link-command.txt
(cd "$base";eval "$command") > /output/link.log 2>&1
echo 0 > /output/link-exit.txt
sha256sum /output/diagnose "$core" > /output/programs.sha256
/output/diagnose /output/request.json > /output/diagnostic.json 2> /output/error.log
echo 0 > /output/run-exit.txt
''')
print('Prepared Docker reader against candidate ABI; originals read-only, no keys transferred')
