#!/usr/bin/env python3
"""Prepare isolated restart from VPS-local audited configuration; emit digests only."""
import copy,hashlib,json,os,shutil,sys,re,stat
from pathlib import Path

def canonical_hash(value):
 return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()).hexdigest()

def qualified_p2p_binary(binary):
 # The controller pins and verifies the exact binary digest before invoking
 # this helper. Validate placement/architecture, not the qualification lot name.
 return isinstance(binary,str) and re.fullmatch(r'/build/[a-z0-9]+(?:-[a-z0-9]+)*-amd64(?:-[0-9]{8})?/network/nano-p2p',binary) is not None

def positive_quantity(value):
 assert isinstance(value,str) and re.fullmatch(r'0x[1-9a-f][0-9a-f]*',value)
 result=int(value,16);assert result<=2**64-1
 return result

def apply_runtime_options(configuration,options):
 assert isinstance(options,dict) and set(options)<={'retentionExperiment','coldCheckpoint','startupCatchup'}
 c=copy.deepcopy(configuration);a=c['capella'];limits=a['limits']
 if 'retentionExperiment' in options:
  e=options['retentionExperiment'];assert isinstance(e,dict) and {'blocks','rationale'}<=set(e)<={'blocks','rationale','captureBudget','catalogRootBytes','sourceBlockBudgets'}
  assert isinstance(e['rationale'],str) and len(e['rationale'].strip())>=20 and len(e['rationale'])<=2048
  budget=e.get('captureBudget',{'blocks':'0x400','envelopeBytes':'0x8000000'});assert isinstance(budget,dict) and set(budget)=={'blocks','envelopeBytes'}
  target=positive_quantity(e['blocks']);count=positive_quantity(budget['blocks']);size=positive_quantity(budget['envelopeBytes'])
  assert size<=(2**64-1-2048)//8 and count<=size
  source=positive_quantity(limits['blocks'])
  if 'sourceBlockBudgets' in e:
   declared=e['sourceBlockBudgets'];assert isinstance(declared,dict) and set(declared)=={'mac-native','mac-docker','vps-docker'}
   values=[positive_quantity(v) for v in declared.values()];assert declared['vps-docker']==limits['blocks']
   assert all(target>=v for v in values) and any(target>v for v in values)
  else:assert source<target
  assert source<=target<=count and positive_quantity(limits['envelopeBytes'])<=size
  if 'catalogRootBytes' in e:
   roots=positive_quantity(e['catalogRootBytes']);assert roots<=size
  else:roots=positive_quantity(a['catalogs']['rootBytes'])
  assert roots>=positive_quantity(a['catalogs']['rootBytes']) and max(target,positive_quantity(limits['messages']),positive_quantity(limits['equivocations']))<=roots//32
  if 'catalogRootBytes' in e:a['catalogs']['rootBytes']=e['catalogRootBytes']
  limits['blocks']=e['blocks']
 if 'coldCheckpoint' in options:
  cold=options['coldCheckpoint'];assert isinstance(cold,dict) and set(cold)=={'cacheRecords','cacheEncodedBytes'}
  assert positive_quantity(cold['cacheRecords'])<=positive_quantity(limits['blocks'])
  assert positive_quantity(cold['cacheEncodedBytes'])<=positive_quantity(limits['envelopeBytes'])
  if 'coldCheckpoint' in a:assert a['coldCheckpoint']==cold,'Existing cold options cannot be changed by preparation'
  a['coldCheckpoint']=copy.deepcopy(cold)
 if 'startupCatchup' in options:
  catchup=options['startupCatchup'];assert isinstance(catchup,dict) and set(catchup)=={'maximumDowntimeMs','maximumSteps'}
  downtime=positive_quantity(catchup['maximumDowntimeMs']);steps=positive_quantity(catchup['maximumSteps'])
  forward=positive_quantity(a['clock']['maximumForwardStepMs'])
  assert steps>=(downtime+forward-1)//forward,'Catch-up steps below unchanged forward-step bound'
  a['clock']['startupCatchup']=copy.deepcopy(catchup)
 return c

