from pathlib import Path
import subprocess,json,hashlib,shutil
root=Path('/private/tmp/nano-maintenance-portability-v3-20261005');source=Path('/tmp/nano-pruned-migration-arm64-fixture-20261005/chain');old=Path('/private/tmp/nano-pruned-migration-arm64-control-20261005');config=old/'config';request=json.loads((old/'audit-request.json').read_text())
request['profile']['file']=str(config/'profile.json');request['anchor']['file']=str(config/'initial.json');(root/'request.json').write_text(json.dumps(request))
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
def run(args,name,expect=0):
 p=subprocess.run([str(a) for a in args],capture_output=True,text=True);(root/(name+'.log')).write_text(p.stdout+p.stderr);(root/(name+'-exit.txt')).write_text(str(p.returncode)+'\n');assert p.returncode==expect,(name,p.returncode,p.stderr)
 
 if expect==1:assert ('marker' in p.stderr.lower() or (name=='migrate-symlink-refused' and 'Owned migration chain copy required' in p.stderr)),(name,p.stderr)
 return json.loads(p.stdout) if p.stdout.strip().startswith('{') else None
before=hashes(source);(root/'source-before.json').write_text(json.dumps(before,indent=2))
for operation in ['migrate','compact']:
 folder=root/('native-'+operation);folder.mkdir();shutil.copytree(source,folder/'chain');marker=folder/('.nano-owned-schema-migration-copy' if operation=='migrate' else '.nano-owned-compaction-copy');flag='owned-offline-schema-migration-v1' if operation=='migrate' else 'owned-offline-qualification-copy-v1'
 args=[root/(operation+'-native'),'--offline-owned-copy',folder/'chain']
 if operation=='migrate':args+=['--audit-request',root/'request.json']
 pre=hashes(folder/'chain')
 marker.symlink_to(root/'marker-target');(root/'marker-target').write_text(flag+'\n');run(args,operation+'-symlink-refused',1);marker.unlink()
 marker.write_text(flag+'\n'+'X'*200);run(args,operation+'-oversize-refused',1)
 marker.write_text(flag+'\nextra');run(args,operation+'-ambiguous-refused',1)
 assert pre==hashes(folder/'chain')
 marker.write_text(flag+'\n')
 run(['/tmp/nano-pruning-wan-native-build-20261004/nano','audit-capella',config/'genesis.json',folder/'chain',root/'request.json'],operation+'-audit-before')
 result=run(args,operation+'-success')
 if operation=='migrate':
  assert result['onlySchemaChanged'];retry=run(args,operation+'-idempotent');assert retry['writes']==0
  audit_program='/tmp/nano-capella-sync-native-build-20261005/nano'
 else:
  assert result['logicalContentUnchanged'];audit_program='/tmp/nano-pruning-wan-native-build-20261004/nano'
 run([audit_program,'audit-capella',config/'genesis.json',folder/'chain',root/'request.json'],operation+'-audit-after')
 assert hashes(source)==before
(root/'source-after.json').write_text(json.dumps(hashes(source),indent=2));(root/'native-result.json').write_text(json.dumps({'passed':True,'sourceUnchanged':True,'rejectionsBeforeOpen':6,'bothCanonicalAuditsPassed':True,'architecture':'native-arm64','rocksdb':'11.8.1'}))
