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
 return {history,randao,slashings,historicalLimit,votesLimit,registryLimit,sync,slots,maximumBytes:quantity(c[8]),extraDataLimit:quantity(c[9]),chainId:c[5]};
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
 return {stateRoot:hex(container(roots)),genesisValidatorsRoot:hex(b.subarray(8,40)),registryRoot:validatorRegistryRoot(registry,p.registryLimit),registry:Buffer.from(registry),registryCount:count,registryLimit:p.registryLimit,slot:b.readBigUInt64LE(40),genesisTime:b.readBigUInt64LE(0),executionGenesisHash:hex(execution.subarray(472,504)),committees,latestHeader:Buffer.from(b.subarray(64,176)),relatedListCounts:[balances.length/8,previous.length,current.length,scores.length/8]};
}
function parsePublicJson(b){
 const text=bytes(b).toString('utf8');assert.ok(Buffer.from(text,'utf8').equals(b),'JSON must be UTF-8');const parsed=JSON.parse(text),tokens=text.match(/"(?:\\.|[^"\\])*"|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?|true|false|null|[{}\[\]:,]/g);let i=0;
 function value(){const t=tokens[i++];if(t==='{'){const seen=new Set();if(tokens[i]==='}'){i++;return;}for(;;){const key=JSON.parse(tokens[i++]);assert.equal(typeof key,'string');assert.ok(!seen.has(key),'Duplicate JSON field');seen.add(key);assert.equal(tokens[i++],':');value();const next=tokens[i++];if(next==='}')break;assert.equal(next,',');}}else if(t==='['){if(tokens[i]===']'){i++;return;}for(;;){value();const next=tokens[i++];if(next===']')break;assert.equal(next,',');}}}
 value();assert.equal(i,tokens.length);return parsed;
}
const FILES={genesisValidators:'genesis-validators.ssz',genesisState:'genesis-state.ssz',genesis:'genesis.json',profile:'profile.json',initial:'initial.json'};
export function verifyIsolatedFixtureDomain({manifest:m,artifacts,legacy}){
 shape(m,['format','synthetic','productionQualified','fork','chainId','campaignId','keySetSha256','genesisValidatorsRoot','genesisStateRoot','executionGenesisHash','initialHeadRoot','initialStateRoot','registryLimit','registryCount','publicKeys','withdrawalPublicKeys','keyDerivation','files','legacyKeyDisjointnessVerified','reservation','transactionSender'],'Isolated domain');
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
 const before=inspectCapellaState(artifacts.get('genesis-state.ssz'),profile);assert.equal(before.slot,0n,'Genesis state must precede slot transitions');assert.equal(before.registryCount,64);assert.deepEqual(before.relatedListCounts,[64,64,64,64]);assert.ok(before.registry.equals(registry));assert.equal(before.registryRoot,m.genesisValidatorsRoot);assert.equal(before.genesisValidatorsRoot,m.genesisValidatorsRoot);assert.equal(before.stateRoot,m.genesisStateRoot);assert.equal(before.executionGenesisHash,m.executionGenesisHash);
 const initial=parsePublicJson(artifacts.get('initial.json'));shape(initial,['ancestors','anchor','blocks','boost','clock','equivocations','finalized','format','justified','messages','profile','timely','unrealized'],'Initial public store');assert.deepEqual(initial.profile,profile);assert.equal(initial.anchor,m.initialHeadRoot);assert.ok(Array.isArray(initial.blocks)&&initial.blocks.length===1);
 const row=initial.blocks[0];assert.ok(Array.isArray(row)&&row.length===6&&Array.isArray(row[1])&&row[1].length===5&&Array.isArray(row[2])&&row[2].length===2);
 assert.equal(row[0],m.initialHeadRoot);assert.equal(row[1][3],m.initialStateRoot);
 const header=Buffer.alloc(112);for(let i=0;i<2;i++)header.writeBigUInt64LE(quantity(row[1][i]),i*8);for(let i=0;i<3;i++)rootBytes(row[1][i+2]).copy(header,16+32*i);assert.equal(hex(headerRoot(header)),m.initialHeadRoot);
 assert.equal(typeof row[2][0],'string');assert.match(row[2][0],/^0x(?:[0-9a-f]{2})+$/);const after=inspectCapellaState(Buffer.from(row[2][0].slice(2),'hex'),profile);
 assert.equal(after.stateRoot,m.initialStateRoot);assert.equal(after.genesisValidatorsRoot,m.genesisValidatorsRoot);assert.equal(after.registryCount,64);assert.deepEqual(after.relatedListCounts,[64,64,64,64]);assert.equal(after.genesisTime,before.genesisTime);assert.equal(after.slot,quantity(row[1][0]));assert.ok(after.slot>0n);assert.notEqual(m.initialStateRoot,m.genesisStateRoot);assert.ok(after.latestHeader.subarray(0,48).equals(header.subarray(0,48))&&after.latestHeader.subarray(48,80).equals(ZERO)&&after.latestHeader.subarray(80).equals(header.subarray(80)),'Initial latest header differs from authenticated header');assert.ok(Array.isArray(initial.ancestors)&&initial.ancestors.some(a=>a.hash===m.executionGenesisHash&&a.number==='0x0'),'Execution genesis absent from initial ancestors');
 const genesis=parsePublicJson(artifacts.get('genesis.json'));shape(genesis,['alloc','config','timestamp'],'Public genesis');shape(genesis.config,['chainId','consensus','evmFork'],'Genesis config');assert.equal(genesis.config.chainId,1337);assert.equal(genesis.config.consensus,'beacon');assert.equal(genesis.config.evmFork,'Shanghai');assert.equal(quantity(genesis.timestamp),before.genesisTime);assert.ok(quantity(genesis.alloc[m.transactionSender.address]?.balance)>0n);assert.ok(!genesis.alloc[legacy.transactionSenderAddress]||quantity(genesis.alloc[legacy.transactionSenderAddress].balance)===0n);
 const positions=before.committees.map(keys=>keys.map(key=>{const member=m.publicKeys.find(x=>x.publicKey===key);assert.ok(member&&member.index<64,'Genesis committee key outside pre-transition registry');return member.index;}));
 for(const keys of after.committees)for(const key of keys)assert.ok(m.publicKeys.slice(0,64).some(k=>k.publicKey===key),'Initial committee contains a legacy or unknown key');
 return {format:m.format,campaignId:m.campaignId,genesisValidatorsRoot:m.genesisValidatorsRoot,registryCount:64,registryLimit:m.registryLimit,independentRegistryRootVerified:true,independentGenesisStateRootVerified:true,independentInitialStateRootVerified:true,independentInitialHeaderRootVerified:true,publicFileHashesVerified:true,reservationBindingsVerified:true,legacyPublicKeyDisjointnessVerified:true,withdrawalCredentialsVerified:true,genesisCommitteePositions:positions,committeeMembershipVerified:true,blsSubgroupVerified:false,committeeAggregateCryptographyVerified:false,senderPublicKeyAddressCryptographyVerified:false,executionGenesisHashCryptographyVerified:false,reservationConsumptionObserved:false,globalLedgerNonReuseVerified:false,universalEvmReplayProtectionVerified:false,networkQualified:false,productionQualified:false};
}