def derive(configuration,capsule,rpc_port,p2p_port,binary,runtime_options=None):
 assert 0<rpc_port<65536 and 0<p2p_port<65536 and rpc_port!=p2p_port
 assert qualified_p2p_binary(binary)
 assert capsule['readOnlyStores'] and capsule['primaryWitnessAndSignersVerified']
 assert capsule['minimumSequenceBoundsVerified'] and not capsule['productionQualified']
 assert capsule['format']=='nano-private-quiescent-recovery-pins-v1' and capsule['chainId']=='0x539'
 assert capsule['profileSha256']==configuration['capella']['profile']['sha256']
 assert capsule['canonicalReplay']['canonicalHistoryVerified'] and capsule['canonicalReplay']['externalFinalizedAncestorVerified']
 assert all(not k['retired'] for k in capsule['validatorKeys'])
 c=copy.deepcopy(configuration);a=c['capella'];a['restart']=None;a['initial']=None;a['automaticRecovery']=False;a['recoveryCommitment']=capsule['restart']
 def identity(k):return ('pending',k['publicKey']) if k['index'] is None else ('index',int(k['index'],16))
 pins={identity(k):k['restart'] for k in capsule['validatorKeys']};assert len(pins)==len(capsule['validatorKeys'])==len(a['validator']['keys'])
 for k in a['validator']['keys']:
  if k['index'] is None:
   pending=[v for v in capsule['validatorKeys'] if v['index'] is None];assert len(pending)==1;key=identity(pending[0]);assert k['keyEnv']=='NANO_TEST_CAPELLA_NETWORK_KEY_64'
  else:key=identity(k)
  assert key in pins;k['restart']=pins.pop(key)
 assert not pins
 if runtime_options is not None:c=apply_runtime_options(c,runtime_options)
 c.update({'p2p_binary':binary,'p2p_identity':'/var/lib/nano/peer.key','bootnodes':[],'rpc_host':'127.0.0.1','rpc_port':rpc_port,'listen_addr':f'/ip4/127.0.0.1/tcp/{p2p_port}'})
 a['profile']['file']='/config/profile.json';a['network']['enabled']=True
 return c

def manifest_records(wire,prefix):
 records={}
 for line in wire.decode('utf8').splitlines():
  digest,path=line.split('  ',1);assert re.fullmatch(r'[0-9a-f]{64}',digest) and path.startswith(prefix)
  rel=path[len(prefix):];parts=rel.split('/')
  assert rel and all(part not in ('','.','..') for part in parts) and not any(c in rel for c in ('\\','\n','\r','\0'))
  assert rel not in records,'Duplicate data manifest entry';records[rel]=digest
 assert records,'Empty data manifest'
 return records

def tree_hashes(directory):
 directory=Path(directory);assert directory.is_dir() and not directory.is_symlink()
 records={};inodes={}
 for path in sorted(directory.rglob('*')):
  assert not path.is_symlink(),'Symlink in restored data'
  mode=path.stat().st_mode
  if stat.S_ISDIR(mode):continue
  assert stat.S_ISREG(mode),'Non-regular restored data file'
  rel=path.relative_to(directory).as_posix();digest=hashlib.sha256()
  with path.open('rb') as stream:
   for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
  records[rel]=digest.hexdigest();info=path.stat();inodes[rel]=(info.st_dev,info.st_ino)
 assert records,'Empty restored data directory'
 return records,inodes

