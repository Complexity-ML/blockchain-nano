from pathlib import Path
import subprocess,json,hashlib
base=Path('/private/tmp');campaign=base/'nano-restored-three-node-v25-timeout-preparation-20261006/campaign';audit=base/'nano-v25-post-stop-native-replay-budget-20261006';source=base/'nano-v25-inventory-diagnostic-source-20261006';out=base/'nano-v25-inventory-diagnostic-native-20261006'
def read(p):return json.loads(p.read_text())
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
assert read(out/'original-terminal-exit.json')['originalSession']==90287 and read(out/'compile-exit.json')['exitCode']==0
assert read(campaign/'terminal-exit.json')['originalSession']==29725 and read(campaign/'result.json')['stopReceipts']==3
parent=base/'nano-network-status-coherent-native-20261006';assert hashlib.sha256((parent/'libnano_core.a').read_bytes()).hexdigest()=='4eb5122e27b9f4ffaa78c5bd63ddbc9e2e6f7f01dff9a45b1d88a69529915d99'
cpp=source/'tests/capella/paused_capture.cpp';source_hash=hashlib.sha256(cpp.read_bytes()).hexdigest();assert source_hash==read(out/'source-before.json')['sha256']
l=read(out/'link-command.json');save(out/'link-repair-command.json',{'args':l,'cwd':'/tmp/nano-pruning-wan-native-build-20261004'})
with (out/'link-repair.log').open('w') as f:r=subprocess.run(l,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT)
save(out/'link-repair-exit.json',{'exitCode':r.returncode});assert r.returncode==0
resources=read(campaign/'resources.json');state=json.loads(subprocess.check_output(['docker','inspect',resources['localContainer'],'--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
processes=subprocess.check_output(['ps','-axo','pid,comm'],text=True);assert not any(x.strip().split(maxsplit=1)[0]==str(resources['nativePid']) for x in processes.splitlines() if x.strip())
for name in ['mac-native','mac-docker']:
 data=Path(resources['prepared'])/name/'data'
 def hashes():return {str(p.relative_to(data)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(data.rglob('*')) if p.is_file()}
 before=hashes();assert before==read(audit/(name+'-before.json')),'Source differs from prior full audit'
 req=read(audit/(name+'-request.json'));req['inventoryOnly']=True;save(out/(name+'-request.json'),req)
 args=[str(out/'reader'),'--capture',str(Path(resources['prepared'])/'config/genesis.json'),str(audit/(name+'-config.json')),str(data/'chain'),str(out/(name+'-request.json'))];save(out/(name+'-command.json'),args)
 with (out/(name+'-capture.json')).open('w') as stdout,(out/(name+'-error.log')).open('w') as stderr:r=subprocess.run(args,stdout=stdout,stderr=stderr)
 save(out/(name+'-exit.json'),{'exitCode':r.returncode});assert r.returncode==0
 assert hashes()==before;capture=read(out/(name+'-capture.json'));assert capture['restart']==read(audit/(name+'-capture.json'))['restart'];save(out/(name+'-immutability.json'),{'files':len(before),'hashesUnchanged':True,'sameFilesAsPriorFullAudit':True});print(name,json.dumps(capture),flush=True)
assert hashlib.sha256(cpp.read_bytes()).hexdigest()==source_hash;save(out/'provenance.json',{'currentCoreSHA256':'4eb5122e27b9f4ffaa78c5bd63ddbc9e2e6f7f01dff9a45b1d88a69529915d99','sourceSHA256':source_hash,'readerSHA256':hashlib.sha256((out/'reader').read_bytes()).hexdigest(),'sourceUnchanged':True,'canonicalAuditRepeated':False,'runtimeChanged':False,'privateFixtureOnly':True})
