// Public typed result validator plus OWNER-LOCAL Python projection source.
// Importing this module reads no file, clock, network, environment or signer.
const need=(ok,message)=>{if(!ok)throw Error('VPS duty floor: '+message);};
const plain=v=>v!==null&&typeof v==='object'&&!Array.isArray(v);
const exact=(v,keys,label)=>need(plain(v)&&Object.keys(v).sort().join(',')===[...keys].sort().join(','),label+' fields');
const sha=/^[0-9a-f]{64}$/,root=/^0x[0-9a-f]{64}$/,pub=/^0x[0-9a-f]{96}$/;
const q=v=>{need(typeof v==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(v),'canonical hexadecimal uint64');const n=BigInt(v);need(n<1n<<64n,'uint64 range');return n;};
const hex=n=>'0x'+n.toString(16);
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':plain(v)?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const pin=(v,physical=false)=>{need(plain(v)&&sha.test(v.sha256)&&Number.isSafeInteger(v.bytes)&&v.bytes>0&&v.bytes<=16777216&&(!physical||sha.test(v.materialSha256)),'bounded external artifact pin');return {sha256:v.sha256,bytes:v.bytes,...(physical?{materialSha256:v.materialSha256}:{})};};
export function vpsProposalFloor(headSlot,proposalSlot){
 need(Number.isSafeInteger(headSlot)&&headSlot>=0,'safe numeric C++ head slot');const head=BigInt(headSlot),proposal=proposalSlot===null?0n:q(proposalSlot);return hex(head>proposal?head:proposal);
}
const fields=['format','owner','campaignId','selectedPublicKey','selectedPreparedPublicKeyIndex','selectedResolvedIndex','selectedProvisionedIndex','headSlot','journalProposalSlot','cutoffSlot','capsule','stop','manifest','witnessOriginSha256','corePrimaryCommitmentSha256','coreWitnessCommitmentSha256','selectedPrimaryCommitmentSha256','selectedWitnessCommitmentSha256','primaryWitnessPairs','readOnlyDatabaseRoles','indexFromAuthenticatedCapturedRegistry','sourceFilesUnchanged','newSignaturesCreated','rawCapsuleExported','snapshotBytesExported','signingRootExported','privatePathsExported','runtimeStarted','productionQualified'];
function validateProjection(value,expected,capsule){
 exact(value,fields,'public projection');need(value.format==='nano-fresh-vps-duty-floor-public-v1'&&value.owner==='vps-docker'&&value.campaignId===expected.campaignId&&sha.test(value.campaignId),'owner/campaign');
 need(pub.test(value.selectedPublicKey)&&value.selectedPublicKey===expected.selectedPublicKey&&value.selectedPreparedPublicKeyIndex===expected.selectedPreparedPublicKeyIndex&&Number.isSafeInteger(value.selectedPreparedPublicKeyIndex)&&value.selectedPreparedPublicKeyIndex>=0,'prepared public identity');
 need(value.selectedResolvedIndex===expected.resolvedIndex&&q(value.selectedResolvedIndex)===BigInt(value.selectedPreparedPublicKeyIndex),'resolved index belongs to captured public identity');
 need(value.selectedProvisionedIndex===(expected.selectedIsPending?null:expected.resolvedIndex),'pending versus provisioned identity');
 need(value.headSlot===hex(BigInt(expected.expectedHeadSlot))&&value.cutoffSlot===vpsProposalFloor(expected.expectedHeadSlot,value.journalProposalSlot),'head/journal maximum');
 for(const [key,known] of [['capsule',capsule],['stop',expected.stop],['manifest',expected.manifest]]){const physical=key!=='manifest';exact(value[key],physical?['sha256','bytes','materialSha256']:['sha256','bytes'],key);need(canonical(value[key])===canonical(pin(known,physical)),key+' external binding');}
 need(value.witnessOriginSha256===expected.witnessOriginSha256&&sha.test(value.witnessOriginSha256),'independent witness origin');
 for(const key of ['corePrimaryCommitmentSha256','coreWitnessCommitmentSha256','selectedPrimaryCommitmentSha256','selectedWitnessCommitmentSha256'])need(sha.test(value[key]),'commitment digest');
 need(value.corePrimaryCommitmentSha256!==value.coreWitnessCommitmentSha256&&value.selectedPrimaryCommitmentSha256!==value.selectedWitnessCommitmentSha256,'independent primary/witness roles');
 need(value.primaryWitnessPairs===33&&value.readOnlyDatabaseRoles===68&&value.indexFromAuthenticatedCapturedRegistry===true&&value.sourceFilesUnchanged===true&&value.newSignaturesCreated===0,'read-only captured role scope');
 for(const key of ['rawCapsuleExported','snapshotBytesExported','signingRootExported','privatePathsExported','runtimeStarted','productionQualified'])need(value[key]===false,'no extra authority or raw export');return value;
}
export function validateVpsDutyFloorPair(value,expected){
 need(plain(expected)&&sha.test(expected.campaignId)&&pub.test(expected.selectedPublicKey)&&typeof expected.selectedIsPending==='boolean'&&Number.isSafeInteger(expected.expectedHeadSlot)&&expected.expectedHeadSlot>=0,'independent request binding');
 exact(value,['format','passed','source','restored','sameIdentityFloorAndCommitments','sourceFilesUnchanged','rawCapsulesExported','newSignaturesCreated','runtimeStarted','productionQualified'],'pair');
 need(value.format==='nano-fresh-vps-duty-floor-pair-public-v1'&&value.passed===true&&value.sameIdentityFloorAndCommitments===true&&value.sourceFilesUnchanged===true&&value.rawCapsulesExported===false&&value.newSignaturesCreated===0&&value.runtimeStarted===false&&value.productionQualified===false,'pair scope');
 validateProjection(value.source,expected,expected.sourceCapture);validateProjection(value.restored,expected,expected.restoredCapture);
 const comparable=row=>Object.fromEntries(Object.entries(row).filter(([k])=>k!=='capsule'));
 need(canonical(comparable(value.source))===canonical(comparable(value.restored)),'source/copy identity, floor or commitments differ');return structuredClone(value);
}

