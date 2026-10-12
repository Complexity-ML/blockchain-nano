// Pure public-artifact identity verifier. No filesystem, secret, signer or network API.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
const ZERO=Buffer.alloc(32),MAX_BYTES=16*1024*1024,U64=(1n<<64n)-1n;
const sha=b=>createHash('sha256').update(b).digest();
const pair=(a,b)=>sha(Buffer.concat([a,b]));
const hex=b=>'0x'+b.toString('hex');
const quantity=s=>{assert.equal(typeof s,'string');assert.match(s,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);const n=BigInt(s);assert.ok(n<=U64);return n;};
const scalar=b=>{assert.ok(b.length<=32);const out=Buffer.alloc(32);b.copy(out);return out;};
const uintRoot=n=>{assert.ok(n>=0n&&n<(1n<<256n));const out=Buffer.alloc(32);for(let i=0;i<32;i++){out[i]=Number(n&255n);n>>=8n;}return out;};
const rootBytes=s=>{assert.equal(typeof s,'string');assert.match(s,/^0x[0-9a-f]{64}$/);return Buffer.from(s.slice(2),'hex');};
const hashText=s=>{assert.equal(typeof s,'string');assert.match(s,/^[0-9a-f]{64}$/);};
const publicKey=s=>{assert.equal(typeof s,'string');assert.match(s,/^0x[0-9a-f]{96}$/);return Buffer.from(s.slice(2),'hex');};
const shape=(v,keys,label)=>{assert.ok(v&&typeof v==='object'&&!Array.isArray(v),label);assert.deepEqual(Object.keys(v).sort(),[...keys].sort(),label+' fields');};
const bytes=b=>{assert.ok(Buffer.isBuffer(b));assert.ok(b.length<=MAX_BYTES);return b;};
export const canonicalPublicJson=v=>Array.isArray(v)?'['+v.map(canonicalPublicJson).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonicalPublicJson(v[k])).join(',')+'}':JSON.stringify(v);
export const publicSha256=b=>sha(bytes(b)).toString('hex');
function merkle(nodes,limit){
 assert.ok(typeof limit==='bigint'&&limit>0n&&BigInt(nodes.length)<=limit);
 let width=1n,depth=0;while(width<limit){width*=2n;depth++;}
 // Populate only actual leaves; the right hand empty subtree is one hash per level.
 let live=nodes.length?[...nodes]:[ZERO],empty=ZERO;
 for(let level=0;level<depth;level++){
  if(live.length%2)live.push(empty);
  const parents=[];for(let i=0;i<live.length;i+=2)parents.push(pair(live[i],live[i+1]));
  live=parents;empty=pair(empty,empty);
 }
 assert.equal(live.length,1);return live[0];
}
const container=roots=>merkle(roots,BigInt(roots.length));
function packed(b,limit){const chunks=[];for(let i=0;i<b.length;i+=32)chunks.push(scalar(b.subarray(i,i+32)));return merkle(chunks,limit);}
const mix=(root,length)=>pair(root,uintRoot(BigInt(length)));
function validatorRoot(b){
 assert.equal(b.length,121);assert.ok(b[88]<=1,'SSZ boolean must be 0 or 1');
 return container([packed(b.subarray(0,48),2n),b.subarray(48,80),scalar(b.subarray(80,88)),scalar(b.subarray(88,89)),...Array.from({length:4},(_,i)=>scalar(b.subarray(89+8*i,97+8*i)))]);
}
export function validatorRegistryRoot(encoded,limit){
 const b=bytes(encoded);assert.equal(b.length%121,0,'Validator list byte width');assert.ok(typeof limit==='bigint'&&limit>0n&&limit<=U64);
 const leaves=[];for(let at=0;at<b.length;at+=121)leaves.push(validatorRoot(b.subarray(at,at+121)));
 return hex(mix(merkle(leaves,limit),leaves.length));
}
function profileParameters(profile){
 shape(profile,['format','common','withdrawals','genesisVersion','maximumBlsChanges'],'Capella profile');assert.equal(profile.format,'nano-capella-profile-v1');
 const c=profile.common;assert.ok(Array.isArray(c)&&[12,13,14].includes(c.length));
 const widths={attestation:7,bodyLimits:6,deposit:3,duties:7,effective:5,epochSync:5,executionLimits:1,exit:5,ffg:2,history:4,inactivity:4,registry:6,rewardWeights:4,rewards:6,slashing:10,stateLimits:7,syncAggregate:9};
 shape(c[0],[...Object.keys(widths),'genesisForkVersion'],'Common fork profile');
 for(const [name,width] of Object.entries(widths)){assert.ok(Array.isArray(c[0][name])&&c[0][name].length===width);for(const value of c[0][name])quantity(value);}
 assert.match(c[0].genesisForkVersion,/^0x[0-9a-f]{8}$/);assert.match(profile.genesisVersion,/^0x[0-9a-f]{8}$/);
 assert.ok(Array.isArray(profile.withdrawals)&&profile.withdrawals.length===3);for(const value of profile.withdrawals)quantity(value);assert.ok(quantity(profile.maximumBlsChanges)>0n);
 for(let i=1;i<12;i++)quantity(c[i]);if(c.length>=13)rootBytes(c[12]);if(c.length===14)assert.equal(c[13],'nano-native-eth1-v1');
 const l=c[0].stateLimits;assert.ok(Array.isArray(l)&&l.length===7);const [history,randao,slashings,historicalLimit,votesLimit,registryLimit,sync]=l.map(quantity);
 assert.ok([history,randao,slashings,historicalLimit,votesLimit,registryLimit,sync].every(v=>v>0n));
 assert.ok(Array.isArray(c[0].ffg)&&c[0].ffg.length===2);const slots=quantity(c[0].ffg[0]);assert.ok(slots>0n);
 return {history,randao,slashings,historicalLimit,votesLimit,registryLimit,sync,slots,maximumBytes:quantity(c[8]),extraDataLimit:quantity(c[9]),maxTransactionBytes:quantity(c[10]),maxTransactions:quantity(c[11]),maxWithdrawals:quantity(profile.withdrawals[1]),bodyLimits:c[0].bodyLimits.map(quantity),maxBlsChanges:quantity(profile.maximumBlsChanges),slotDurationMs:quantity(c[1]),chainId:c[5]};
}
const headerRoot=b=>{assert.equal(b.length,112);return container([scalar(b.subarray(0,8)),scalar(b.subarray(8,16)),b.subarray(16,48),b.subarray(48,80),b.subarray(80,112)]);};
const checkpointRoot=b=>{assert.equal(b.length,40);return container([scalar(b.subarray(0,8)),b.subarray(8,40)]);};
const eth1Root=b=>{assert.equal(b.length,72);return container([b.subarray(0,32),scalar(b.subarray(32,40)),b.subarray(40,72)]);};
function compositeList(b,width,limit,hash){assert.equal(b.length%width,0);const roots=[];for(let i=0;i<b.length;i+=width)roots.push(hash(b.subarray(i,i+width)));return mix(merkle(roots,limit),roots.length);}
function basicList(b,width,limit){assert.equal(b.length%width,0);const count=b.length/width;assert.ok(BigInt(count)<=limit);return mix(packed(b,(limit*BigInt(width)+31n)/32n),count);}
export function capellaExecutionHeaderRoot(encoded,extraLimit=32n){
 const b=bytes(encoded);assert.ok(b.length>=568&&b.readUInt32LE(436)===568,'Capella execution header offset');assert.ok(BigInt(b.length-568)<=extraLimit);
 const roots=[b.subarray(0,32),scalar(b.subarray(32,52)),b.subarray(52,84),b.subarray(84,116),packed(b.subarray(116,372),8n),b.subarray(372,404)];
 for(let at=404;at<436;at+=8)roots.push(scalar(b.subarray(at,at+8)));
 roots.push(mix(packed(b.subarray(568),(extraLimit+31n)/32n),b.length-568),b.subarray(440,472),b.subarray(472,504),b.subarray(504,536),b.subarray(536,568));
 return hex(container(roots));
}
export function inspectCapellaState(encoded,profile){
 const b=bytes(encoded),p=profileParameters(profile);assert.ok(BigInt(b.length)<=p.maximumBytes);
 const fixedSize=176n+64n*p.history+96n+32n*p.randao+8n*p.slashings+253n+96n*p.sync;
 assert.ok(fixedSize<=BigInt(b.length),'Capella fixed section truncated');
 const hist=176+64*Number(p.history),randao=hist+96,prior=randao+32*Number(p.randao)+8*Number(p.slashings),executionAt=prior+229+96*Number(p.sync);
 const offsetPositions=[hist,hist+76,hist+88,hist+92,prior,prior+4,prior+129,executionAt,executionAt+20];
 const offsets=offsetPositions.map(at=>b.readUInt32LE(at));assert.equal(offsets[0],Number(fixedSize),'Capella first offset');
 assert.ok(offsets.every((v,i)=>v<=b.length&&(i===0||v>=offsets[i-1])),'Capella offsets order');
 const variable=offsets.map((v,i)=>b.subarray(v,i+1<offsets.length?offsets[i+1]:b.length));
 const [history,votes,registry,balances,previous,current,scores,execution,summaries]=variable;
 assert.equal(registry.length%121,0);const count=registry.length/121;assert.ok(BigInt(count)<=p.registryLimit);
 assert.equal(balances.length%8,0);assert.equal(scores.length%8,0);
 assert.ok(b[prior+8]<=15,'SSZ justification bitvector');
 const roots=[scalar(b.subarray(0,8)),b.subarray(8,40),scalar(b.subarray(40,48)),container([scalar(b.subarray(48,52)),scalar(b.subarray(52,56)),scalar(b.subarray(56,64))]),headerRoot(b.subarray(64,176)),packed(b.subarray(176,hist-32*Number(p.history)),p.history),packed(b.subarray(hist-32*Number(p.history),hist),p.history)];
 roots.push(compositeList(history,32,p.historicalLimit,x=>x),eth1Root(b.subarray(hist+4,hist+76)),compositeList(votes,72,p.votesLimit,eth1Root),scalar(b.subarray(hist+80,hist+88)),rootBytes(validatorRegistryRoot(registry,p.registryLimit)),basicList(balances,8,p.registryLimit),packed(b.subarray(randao,prior-8*Number(p.slashings)),p.randao),packed(b.subarray(prior-8*Number(p.slashings),prior),(p.slashings+3n)/4n),basicList(previous,1,p.registryLimit),basicList(current,1,p.registryLimit),scalar(b.subarray(prior+8,prior+9)));
 for(let at=prior+9;at<prior+129;at+=40)roots.push(checkpointRoot(b.subarray(at,at+40)));
 roots.push(basicList(scores,8,p.registryLimit));
 const committees=[];
 for(let c=0;c<2;c++){
  const at=prior+133+c*48*(Number(p.sync)+1),keys=[];for(let i=0;i<Number(p.sync);i++)keys.push(b.subarray(at+48*i,at+48*(i+1)));
  const aggregate=b.subarray(at+48*Number(p.sync),at+48*(Number(p.sync)+1));
  roots.push(container([merkle(keys.map(k=>packed(k,2n)),p.sync),packed(aggregate,2n)]));committees.push(keys.map(hex));
 }
 roots.push(rootBytes(capellaExecutionHeaderRoot(execution,p.extraDataLimit)),scalar(b.subarray(executionAt+4,executionAt+12)),scalar(b.subarray(executionAt+12,executionAt+20)),compositeList(summaries,64,p.historicalLimit,x=>container([x.subarray(0,32),x.subarray(32,64)])));
 assert.equal(roots.length,28);
 return {stateRoot:hex(container(roots)),genesisValidatorsRoot:hex(b.subarray(8,40)),registryRoot:validatorRegistryRoot(registry,p.registryLimit),registry:Buffer.from(registry),registryCount:count,registryLimit:p.registryLimit,slot:b.readBigUInt64LE(40),genesisTime:b.readBigUInt64LE(0),executionGenesisHash:hex(execution.subarray(472,504)),executionHeaderRoot:capellaExecutionHeaderRoot(execution,p.extraDataLimit),committees,latestHeader:Buffer.from(b.subarray(64,176)),relatedListCounts:[balances.length/8,previous.length,current.length,scores.length/8]};
}
function variableSlices(b,positions,fixed,label){
 assert.ok(b.length>=fixed,label+' fixed section');const offsets=positions.map(at=>b.readUInt32LE(at));assert.equal(offsets[0],fixed,label+' first offset');assert.ok(offsets.every((n,i)=>n<=b.length&&(!i||n>=offsets[i-1])),label+' offsets order');return offsets.map((n,i)=>b.subarray(n,i+1<offsets.length?offsets[i+1]:b.length));
}
function variableListRoot(b,limit,itemRoot){
 if(!b.length)return mix(merkle([],limit),0);assert.ok(b.length>=4);const first=b.readUInt32LE(0);assert.ok(first>=4&&first%4===0&&first<=b.length,'SSZ variable list first offset');const count=first/4;assert.ok(BigInt(count)<=limit);const items=variableSlices(b,Array.from({length:count},(_,i)=>4*i),first,'SSZ variable list');return mix(merkle(items.map(itemRoot),limit),count);
}
function executionPayloadRoot(b,p){
 const [extra,transactions,withdrawals]=variableSlices(bytes(b),[436,504,508],512,'Capella execution payload');assert.ok(BigInt(extra.length)<=p.extraDataLimit);
 const roots=[b.subarray(0,32),scalar(b.subarray(32,52)),b.subarray(52,84),b.subarray(84,116),packed(b.subarray(116,372),8n),b.subarray(372,404)];for(let at=404;at<436;at+=8)roots.push(scalar(b.subarray(at,at+8)));
 roots.push(mix(packed(extra,(p.extraDataLimit+31n)/32n),extra.length),b.subarray(440,472),b.subarray(472,504),variableListRoot(transactions,p.maxTransactions,x=>{assert.ok(BigInt(x.length)<=p.maxTransactionBytes);return mix(packed(x,(p.maxTransactionBytes+31n)/32n),x.length);}),compositeList(withdrawals,44,p.maxWithdrawals,x=>container([scalar(x.subarray(0,8)),scalar(x.subarray(8,16)),scalar(x.subarray(16,36)),scalar(x.subarray(36,44))])));
 return container(roots);
}
export const capellaExecutionPayloadRoot=(encoded,profile)=>hex(executionPayloadRoot(bytes(encoded),profileParameters(profile)));
const blsChangeRoot=b=>{assert.equal(b.length,76);return container([scalar(b.subarray(0,8)),packed(b.subarray(8,56),2n),scalar(b.subarray(56,76))]);};
// This is deliberately the clean bootstrap subset: five empty operation lists,
// no sync votes, and one BLS-to-execution change. It is not a general block validator.
export function inspectCapellaBootstrapBlock(encoded,profile){
 const wire=bytes(encoded),p=profileParameters(profile);assert.ok(wire.length>=184&&wire.readUInt32LE(0)===100,'Signed bootstrap block offset');const message=wire.subarray(100);assert.equal(message.readUInt32LE(80),84,'Bootstrap message body offset');const body=message.subarray(84),syncBytes=Number((p.sync+7n)/8n),fixed=324+syncBytes;
 const operations=variableSlices(body,[200,204,208,212,216,316+syncBytes,320+syncBytes],fixed,'Bootstrap body');for(let i=0;i<5;i++)assert.equal(operations[i].length,0,'Undeclared bootstrap operation');assert.equal(operations[6].length,172,'Bootstrap must have one BLS change');assert.ok(body.subarray(220,220+syncBytes).every(n=>n===0),'Undeclared bootstrap sync votes');
 const change=operations[6],syncSignature=body.subarray(220+syncBytes,316+syncBytes);assert.equal(syncSignature[0],0xc0,'Empty sync signature must use infinity encoding');assert.ok(syncSignature.subarray(1).every(n=>n===0),'Empty sync signature encoding');
 const roots=[packed(body.subarray(0,96),3n),eth1Root(body.subarray(96,168)),body.subarray(168,200),...p.bodyLimits.slice(0,5).map(limit=>mix(merkle([],limit),0)),container([packed(body.subarray(220,220+syncBytes),(p.sync+255n)/256n),packed(syncSignature,3n)]),executionPayloadRoot(operations[5],p),mix(merkle([container([blsChangeRoot(change.subarray(0,76)),packed(change.subarray(76),3n)])],p.maxBlsChanges),1)];
 const payloadRoot=hex(roots[9]),bodyRoot=container(roots),header=Buffer.concat([message.subarray(0,80),bodyRoot]);return {root:hex(headerRoot(header)),header,signature:hex(wire.subarray(4,100)),randaoSignature:hex(body.subarray(0,96)),change:Buffer.from(change),bodyRoot:hex(bodyRoot),executionPayloadRoot:payloadRoot};
}
const signature=s=>{assert.equal(typeof s,'string');assert.match(s,/^0x[0-9a-f]{192}$/);return Buffer.from(s.slice(2),'hex');};
const signingRoot=(objectRoot,type,version,genesisRoot)=>{const fork=container([scalar(Buffer.from(version.slice(2),'hex')),rootBytes(genesisRoot)]),domain=Buffer.concat([Buffer.from([type,0,0,0]),fork.subarray(0,28)]);return hex(pair(objectRoot,domain));};
function verifyBootstrap(m,initial,before,after,header,profile,p){
 shape(m.bootstrapDutyPolicy,['anchorSlot','maximumPresignedProposalSlot','minimumNewProposalSlot','historicalProposalSigningAllowed','requiresClockAtOrAfterAnchor','driverMayRegeneratePresignedHistory'],'Bootstrap duty policy');assert.deepEqual(m.bootstrapDutyPolicy,{anchorSlot:1,maximumPresignedProposalSlot:1,minimumNewProposalSlot:2,historicalProposalSigningAllowed:false,requiresClockAtOrAfterAnchor:true,driverMayRegeneratePresignedHistory:false});
 const history=m.bootstrapSigningHistory;shape(history,['block','randao','blsToExecutionChange','deposit','attestations','syncMessages'],'Bootstrap history');assert.deepEqual(history.attestations,[]);assert.deepEqual(history.syncMessages,[]);
 shape(history.block,['slot','validatorIndex','signingRoot','signature','ssz'],'Bootstrap block');shape(history.randao,['epoch','validatorIndex','signingRoot','signature'],'Bootstrap randao');shape(history.blsToExecutionChange,['validatorIndex','publicKey','signingRoot','signature'],'Bootstrap change');shape(history.deposit,['validatorIndex','publicKey','signingRoot','signature','withdrawalCredentials','amountGwei'],'Bootstrap deposit');
 const parent=Buffer.from(before.latestHeader);if(parent.subarray(48,80).equals(ZERO))rootBytes(before.stateRoot).copy(parent,48);assert.equal(hex(headerRoot(parent)),hex(header.subarray(16,48)),'Bootstrap parent differs from genesis anchor');
 const block=history.block,randao=history.randao,change=history.blsToExecutionChange,deposit=history.deposit;
 assert.equal(block.slot,1);assert.equal(after.slot,1n);assert.ok(Number.isInteger(block.validatorIndex)&&block.validatorIndex>=0&&block.validatorIndex<64);assert.equal(BigInt(block.validatorIndex),header.readBigUInt64LE(8));assert.equal(randao.epoch,0);assert.equal(randao.validatorIndex,block.validatorIndex);assert.equal(change.validatorIndex,0);assert.equal(change.publicKey,m.withdrawalPublicKeys[0].publicKey);assert.equal(deposit.validatorIndex,64);assert.equal(deposit.publicKey,m.publicKeys[64].publicKey);assert.ok(Number.isSafeInteger(deposit.amountGwei)&&BigInt(deposit.amountGwei)===quantity(profile.common[0].deposit[1]));rootBytes(deposit.withdrawalCredentials);
 for(const item of [block,randao,change,deposit]){signature(item.signature);rootBytes(item.signingRoot);}assert.equal(typeof block.ssz,'string');assert.ok(block.ssz.length<=MAX_BYTES*2+2);assert.match(block.ssz,/^0x(?:[0-9a-f]{2})+$/);
 const parsed=inspectCapellaBootstrapBlock(Buffer.from(block.ssz.slice(2),'hex'),profile);assert.ok(parsed.header.equals(header),'Presigned bootstrap differs from initial header');assert.equal(parsed.root,m.initialHeadRoot);assert.equal(parsed.executionPayloadRoot,after.executionHeaderRoot,'Bootstrap execution payload differs from post-state execution header');assert.equal(parsed.signature,block.signature);assert.equal(parsed.randaoSignature,randao.signature);assert.equal(parsed.change.readBigUInt64LE(0),0n);assert.equal(hex(parsed.change.subarray(8,56)),change.publicKey);assert.equal(hex(parsed.change.subarray(76)),change.signature);assert.ok(after.registry.subarray(48,80).equals(Buffer.concat([Buffer.from([1]),Buffer.alloc(11),parsed.change.subarray(56,76)])),'Post-transition changed withdrawal credential differs');
 const state=before.bytes,version=hex(state.subarray(0n<state.readBigUInt64LE(56)?48:52,0n<state.readBigUInt64LE(56)?52:56));
 assert.equal(block.signingRoot,signingRoot(rootBytes(m.initialHeadRoot),0,version,m.genesisValidatorsRoot));assert.equal(randao.signingRoot,signingRoot(uintRoot(0n),2,version,m.genesisValidatorsRoot));assert.equal(change.signingRoot,signingRoot(blsChangeRoot(parsed.change.subarray(0,76)),10,profile.genesisVersion,m.genesisValidatorsRoot));
 const depositRoot=container([packed(publicKey(deposit.publicKey),2n),rootBytes(deposit.withdrawalCredentials),uintRoot(BigInt(deposit.amountGwei))]);assert.equal(deposit.signingRoot,signingRoot(depositRoot,3,profile.genesisVersion,hex(ZERO)));
 assert.ok(Array.isArray(initial.clock)&&initial.clock.length===4);assert.equal(quantity(initial.clock[0]),before.genesisTime*1000n);assert.ok(quantity(initial.clock[1])>=before.genesisTime*1000n+p.slotDurationMs,'Initial clock precedes its signed anchor');
}
function parsePublicJson(b){
 const text=bytes(b).toString('utf8');assert.ok(Buffer.from(text,'utf8').equals(b),'JSON must be UTF-8');const parsed=JSON.parse(text),tokens=text.match(/"(?:\\.|[^"\\])*"|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?|true|false|null|[{}\[\]:,]/g);let i=0;
 function value(){const t=tokens[i++];if(t==='{'){const seen=new Set();if(tokens[i]==='}'){i++;return;}for(;;){const key=JSON.parse(tokens[i++]);assert.equal(typeof key,'string');assert.ok(!seen.has(key),'Duplicate JSON field');seen.add(key);assert.equal(tokens[i++],':');value();const next=tokens[i++];if(next==='}')break;assert.equal(next,',');}}else if(t==='['){if(tokens[i]===']'){i++;return;}for(;;){value();const next=tokens[i++];if(next===']')break;assert.equal(next,',');}}}
 value();assert.equal(i,tokens.length);return parsed;
}
const FILES={genesisValidators:'genesis-validators.ssz',genesisState:'genesis-state.ssz',genesis:'genesis.json',profile:'profile.json',initial:'initial.json'};
export function verifyIsolatedFixtureDomain({manifest:m,artifacts,legacy}){
 shape(m,['format','synthetic','productionQualified','fork','chainId','campaignId','keySetSha256','genesisValidatorsRoot','genesisStateRoot','executionGenesisHash','initialHeadRoot','initialStateRoot','registryLimit','registryCount','publicKeys','withdrawalPublicKeys','keyDerivation','files','legacyKeyDisjointnessVerified','reservation','transactionSender','bootstrapSigningHistory','bootstrapDutyPolicy'],'Isolated domain');
 assert.equal(m.format,'nano-isolated-capella-fixture-domain-v1');assert.equal(m.synthetic,true);assert.equal(m.productionQualified,false);assert.equal(m.fork,'Capella/Shanghai');assert.equal(m.chainId,'0x539');
 for(const key of ['campaignId','keySetSha256'])hashText(m[key]);for(const key of ['genesisValidatorsRoot','genesisStateRoot','executionGenesisHash','initialHeadRoot','initialStateRoot'])rootBytes(m[key]);
 assert.equal(m.registryCount,64);assert.ok(Number.isSafeInteger(m.registryLimit)&&m.registryLimit>0);
 shape(m.keyDerivation,['algorithm','infoPrefix','infoEncoding','validatorRole','withdrawalRole','senderRole'],'Public derivation');
 assert.deepEqual(m.keyDerivation,{algorithm:'blst-keygen',infoPrefix:'Nano/Test/IsolatedCapella/v1',infoEncoding:'tag-bytes||role-u8||index-u64-be',validatorRole:0,withdrawalRole:1,senderRole:2});
 shape(m.transactionSender,['address','publicKey'],'Transaction sender');assert.match(m.transactionSender.address,/^0x[0-9a-f]{40}$/);assert.match(m.transactionSender.publicKey,/^0x[0-9a-f]{128}$/);
 shape(legacy,['genesisValidatorsRoot','publicKeys','transactionSenderAddress'],'Legacy public identity');rootBytes(legacy.genesisValidatorsRoot);assert.match(legacy.transactionSenderAddress,/^0x[0-9a-f]{40}$/);
 assert.ok(Array.isArray(legacy.publicKeys)&&legacy.publicKeys.length===65);for(const k of legacy.publicKeys)publicKey(k);assert.equal(new Set(legacy.publicKeys).size,65);
 assert.notEqual(m.genesisValidatorsRoot,legacy.genesisValidatorsRoot);assert.notEqual(m.transactionSender.address,legacy.transactionSenderAddress);
 const seen=new Set(),previous=new Set(legacy.publicKeys);
 for(const [name,count] of [['publicKeys',65],['withdrawalPublicKeys',64]]){
  assert.ok(Array.isArray(m[name])&&m[name].length===count);
  for(const [i,k] of m[name].entries()){shape(k,['index','publicKey'],name+' member');assert.equal(k.index,i);publicKey(k.publicKey);assert.ok(!seen.has(k.publicKey)&&!previous.has(k.publicKey),'Public key reused across roles or legacy domain');seen.add(k.publicKey);}
 }
 assert.equal(m.legacyKeyDisjointnessVerified,true);
 assert.equal(m.keySetSha256,publicSha256(Buffer.from(canonicalPublicJson({publicKeys:m.publicKeys,withdrawalPublicKeys:m.withdrawalPublicKeys}))));
 assert.equal(m.campaignId,publicSha256(Buffer.from(canonicalPublicJson({format:'nano-isolated-capella-key-identity-v1',genesisValidatorsRoot:m.genesisValidatorsRoot,keySetSha256:m.keySetSha256,transactionSenderAddress:m.transactionSender.address}))));
 shape(m.reservation,['format','campaignId','keySetSha256','genesisValidatorsRoot','status','ledgerScope','ledgerBindingSha256'],'Public reservation');
 assert.equal(m.reservation.format,'nano-isolated-capella-reservation-v1');assert.equal(m.reservation.status,'consumed-before-signing');assert.equal(m.reservation.ledgerScope,'explicit-operator-managed-ledger');hashText(m.reservation.ledgerBindingSha256);
 for(const field of ['campaignId','keySetSha256','genesisValidatorsRoot'])assert.equal(m.reservation[field],m[field]);
 shape(m.files,Object.keys(FILES),'Public files');assert.ok(artifacts instanceof Map);assert.deepEqual([...artifacts.keys()].sort(),Object.values(FILES).sort());
 for(const [key,path] of Object.entries(FILES)){const binding=m.files[key];shape(binding,['path','sha256','bytes'],key+' binding');assert.equal(binding.path,path);hashText(binding.sha256);const b=bytes(artifacts.get(path));assert.equal(binding.bytes,b.length);assert.equal(binding.sha256,publicSha256(b));}
 const profile=parsePublicJson(artifacts.get('profile.json')),parameters=profileParameters(profile);assert.equal(parameters.chainId,m.chainId);assert.equal(parameters.registryLimit,BigInt(m.registryLimit));
 const registry=artifacts.get('genesis-validators.ssz');assert.equal(registry.length,64*121);assert.equal(validatorRegistryRoot(registry,parameters.registryLimit),m.genesisValidatorsRoot);
 for(let i=0;i<64;i++){assert.equal(hex(registry.subarray(i*121,i*121+48)),m.publicKeys[i].publicKey);const credentials=sha(publicKey(m.withdrawalPublicKeys[i].publicKey));credentials[0]=0;assert.ok(credentials.equals(registry.subarray(i*121+48,i*121+80)),'Pre-transition withdrawal credentials mismatch');}
 const before=inspectCapellaState(artifacts.get('genesis-state.ssz'),profile);before.bytes=artifacts.get('genesis-state.ssz');assert.equal(before.slot,0n,'Genesis state must precede slot transitions');assert.equal(before.registryCount,64);assert.deepEqual(before.relatedListCounts,[64,64,64,64]);assert.ok(before.registry.equals(registry));assert.equal(before.registryRoot,m.genesisValidatorsRoot);assert.equal(before.genesisValidatorsRoot,m.genesisValidatorsRoot);assert.equal(before.stateRoot,m.genesisStateRoot);assert.equal(before.executionGenesisHash,m.executionGenesisHash);
 for(let i=0;i<64;i++)assert.equal(hex(before.registry.subarray(i*121,i*121+48)),m.publicKeys[i].publicKey);
 const initial=parsePublicJson(artifacts.get('initial.json'));assert.equal(initial.format,'nano-capella-store-v1');shape(initial,['ancestors','anchor','blocks','boost','clock','equivocations','finalized','format','justified','messages','profile','timely','unrealized'],'Initial public store');assert.deepEqual(initial.profile,profile);assert.equal(initial.anchor,m.initialHeadRoot);assert.ok(Array.isArray(initial.blocks)&&initial.blocks.length===1);
 const row=initial.blocks[0];assert.ok(Array.isArray(row)&&row.length===6&&Array.isArray(row[1])&&row[1].length===5&&Array.isArray(row[2])&&row[2].length===2);
 assert.equal(row[0],m.initialHeadRoot);assert.equal(row[1][3],m.initialStateRoot);
 const header=Buffer.alloc(112);for(let i=0;i<2;i++)header.writeBigUInt64LE(quantity(row[1][i]),i*8);for(let i=0;i<3;i++)rootBytes(row[1][i+2]).copy(header,16+32*i);assert.equal(hex(headerRoot(header)),m.initialHeadRoot);
 assert.equal(typeof row[2][0],'string');assert.match(row[2][0],/^0x(?:[0-9a-f]{2})+$/);const after=inspectCapellaState(Buffer.from(row[2][0].slice(2),'hex'),profile);
 assert.equal(after.stateRoot,m.initialStateRoot);assert.equal(after.genesisValidatorsRoot,m.genesisValidatorsRoot);assert.equal(after.registryCount,64);assert.deepEqual(after.relatedListCounts,[64,64,64,64]);assert.equal(after.genesisTime,before.genesisTime);assert.equal(after.slot,quantity(row[1][0]));assert.ok(after.slot>0n);assert.notEqual(m.initialStateRoot,m.genesisStateRoot);assert.ok(after.latestHeader.subarray(0,48).equals(header.subarray(0,48))&&after.latestHeader.subarray(48,80).equals(ZERO)&&after.latestHeader.subarray(80).equals(header.subarray(80)),'Initial latest header differs from authenticated header');assert.ok(Array.isArray(initial.ancestors)&&initial.ancestors.some(a=>a.hash===m.executionGenesisHash&&a.number==='0x0'),'Execution genesis absent from initial ancestors');
 for(let i=0;i<64;i++)assert.equal(hex(after.registry.subarray(i*121,i*121+48)),m.publicKeys[i].publicKey,'Post-transition validator identity differs');
 verifyBootstrap(m,initial,before,after,header,profile,parameters);
 const genesis=parsePublicJson(artifacts.get('genesis.json'));shape(genesis,['alloc','config','timestamp'],'Public genesis');shape(genesis.config,['chainId','consensus','evmFork'],'Genesis config');assert.equal(genesis.config.chainId,1337);assert.equal(genesis.config.consensus,'beacon');assert.equal(genesis.config.evmFork,'Shanghai');assert.equal(quantity(genesis.timestamp),before.genesisTime);assert.ok(quantity(genesis.alloc[m.transactionSender.address]?.balance)>0n);assert.ok(!genesis.alloc[legacy.transactionSenderAddress]||quantity(genesis.alloc[legacy.transactionSenderAddress].balance)===0n);
 const positions=before.committees.map(keys=>keys.map(key=>{const member=m.publicKeys.find(x=>x.publicKey===key);assert.ok(member&&member.index<64,'Genesis committee key outside pre-transition registry');return member.index;}));
 for(const keys of after.committees)for(const key of keys)assert.ok(m.publicKeys.slice(0,64).some(k=>k.publicKey===key),'Initial committee contains a legacy or unknown key');
 return {format:m.format,campaignId:m.campaignId,genesisValidatorsRoot:m.genesisValidatorsRoot,registryCount:64,registryLimit:m.registryLimit,independentRegistryRootVerified:true,independentGenesisStateRootVerified:true,independentInitialStateRootVerified:true,independentInitialHeaderRootVerified:true,publicFileHashesVerified:true,reservationBindingsVerified:true,bootstrapWireBindingsVerified:true,bootstrapSigningRootsVerified:true,bootstrapDutyPolicyBindingsVerified:true,declaredBootstrapSignatureCount:4,bootstrapCryptographicSignaturesVerified:false,preExportSigningOrderObserved:false,bootstrapJournalImportQualified:false,pendingDepositTransactionBindingsVerified:false,legacyValidatorPublicKeyDisjointnessVerified:true,legacyPublicKeyBaselineCount:65,legacyWithdrawalPublicKeyDisjointnessVerified:false,withdrawalCredentialsVerified:true,genesisCommitteePositions:positions,committeeMembershipVerified:true,blsSubgroupVerified:false,committeeAggregateCryptographyVerified:false,senderPublicKeyAddressCryptographyVerified:false,executionGenesisHashCryptographyVerified:false,reservationConsumptionObserved:false,globalLedgerNonReuseVerified:false,universalEvmReplayProtectionVerified:false,networkQualified:false,productionQualified:false};
}
