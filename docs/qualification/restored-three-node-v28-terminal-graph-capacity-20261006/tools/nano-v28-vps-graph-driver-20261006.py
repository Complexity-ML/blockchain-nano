from pathlib import Path
import json, subprocess, hashlib
root=Path('/tmp/nano-restored-three-node-vps-muwtdigu');out=Path('/tmp/nano-v28-readonly-graph-metrics-20261006');audit=root/'local-readonly-audit'
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';build='nano-clean-rpc-build-20261002';volume='nano-restored-three-node-vps-20261005-data'
def run(cmd):return subprocess.check_output(cmd,text=True)
def save(name,data): (out/name).write_text(json.dumps(data,indent=2)+'\n')
state=json.loads(run(['docker','inspect','nano-restored-three-node-vps-muwtdigu','--format','{{json .State}}']));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled'];assert not run(['docker','ps','-q','--filter','volume='+volume]).strip()
source=out/'paused_capture_graph.cpp';source.write_text(source.read_text().replace('/private/tmp/nano-catalog-reactor-failfast-20261006/source/src/consensus_capella_projection.hpp','/source/src/consensus_capella_projection.hpp'))
compile_script=r'''set -eu
base=/build/coherent-efbeb9bc-amd64
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/reader.d -c /probe/paused_capture_graph.cpp -o /probe/reader.o"
printf '%s\n' "$cmd" > /probe/compile-command.txt
eval "$cmd" > /probe/compile.log 2>&1
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/\/probe\/reader.o}
cmd=${cmd//-o nano-capella-transition-tests /-o \/probe\/reader }
cmd=${cmd// libnano_core.a / \/build\/catalog-reactor-failfast-amd64-20261006\/libnano_core.a }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
sha256sum /probe/reader /probe/paused_capture_graph.cpp /build/catalog-reactor-failfast-amd64-20261006/libnano_core.a > /probe/programs.sha256
'''
(out/'compile.sh').write_text(compile_script)
cmd=['docker','run','--rm','--network','none','-v',build+':/build','-v','/tmp/nano-catalog-reactor-failfast-20261006/source:/source:ro','-v',str(out)+':/probe','--entrypoint','bash',image,'/probe/compile.sh']
p=subprocess.run(cmd);save('compile-controller-exit.json',{'exitCode':p.returncode});assert p.returncode==0
request=json.loads((audit/'request.json').read_text());request['runtimeBlockLimit']=1024;save('request.json',request)
base=['docker','run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',build+':/build:ro','-v',volume+':/var/lib/nano:ro','-v',str(root)+':/config:ro','-v',str(audit)+':/audit:ro','-v',str(out)+':/probe:ro']
hashcmd=base+['--entrypoint','sh',image,'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort'];before=run(hashcmd);assert before==(audit/'data-before.sha256').read_text()
with (out/'capture.json').open('w') as stdout,(out/'error.log').open('w') as stderr:p=subprocess.run(base+['--entrypoint','/probe/reader',image,'--capture','/config/genesis.json','/audit/config.json','/var/lib/nano/chain','/probe/request.json'],stdout=stdout,stderr=stderr)
save('reader-exit.json',{'exitCode':p.returncode});assert p.returncode==0
capture=json.loads((out/'capture.json').read_text());old=json.loads((audit/'capture.json').read_text());assert capture['restart']==old['restart'] and capture['validatorKeys']==old['validatorKeys'];assert before==run(hashcmd)
save('metrics.json',capture['forkGraphMetrics']);save('immutability.json',{'files':len(before.splitlines()),'hashesUnchanged':True,'stopPinsUnchanged':True,'privateRequestDerivedOnVps':True,'productionQualified':False});print(json.dumps(capture['forkGraphMetrics']))
