from pathlib import Path
import subprocess,json,hashlib,sys
root=Path(sys.argv[1]);container=sys.argv[2];volume=sys.argv[3];image=sys.argv[4];buildvolume=sys.argv[5]
assert str(root).startswith('/tmp/nano-restored-three-node-vps-')
assert container.startswith('nano-restored-three-node-vps-') and volume.startswith('nano-restored-three-node-vps-')
def run(args):return subprocess.check_output(args,text=True)
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
state=json.loads(run(['docker','inspect',container,'--format','{{json .State}}']));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
assert not run(['docker','ps','-q','--filter','volume='+volume]).strip()
log=subprocess.run(['docker','logs',container],capture_output=True,text=True,check=True)
rows=[]
for line in (log.stdout+'\n'+log.stderr).splitlines():
 try:rows.append(json.loads(line))
 except ValueError:pass
receipt=[x for x in rows if x.get('event')=='capella-runtime-stopped'][-1]
config=json.loads((root/'vps.json').read_text());c=config['capella'];c['recoveryCommitment']=receipt['restart'];c['automaticRecovery']=False;c['initial']=None
for key in c['validator']['keys']:key['restart']=next(x['restart'] for x in receipt['validatorKeys'] if x['index']==key['index'])
request={'profile':{k:c['profile'][k] for k in ['file','sha256']},'anchor':{'file':'/config/initial.json','sha256':'0x780dd0ba2cb38f95f2c0e24c9ee15887c19f974a9838679ca99acaae923e239a'},'limits':c['limits'],'maximumBlocks':'0x400','maximumArchiveBytes':'0x20000000','requiredFinalized':receipt['finalized'],'minimumCheckpoint':receipt['restart'],'minimumValidatorKeys':receipt['validatorKeys'],'minimumAttestationTarget':'0x4c'}
out=root/'local-readonly-audit';out.mkdir(exist_ok=False);save(out/'config.json',config);save(out/'request.json',request);save(out/'source-stopped-state.json',state)
base=['docker','run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',buildvolume+':/build:ro','-v',volume+':/var/lib/nano:ro','-v',str(root)+':/config:ro','-v',str(out)+':/output:ro']
reader='/build/catalog-selection-amd64-20261006/nano-capella-paused-capture'
program=run(base+['--entrypoint','sha256sum',image,reader]).split()[0];assert program=='c969fc627a586e19cb08f191eedfafc90947ef9342774ef138e84dcadb287e33';save(out/'program-hash.json',{'sha256':program})
hashcmd=base+['--entrypoint','sh',image,'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort']
before=run(hashcmd);(out/'data-before.sha256').write_text(before)
cmd=base+['--entrypoint',reader,image,'--capture','/config/genesis.json','/output/config.json','/var/lib/nano/chain','/output/request.json']
with (out/'capture.json').open('w') as stdout,(out/'error.log').open('w') as stderr:p=subprocess.run(cmd,stdout=stdout,stderr=stderr)
save(out/'exit.json',{'exitCode':p.returncode});assert p.returncode==0
assert before==run(hashcmd);d=json.loads((out/'capture.json').read_text());assert d['restart']==receipt['restart'];assert {x['index']:x['restart'] for x in d['validatorKeys']}=={x['index']:x['restart'] for x in receipt['validatorKeys']}
save(out/'immutability.json',{'files':len(before.splitlines()),'hashesUnchanged':True,'exactStopCommitmentsVerified':True,'configAndRequestBuiltOnVps':True,'macConfigOrDatabaseTransferred':False,'productionQualified':False})
save(out/'request-config-hashes.json',{f:hashlib.sha256((out/f).read_bytes()).hexdigest() for f in ['config.json','request.json']})
print(json.dumps({'passed':True,'files':len(before.splitlines()),'output':str(out)}))