def verify_runtime_copy_audit(proof,capture_bytes,bindings,audit):
 # A witnessed runtime refusal is not a healthy stop or a forced-stop137 audit.
 # All evidence is sealed before preparing a destination; actual data are read.
 assert proof.get('format')=='nano-private-audited-runtime-copy-source-v1'
 assert proof.get('exactStopCommitmentsVerified') is not True
 for key in ['hashesUnchanged','requestPreparedOnVps','noPrivateDataExport','readOnly']:assert proof.get(key) is True,key
 assert audit is not None,'Runtime-copy audit requires filesystem evidence'
 audit=Path(audit);assert audit.is_dir() and not audit.is_symlink()
 parent=audit.parent.parent if audit.name=='local-readonly-audit' else audit.parent
 assert audit.name in ('local-readonly-audit','private-audit')
 def bound(name):
  path=audit/name;assert path.is_file() and not path.is_symlink()
  wire=path.read_bytes();assert bindings.get(name)==hashlib.sha256(wire).hexdigest(),'Unbound runtime-copy artifact: '+name
  return wire
 raw=bound('source-runtime-refusal-copy-audit.json');a=json.loads(raw)
 assert proof.get('runtimeRefusalAuditSHA256')==hashlib.sha256(raw).hexdigest()
 assert a.get('format')=='nano-runtime-refusal-copy-audit-v1'
 for key in ['sourceObservedExited1NoOom','sourcePassedRuntimeReady','sourceFilesHashesUnchanged','independentCopyVerified','sourcePrimaryWitnessMismatchObserved','witnessedIntentRecoveryVerified','recoveryPerformedOnCopyOnly','canonicalReplayVerified','signerAndPeerFilesUnchanged','independentSignerWitnessPairsVerified','preCampaignExternalMinimumsVerified']:assert a.get(key) is True,key
 for key in ['terminalExternalStopReceiptAvailable','exactTerminalExternalPinsVerified','recoveryRuntimeDutiesResumed','networkQualified','productionQualified']:assert a.get(key) is False,key
 for token in [a['sourcePrimary'],a['witnessIntentNext'],a['recoveredPrimary']]:
  assert set(token)=={'sequence','digest'};positive_quantity(token['sequence']);assert re.fullmatch(r'0x[0-9a-f]{64}',token['digest'])
 assert int(a['witnessIntentNext']['sequence'],16)==int(a['sourcePrimary']['sequence'],16)+1
 assert a['recoveredPrimary']==a['witnessIntentNext']
 assert bound('capture.json')==capture_bytes
 capsule=json.loads(capture_bytes);digest=hashlib.sha256(capture_bytes).hexdigest()
 assert proof.get('captureSHA256')==a['externalCurrentCaptureSha256']==digest
 assert proof.get('auditedRecoveryCommitment')==capsule['restart']==a['recoveredPrimary']
 assert len(capsule['validatorKeys'])==33 and capsule['primaryWitnessAndSignersVerified'] is True
 assert capsule['canonicalReplay']['canonicalHistoryVerified'] is True and capsule['minimumSequenceBoundsVerified'] is True
 before=bound('source-before.sha256');copied=bound('copy-before.sha256');after=bound('copy-after.sha256')
 assert bound('source-volume-current.sha256')==before,'Original source volume changed since sealed audit'
 assert hashlib.sha256(before).hexdigest()==proof.get('sourceManifestSha256')==a['sourceManifestSha256']
 assert hashlib.sha256(after).hexdigest()==proof.get('recoveredManifestSha256')
 original=manifest_records(before,'/source/');initial=manifest_records(copied,'/recovery/data/');recovered=manifest_records(after,'/recovery/data/')
 assert original==initial,'Physical copy differs from original audit inventory'
 protected=lambda rows:{name:h for name,h in rows.items() if not name.startswith(('chain/','witness/'))}
 assert protected(initial)==protected(recovered),'Signer journals, independent witnesses or peer identity changed during recovery'
 copy_proof=json.loads(bound('copy-proof.json'));recovery_proof=json.loads(bound('recovery-proof.json'));recovery=json.loads(bound('recovery.json'))
 assert copy_proof.get('sourceManifestSha256')==a['sourceManifestSha256'] and copy_proof.get('files')==len(original)
 assert isinstance(copy_proof.get('sourceVolume'),str) and re.fullmatch(r'nano-restored-three-node-vps-[a-z0-9-]+-data',copy_proof['sourceVolume'])
 assert isinstance(copy_proof.get('copyVolume'),str) and copy_proof['copyVolume']!=copy_proof['sourceVolume']
 for key in ['independentCopyVerified','sourceAllWritersStopped','sourceHashesUnchanged','noSignersStarted','originalSourceMountedReadOnly']:assert copy_proof.get(key) is True,key
 assert recovery_proof.get('result')=='PASSED_OFFLINE_COPY_ONLY'
 for key in ['sourceOriginalHashesUnchanged','allSignerAndPeerFilesUnchanged','primaryWitnessIntentRecovered','idempotentReopenCheckpointUnchanged','current33SignerWitnessPairsVerified','canonicalReplayVerified','externalPreV37MinimumsVerified']:assert recovery_proof.get(key) is True,key
 for key in ['terminalV37ExternalPinsAvailable','runtimeDutiesResumed','networkQualified','productionQualified']:assert recovery_proof.get(key) is False,key
 assert recovery_proof.get('captureSha256')==digest and recovery_proof.get('sourceFiles')==len(original)
 assert recovery.get('beforePrimary')==a['sourcePrimary'] and recovery.get('witnessNext')==recovery.get('afterPrimary')==a['recoveredPrimary']
 assert recovery.get('copiedStoresOnly') is True and recovery.get('signingInvoked') is False and recovery.get('validatorDutiesStarted') is False
 original_path=parent/'source-original';recovered_path=parent/'recovered-data'
 assert original_path.resolve().parent==parent.resolve() and recovered_path.resolve().parent==parent.resolve()
 actual_original,original_inodes=tree_hashes(original_path);actual_recovered,recovered_inodes=tree_hashes(recovered_path)
 assert actual_original==original,'Preserved original full-file inventory changed'
 assert actual_recovered==recovered,'Recovered full-file inventory changed'
 assert not set(original_inodes.values())&set(recovered_inodes.values()),'Recovered copy shares original file storage'
 assert type(proof.get('files')) is int and proof['files']==len(recovered)
 return {'runtimeRefusalAuditSHA256':hashlib.sha256(raw).hexdigest(),'sourceManifestSha256':a['sourceManifestSha256'],'recoveredManifestSha256':hashlib.sha256(after).hexdigest(),'captureSHA256':digest,'sourceFiles':len(original),'recoveredFiles':len(recovered),'actualOriginalAndRecoveredFilesVerified':True}