// Executed only by lifecycleOwnerProjection ON THE VPS. Existing read() provides
// owner/mode/NOFOLLOW/full-nine/SHA/bytes checks; no capsule is returned to Mac.
export const vpsDutyFloorProjectionPython=String.raw`
def vps_floor_project_pair(request):
 keys={'mode','sourceCapture','restoredCapture','stop','manifest','campaignId','selectedPublicKey','selectedPreparedPublicKeyIndex','selectedIsPending','resolvedIndex','expectedHeadSlot','expectedFinalized','witnessOriginSha256'}
 need(type(request) is dict and set(request)==keys and request['mode']=='vps-duty-floor-pair')
 def obj(v,keys):need(type(v) is dict and set(v)==set(keys));return v
 def digest(v):need(type(v) is str and re.fullmatch('[0-9a-f]{64}',v));return v
 def pk(v):need(type(v) is str and re.fullmatch('0x[0-9a-f]{96}',v));return v
 def quantity(v):
  need(type(v) is str and re.fullmatch('0x(?:0|[1-9a-f][0-9a-f]*)',v));n=int(v,16);need(0<=n<18446744073709551616);return n
 def token(v):obj(v,('sequence','digest'));need(quantity(v['sequence'])>0 and type(v['digest']) is str and re.fullmatch('0x[0-9a-f]{64}',v['digest']));return v
 def unique(pairs):
  out={}
  for k,v in pairs:need(k not in out);out[k]=v
  return out
 def invalid_constant(_):raise ValueError('nonfinite JSON')
 def document(raw):
  v=json.loads(raw,object_pairs_hook=unique,parse_constant=invalid_constant)
  def bound(x,depth=0):
   need(depth<=40)
   if type(x) is dict:
    for y in x.values():bound(y,depth+1)
   elif type(x) is list:
    for y in x:bound(y,depth+1)
  bound(v);return v
 def artifact(v,physical):
  obj(v,('path','sha256','bytes','materialSha256') if physical else ('path','sha256','bytes'));digest(v['sha256']);need(type(v['bytes']) is int and 0<v['bytes']<=16777216)
  if physical:digest(v['materialSha256'])
  return v
 need(type(request['selectedPreparedPublicKeyIndex']) is int and request['selectedPreparedPublicKeyIndex']>=0 and type(request['selectedIsPending']) is bool)
 need(type(request['expectedHeadSlot']) is int and 0<=request['expectedHeadSlot']<=9007199254740991)
 campaign=digest(request['campaignId']);selected=pk(request['selectedPublicKey']);selected_index=request['selectedPreparedPublicKeyIndex'];resolved=quantity(request['resolvedIndex']);need(resolved==selected_index)
 origin=digest(request['witnessOriginSha256']);known={key:artifact(request[key],key!='manifest') for key in ('sourceCapture','restoredCapture','stop','manifest')}
 need(len({z['path'] for z in known.values()})==4)
 held={};docs={}
 for key,z in known.items():
  raw,row=read(z['path'],z,key!='manifest');held[key]=row;docs[key]=document(raw)
 need(len({tuple(z['material'][:2]) for z in held.values()})==4)
 manifest=docs['manifest'];need(type(manifest) is dict and manifest['format']=='nano-isolated-capella-fixture-domain-v1' and manifest['campaignId']==campaign)
 identities=manifest['publicKeys'];need(type(identities) is list and len(identities)>0 and type(manifest['registryCount']) is int and 0<=manifest['registryCount']<len(identities))
 seen=set()
 for index,row in enumerate(identities):obj(row,('index','publicKey'));need(type(row['index']) is int and row['index']==index and pk(row['publicKey']) not in seen);seen.add(row['publicKey'])
 pending=manifest['bootstrapSigningHistory']['deposit']['publicKey'];need(pk(pending)==identities[manifest['registryCount']]['publicKey'] and selected_index<len(identities) and identities[selected_index]['publicKey']==selected and request['selectedIsPending']==(selected==pending))
 stop=docs['stop'];need(type(stop) is dict and stop['event']=='capella-runtime-stopped' and stop['clockHealthy'] is True and stop['validator']['healthy'] is True and stop['validator']['failure'] is None and type(stop['validator']['localKeys']) is int and stop['validator']['localKeys']==33)
 token(stop['restart']);witness=obj(stop['witnessCommitments'],('format','sourceCaptureSha256','core','validatorKeys'));need(witness['format']=='nano-isolated-stopped-external-witness-commitments-v1' and witness['sourceCaptureSha256']==origin);token(witness['core'])
 def stop_rows(rows):
  need(type(rows) is list and len(rows)==33);mapped={};pending_seen=False
  for row in rows:
   need(type(row) is dict)
   if row['index'] is None:
    obj(row,('index','publicKey','restart'));need(row['publicKey']==pending and not pending_seen);pending_seen=True;index=manifest['registryCount']
   else:obj(row,('index','restart'));index=quantity(row['index']);need(index!=manifest['registryCount'])
   need(index not in mapped and index<len(identities));token(row['restart']);mapped[index]=row
  return mapped
 stop_keys=stop_rows(stop['validatorKeys']);witness_keys=stop_rows(witness['validatorKeys']);need(set(stop_keys)==set(witness_keys))
 def one(capture,key):
  need(type(capture) is dict and capture['format']=='nano-isolated-stopped-recovery-pins-v1' and capture['passed'] is True and capture['owner']=='vps-docker' and capture['campaignId']==campaign and capture['manifestSha256']==known['manifest']['sha256'] and capture['externalStopReceiptSha256']==known['stop']['sha256'])
  for flag in ('canonicalStoreSnapshotVerified','primaryAndWitnessBlobClosuresVerified','allObservedFileBytesAndInodesUnchanged','observedWitnessPairsAuthenticated','externalStopTokensMatched','externalWitnessTokensMatched','externalWitnessPinOriginExternallyRequired'):need(capture[flag] is True)
  for flag in ('privateKeysRead','envRead','journalWritesRequested','runtimeStarted','networkUsed','productionQualified'):need(capture[flag] is False)
  need(type(capture['signerCallbacksInvoked']) is int and capture['signerCallbacksInvoked']==0 and type(capture['newSignaturesCreated']) is int and capture['newSignaturesCreated']==0 and type(capture['primaryWitnessPairs']) is int and capture['primaryWitnessPairs']==33 and type(capture['databasesReadOnly']) is int and capture['databasesReadOnly']==68)
  need(type(capture['headSlot']) is int and 0<=capture['headSlot']<=9007199254740991 and capture['headSlot']==request['expectedHeadSlot'] and capture['finalized']==stop['finalized']==request['expectedFinalized'])
  core=capture['core'];need(token(capture['restart'])==token(core['primary'])==token(core['restart'])==stop['restart'] and token(capture['recoveryWitnessCommitment'])==token(core['witness'])==witness['core'] and token(core['witnessNext'])==core['primary'] and core['primary']['digest']!=core['witness']['digest'])
  rows=capture['validatorKeys'];need(type(rows) is list and len(rows)==33);by_pub={};by_index={}
  for row in rows:
   index=row['publicKeyIndex'];public=pk(row['publicKey']);need(type(index) is int and 0<=index<len(identities) and identities[index]['publicKey']==public and public not in by_pub and index not in by_index and index in stop_keys and index in witness_keys)
   need((row['index'] is None and public==pending) or (row['index'] is not None and public!=pending and quantity(row['index'])==index))
   need(token(row['primary'])==token(row['restart'])==stop_keys[index]['restart'] and token(row['witness'])==witness_keys[index]['restart'] and token(row['witnessNext'])==row['primary'] and row['primary']['digest']!=row['witness']['digest'])
   by_pub[public]=row;by_index[index]=row
  need(set(by_index)==set(stop_keys) and selected in by_pub);row=by_pub[selected];need(type(capture['registryCount']) is int and resolved<capture['registryCount']<=len(identities) and row['publicKeyIndex']==resolved and row['registeredInCurrentHead'] is True and row['retired'] is False)
  need(type(row['snapshot']) is str and type(row['witnessSnapshot']) is str and row['primary']['digest']=='0x'+sha(row['snapshot'].encode('utf-8')) and row['witness']['digest']=='0x'+sha(row['witnessSnapshot'].encode('utf-8')))
  snapshot=document(row['snapshot']);need(snapshot['format'] in ('nano-beacon-signing-v1','nano-beacon-signing-v2','nano-beacon-signing-v3') and snapshot['publicKey']==selected and snapshot['genesisValidatorsRoot']==manifest['genesisValidatorsRoot'] and snapshot['proposal']==row['proposal'])
  intent=document(row['witnessSnapshot']);need(intent['format']=='nano-beacon-signing-witness-v1' and token(intent['next'])==row['primary'] and intent['snapshot']==row['snapshot'])
  proposal=row['proposal'];slot=None
  if proposal is not None:
   obj(proposal,('slot','root'));slot=quantity(proposal['slot']);need(proposal['root'] is None or type(proposal['root']) is str and re.fullmatch('0x[0-9a-f]{64}',proposal['root']))
  floor=max(capture['headSlot'],slot if slot is not None else 0)
  return {'format':'nano-fresh-vps-duty-floor-public-v1','owner':'vps-docker','campaignId':campaign,'selectedPublicKey':selected,'selectedPreparedPublicKeyIndex':selected_index,'selectedResolvedIndex':hex(resolved),'selectedProvisionedIndex':None if row['index'] is None else hex(quantity(row['index'])),'headSlot':hex(capture['headSlot']),'journalProposalSlot':None if slot is None else hex(slot),'cutoffSlot':hex(floor),'capsule':{k:known[key][k] for k in ('sha256','bytes','materialSha256')},'stop':{k:known['stop'][k] for k in ('sha256','bytes','materialSha256')},'manifest':{k:known['manifest'][k] for k in ('sha256','bytes')},'witnessOriginSha256':origin,'corePrimaryCommitmentSha256':sha(canon(core['primary'])),'coreWitnessCommitmentSha256':sha(canon(core['witness'])),'selectedPrimaryCommitmentSha256':sha(canon(row['primary'])),'selectedWitnessCommitmentSha256':sha(canon(row['witness'])),'primaryWitnessPairs':33,'readOnlyDatabaseRoles':68,'indexFromAuthenticatedCapturedRegistry':True,'sourceFilesUnchanged':True,'newSignaturesCreated':0,'rawCapsuleExported':False,'snapshotBytesExported':False,'signingRootExported':False,'privatePathsExported':False,'runtimeStarted':False,'productionQualified':False}
 source=one(docs['sourceCapture'],'sourceCapture');restored=one(docs['restoredCapture'],'restoredCapture');need({k:v for k,v in source.items() if k!='capsule'}=={k:v for k,v in restored.items() if k!='capsule'})
 for key,row in held.items():read(row['path'],row,key!='manifest')
 return {'format':'nano-fresh-vps-duty-floor-pair-public-v1','passed':True,'source':source,'restored':restored,'sameIdentityFloorAndCommitments':True,'sourceFilesUnchanged':True,'rawCapsulesExported':False,'newSignaturesCreated':0,'runtimeStarted':False,'productionQualified':False}
`;
