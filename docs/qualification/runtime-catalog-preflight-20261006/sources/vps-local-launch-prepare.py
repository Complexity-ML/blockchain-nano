#!/usr/bin/env python3
"""Prepare isolated restart from VPS-local audited configuration; emit digests only."""
import copy,hashlib,json,os,shutil,sys,re
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
 assert isinstance(options,dict) and set(options)<={'retentionExperiment','coldCheckpoint'}
 c=copy.deepcopy(configuration);a=c['capella'];limits=a['limits']
 if 'retentionExperiment' in options:
  e=options['retentionExperiment'];assert isinstance(e,dict) and {'blocks','rationale'}<=set(e)<={'blocks','rationale','captureBudget','catalogRootBytes'}
  assert isinstance(e['rationale'],str) and len(e['rationale'].strip())>=20 and len(e['rationale'])<=2048
  budget=e.get('captureBudget',{'blocks':'0x400','envelopeBytes':'0x8000000'});assert isinstance(budget,dict) and set(budget)=={'blocks','envelopeBytes'}
  target=positive_quantity(e['blocks']);count=positive_quantity(budget['blocks']);size=positive_quantity(budget['envelopeBytes'])
  assert size<=(2**64-1-2048)//8 and count<=size
  assert positive_quantity(limits['blocks'])<target<=count and positive_quantity(limits['envelopeBytes'])<=size
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

def prepare(old,dest,rpc_port,p2p_port,binary,expected,runtime_options=None):
 old=Path(old);dest=Path(dest);assert str(old).startswith('/tmp/nano-restored-three-node-vps-') and str(dest).startswith('/tmp/nano-restored-three-node-vps-') and old!=dest
 audit=old/'local-readonly-audit';proof=json.loads((audit/'immutability.json').read_text());assert proof['hashesUnchanged'] and proof['exactStopCommitmentsVerified']
 for name,h in json.loads((audit/'request-config-hashes.json').read_text()).items():assert hashlib.sha256((audit/name).read_bytes()).hexdigest()==h
 c=derive(json.loads((audit/'config.json').read_text()),json.loads((audit/'capture.json').read_text()),rpc_port,p2p_port,binary,runtime_options)
 assert canonical_hash(c)==expected,'Local derived configuration differs from independent controller plan'
 keys={k['keyEnv'] for k in c['capella']['validator']['keys']};env=(old/'vps.env').read_text();lines=env.splitlines();assert len(lines)==len(set(v.split('=',1)[0] for v in lines));assert {v.split('=',1)[0] for v in lines}==keys|{'NANO_TEST_NETWORK_TRACE'}
 assert not dest.exists();dest.mkdir(mode=0o755)
 for name in ['genesis.json','profile.json','initial.json']:shutil.copy2(old/name,dest/name)
 (dest/'vps.json').write_text(json.dumps(c,indent=2)+'\n');(dest/'vps.env').write_text(env);os.chmod(dest/'vps.env',0o600)
 result={'configCanonicalSHA256':expected,'configPreparedLocally':True,'configurationExportedFromMac':False,'signingKeysExportedFromMac':False,'validatorKeys':len(keys),'files':{name:hashlib.sha256((dest/name).read_bytes()).hexdigest() for name in ['genesis.json','profile.json','initial.json','vps.json','vps.env']}}
 (dest/'local-preparation-proof.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
 print(json.dumps(prepare(sys.argv[1],sys.argv[2],int(sys.argv[3]),int(sys.argv[4]),sys.argv[5],sys.argv[6],json.loads(sys.argv[7]) if len(sys.argv)==8 else None)))