def remap_runtime_copy_paths(configuration):
 c=copy.deepcopy(configuration)
 def remap(path):
  assert isinstance(path,str) and path.startswith('/recovery/data/')
  rel=path[len('/recovery/data/'):];assert rel and all(part not in ('','.','..') for part in rel.split('/'))
  return '/var/lib/nano/'+rel
 a=c['capella'];a['recoveryWitness']=remap(a['recoveryWitness'])
 for key in a['validator']['keys']:
  for field in ['journal','recoveryWitness']:key[field]=remap(key[field])
 return c

def verify_local_audit(proof,capture_bytes,bindings,audit=None):
 assert proof.get('hashesUnchanged') is True
 if proof.get('format')=='nano-private-audited-runtime-copy-source-v1':return verify_runtime_copy_audit(proof,capture_bytes,bindings,audit)
 if proof.get('exactStopCommitmentsVerified') is True:return
 # A forced stop has no healthy stop receipt. Bind its independently replayed
 # current capsule instead; signer pins must still match the external inventory.
 assert proof.get('format')=='nano-private-audited-crash-source-v1'
 for key in ['minimumCoreAndSignerCommitmentsVerified','primaryAndIndependentWitnessVerified','requestPreparedOnVps','noPrivateDataExport','readOnly']:
  assert proof.get(key) is True,key
 assert proof.get('sourceForcedStopExit')==137 and proof.get('sourceOomKilled') is False
 capsule=json.loads(capture_bytes);count=len(capsule['validatorKeys'])
 assert count>0 and proof.get('exactUnchangedSignerPins')==proof.get('signerInventory')==count
 assert isinstance(proof.get('files'),int) and proof['files']>0
 digest=hashlib.sha256(capture_bytes).hexdigest()
 assert proof.get('captureSHA256')==bindings.get('capture.json')==digest
 assert proof.get('auditedRecoveryCommitment')==capsule['restart']

