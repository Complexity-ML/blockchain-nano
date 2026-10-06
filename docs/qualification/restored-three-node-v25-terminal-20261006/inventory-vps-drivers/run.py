from pathlib import Path
import subprocess,json,hashlib,sys
root=Path(sys.argv[1]);container=sys.argv[2];volume=sys.argv[3];image=sys.argv[4];buildvolume=sys.argv[5];driver=Path(sys.argv[6])
def run(args):return subprocess.check_output(args,text=True)
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
assert str(root).startswith('/tmp/nano-restored-three-node-vps-')
state=json.loads(run(['docker','inspect',container,'--format','{{json .State}}']));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled'];assert not run(['docker','ps','-q','--filter','volume='+volume]).strip()
audit=root/'local-readonly-audit';assert json.loads((audit/'exit.json').read_text())['exitCode']==0 and json.loads((audit/'immutability.json').read_text())['hashesUnchanged']
proof=driver/'proof';proof.mkdir(exist_ok=False);(driver/'source/src').symlink_to('/work/src',target_is_directory=True)
build=['docker','run','--rm','--network','none','--memory','4g','--cpus','2','--ulimit','nofile=65536:65536','-v',buildvolume+':/build','-v','/home/ubuntu/blockchain-nano-git:/work:ro','-v',str(driver)+':/drivers',image,'bash','/drivers/build.sh']
save(proof/'build-command.json',build);p=subprocess.run(build);assert p.returncode==0
request=json.loads((audit/'request.json').read_text());request['inventoryOnly']=True;requestfile=root/'inventory-diagnostic-request.json';save(requestfile,request)
base=['docker','run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',buildvolume+':/build:ro','-v',volume+':/var/lib/nano:ro','-v',str(root)+':/config:ro']
hashcmd=base+['--entrypoint','sh',image,'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort'];before=run(hashcmd);assert before==(audit/'data-before.sha256').read_text(),'Inventory source changed after full audit'
reader='/build/v25-inventory-diagnostic-amd64-20261006/reader';cmd=base+['--entrypoint',reader,image,'--capture','/config/genesis.json','/config/local-readonly-audit/config.json','/var/lib/nano/chain','/config/inventory-diagnostic-request.json']
with (proof/'capture.json').open('w') as stdout,(proof/'error.log').open('w') as stderr:p=subprocess.run(cmd,stdout=stdout,stderr=stderr)
save(proof/'exit.json',{'exitCode':p.returncode});assert p.returncode==0;assert run(hashcmd)==before
capture=json.loads((proof/'capture.json').read_text());assert capture['restart']==json.loads((audit/'capture.json').read_text())['restart'];save(proof/'immutability.json',{'files':len(before.splitlines()),'hashesUnchanged':True,'sameFilesAsPriorFullAudit':True,'configAndRequestKeptOnVps':True,'canonicalAuditRepeated':False})
save(proof/'source.json',{'sha256':hashlib.sha256((driver/'source/tests/capella/paused_capture.cpp').read_bytes()).hexdigest()});print(json.dumps({'passed':True,'readOnly':True,'publicProof':str(proof)}))
