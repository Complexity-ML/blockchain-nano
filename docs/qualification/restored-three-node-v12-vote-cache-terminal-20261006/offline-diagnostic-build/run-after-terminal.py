from pathlib import Path
import json,hashlib,subprocess
root=Path('/private/tmp/nano-vote-refusal-diagnostic-20261006');campaign=Path('/private/tmp/nano-restored-three-node-v12-vote-cache-20261006/campaign');repo=Path('/Users/boris/Dev/blockchain-nano')
def read(p):return json.loads(p.read_text())
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
t=read(campaign/'terminal-exit.json');assert t['originalSession']==70646 and t['exitCode'] in [0,1]
assert read(campaign/'result.json')['stopReceipts']==3
assert len(list(campaign.glob('[0-2]-external-stop-receipt.json')))==3
ns=read(campaign/'native-state.json');assert ns['exitCode']==0 and ns['signalCode'] is None
assert not any(line.strip().split(maxsplit=1)[0]==str(ns['pid']) for line in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines() if line.strip())
res=read(campaign/'resources.json');ds=json.loads(subprocess.check_output(['docker','inspect',res['localContainer'],'--format','{{json .State}}'],text=True));assert not ds['Running'] and not ds['OOMKilled'] and ds['ExitCode']==0
# Independently compare old stopped v11 datasets against their committed post-stop digests.
request=read(root/'request.json');maps={}
for source in request['sources']:
 data=Path(source['database']).parent
 def hashes(d):return {str(p.relative_to(d)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(d.rglob('*')) if p.is_file()}
 expected=read(repo/('docs/qualification/restored-three-node-v11-target-terminal-20261006/post-stop-mac/'+source['name']+'-before.json'))
 actual=hashes(data);assert actual==expected,'Old source changed; refuse diagnostic'
 maps[source['name']]=actual
save(root/'data-before.json',maps)
compile=read(Path('/private/tmp/nano-atomic-proposal-native-20261006/test-compile-command.json'))
compile[compile.index('-c')+1]=str(root/'diagnose.cpp');compile[compile.index('-o')+1]=str(root/'diagnose.o')
link=read(Path('/private/tmp/nano-vote-cache-integrated-native-20261006/nano-capella-paused-capture-link-command.json'))
link=[str(root/'diagnose.o') if x.endswith('/capture.o') else x for x in link];link[link.index('-o')+1]=str(root/'diagnose')
for name,args in [('compile',compile),('link',link),('diagnostic',[str(root/'diagnose'),str(root/'request.json')])]:
 save(root/(name+'-command.json'),args)
 with (root/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT)
 save(root/(name+'-exit.json'),{'exitCode':r.returncode});assert r.returncode==0,name+' failed; retained diagnostic-development failure'
for source in request['sources']:assert hashes(Path(source['database']).parent)==maps[source['name']]
save(root/'immutability.json',{'hashesUnchanged':True,'sources':'stopped-v11-mac-only','activeNetworkCauseProven':False,'productionQualified':False})
print('Offline diagnostic completed; inspect classifications without inferring a v12 cause.')