def prepare(old,dest,rpc_port,p2p_port,binary,expected,runtime_options=None):
 old=Path(old);dest=Path(dest);durable=Path('/home/ubuntu/nanobees-qualification')
 legacy=str(old).startswith('/tmp/nano-restored-three-node-vps-') and str(dest).startswith('/tmp/nano-restored-three-node-vps-')
 persistent=old.name=='config-origin' and old.resolve().is_relative_to(durable.resolve()) and dest.resolve().is_relative_to(durable.resolve()) and dest.parent.resolve()==(durable/'runtime').resolve() and re.fullmatch(r'nano-restored-three-node-vps-[a-z0-9-]+',dest.name)
 assert (legacy or persistent) and old!=dest and not old.is_symlink() and not dest.is_symlink()
 if persistent:assert old.parent.resolve().is_relative_to(durable.resolve()) and not old.parent.is_symlink()
 if legacy:assert old.resolve().is_relative_to(Path('/tmp').resolve()) and dest.resolve().is_relative_to(Path('/tmp').resolve())
 audit=old/'local-readonly-audit'
 if not audit.exists() and old.name=='config-origin':audit=old.parent/'private-audit'
 proof=json.loads((audit/'immutability.json').read_text());bindings=json.loads((audit/'request-config-hashes.json').read_text())
 for name,h in bindings.items():assert hashlib.sha256((audit/name).read_bytes()).hexdigest()==h
 runtime=verify_local_audit(proof,(audit/'capture.json').read_bytes(),bindings,audit)
 configuration=json.loads((audit/'config.json').read_text())
 if runtime is not None:
  assert (audit/'config.json').is_file() and not (audit/'config.json').is_symlink()
  assert bindings.get('config.json')==hashlib.sha256((audit/'config.json').read_bytes()).hexdigest()
  configuration=remap_runtime_copy_paths(configuration)
 c=derive(configuration,json.loads((audit/'capture.json').read_text()),rpc_port,p2p_port,binary,runtime_options)
 assert canonical_hash(c)==expected,'Local derived configuration differs from independent controller plan'
 keys={k['keyEnv'] for k in c['capella']['validator']['keys']};env=(old/'vps.env').read_text();lines=env.splitlines();assert len(lines)==len(set(v.split('=',1)[0] for v in lines));assert {v.split('=',1)[0] for v in lines}==keys|{'NANO_TEST_NETWORK_TRACE'}
 assert not dest.exists();dest.mkdir(mode=0o755)
 for name in ['genesis.json','profile.json','initial.json']:shutil.copy2(old/name,dest/name)
 (dest/'vps.json').write_text(json.dumps(c,indent=2)+'\n');(dest/'vps.env').write_text(env);os.chmod(dest/'vps.env',0o600)
 result={'configCanonicalSHA256':expected,'configPreparedLocally':True,'configurationExportedFromMac':False,'signingKeysExportedFromMac':False,'validatorKeys':len(keys),'files':{name:hashlib.sha256((dest/name).read_bytes()).hexdigest() for name in ['genesis.json','profile.json','initial.json','vps.json','vps.env']}}
 if runtime is not None:result['runtimeCopySource']={**runtime,'auditedConfigSHA256':bindings['config.json'],'onlyWitnessAndJournalPathsRemapped':True}
 (dest/'local-preparation-proof.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
 print(json.dumps(prepare(sys.argv[1],sys.argv[2],int(sys.argv[3]),int(sys.argv[4]),sys.argv[5],sys.argv[6],json.loads(sys.argv[7]) if len(sys.argv)==8 else None)))
