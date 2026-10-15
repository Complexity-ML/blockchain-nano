#!/usr/bin/env node
// Repartition PUBLIC exporter fixtures only. Never use a production genesis.
import {readFile,writeFile,mkdir,copyFile,cp,lstat,realpath} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {join,resolve} from 'node:path';
async function legacyMain(){
const [input,output,topologyFile]=process.argv.slice(2);
if(!input||!output||!topologyFile)throw Error('Usage: prepare.mjs exported-fixtures new-output topology.json');
const read=async p=>JSON.parse(await readFile(p,'utf8'));
const topology=await read(topologyFile), genesis=await read(join(input,'genesis.json'));
const nodes=await Promise.all([0,1].map(i=>read(join(input,`node-${i}.json`))));
const envs=await Promise.all([0,1].map(i=>read(join(input,`env-${i}.json`))));
const fork=nodes.every(n=>n.capella&&!n.bellatrix)?'capella':nodes.every(n=>n.bellatrix&&!n.capella)?'bellatrix':null;
if(!fork)throw Error('Mixed or unsupported consensus fixtures');
const snapshot=fork==='capella'?'initial.json':'snapshot.json';
const keys=nodes.flatMap(n=>n[fork].validator.keys), environment=Object.assign({},...envs);
// Exporter uses chain 1337 and scalar index+1 for every PUBLIC synthetic key.
const chain=genesis.config?.chainId;
if(BigInt(chain)!==1337n||keys.length!==64)throw Error('Not the expected isolated exporter fixture');
const indices=new Set(),names=new Set();
for(const k of keys){
 const index=BigInt(k.index),name=fork==='capella'?`NANO_TEST_CAPELLA_NETWORK_KEY_${index}`:`NANO_KURTOSIS_FIXTURE_${index}`;
 if(index<0n||index>=64n||indices.has(String(index))||k.keyEnv!==name||BigInt(environment[name])!==index+1n)throw Error('Unexpected or duplicate fixture key');
 indices.add(String(index));
}
if(!Array.isArray(topology)||topology.length<2||topology.length>keys.length)throw Error('Each test instance needs a nonempty key partition');
for(const t of topology){
 if(typeof t.host!=='string'||!t.host||!/^[a-z0-9-]+$/.test(t.name)||names.has(t.name)||!['native','docker'].includes(t.mode))throw Error('Invalid topology identity');
 names.add(t.name);
 if(!Number.isInteger(t.rpcPort)||t.rpcPort<1024||t.rpcPort>65535||!Number.isInteger(t.p2pPort)||t.p2pPort<1024||t.p2pPort>65535)throw Error('Invalid port');
 if(t.mode==='native'&&(!t.p2pBinary?.startsWith('/')||!t.runtimeDirectory?.startsWith('/')))throw Error('Native absolute paths required');
}
await mkdir(output); // Refuse to overwrite an earlier test or its journals.
const manifest={fork,physicalHosts:new Set(topology.map(t=>t.host)).size,syntheticValidators:keys.length,networkQualified:false,nodes:[]};
for(let i=0;i<topology.length;i++){
 const t=topology[i],directory=join(output,t.name);await mkdir(directory);
 const selected=keys.filter((_,index)=>index%topology.length===i);
 const node=structuredClone(nodes[0]),config=t.mode==='docker'?'/config':t.runtimeDirectory;
 const data=t.mode==='docker'?'/var/lib/nano':join(t.runtimeDirectory,'data');
 node.rpc_host='127.0.0.1';node.rpc_port=t.rpcPort;
 node.listen_addr=`/ip4/0.0.0.0/tcp/${t.p2pPort}`;
 node.p2p_binary=t.mode==='docker'?'/usr/local/bin/nano-p2p':t.p2pBinary;
 node.bootnodes=[]; // Filled from the actual remote peer ID at startup.
 node.p2p_identity=join(data,'peer.key');
 node[fork].profile.file=join(config,'profile.json');node[fork].initial.file=join(config,snapshot);
 node[fork].recoveryWitness=join(data,'core-witness');
 node[fork].validator.keys=selected.map(k=>({...k,journal:join(data,`key-${BigInt(k.index)}`),recoveryWitness:join(data,`key-${BigInt(k.index)}-witness`)}));
 for(const name of ['genesis.json','profile.json',snapshot])await copyFile(join(input,name),join(directory,name));
 await writeFile(join(directory,'node.json'),JSON.stringify(node,null,2)+'\n');
 await writeFile(join(directory,'fixture.env'),selected.map(k=>`${k.keyEnv}=${environment[k.keyEnv]}`).join('\n')+'\n',{mode:0o600});
 manifest.nodes.push({name:t.name,mode:t.mode,indices:selected.map(k=>String(BigInt(k.index))),configuration:resolve(directory)});
}
await writeFile(join(output,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify(manifest));

}

const ownerNames=['mac-native','mac-docker','vps-docker'];
const sha256=b=>createHash('sha256').update(b).digest('hex');
function freshNeed(ok,message){if(!ok)throw Error(message);}
function freshAbsolute(p){freshNeed(typeof p==='string'&&p.startsWith('/')&&resolve(p)===p&&p!=='/'&&!/[\0\r\n,]/.test(p),'Canonical fresh absolute path required');return p;}
function freshToken(t){freshNeed(t&&Object.keys(t).sort().join(',')==='digest,sequence'&&/^0x[1-9a-f][0-9a-f]*$/.test(t.sequence)&&/^0x[0-9a-f]{64}$/.test(t.digest),'Imported primary token required');}
// Pure model: caller data is copied before inspection. No key generation,
// signing, environment lookup, DB open, file copy or process operation here.
export function buildFreshNodePlan(input){
 const {nodes,envs,domain,bootstrapPlan,topology}=structuredClone(input);
 freshNeed(domain?.format==='nano-isolated-capella-fixture-domain-v1'&&domain.synthetic===true&&domain.productionQualified===false&&domain.registryCount===64&&domain.publicKeys?.length===65,'New isolated 64+pending fixture required');
 freshNeed(bootstrapPlan?.format==='nano-isolated-bootstrap-three-owner-plan-v1'&&bootstrapPlan.campaignId===domain.campaignId&&bootstrapPlan.genesisValidatorsRoot===domain.genesisValidatorsRoot&&bootstrapPlan.executionGenesisHash===domain.executionGenesisHash&&bootstrapPlan.modelOnly===true&&bootstrapPlan.runtimeStarted===false&&bootstrapPlan.productionQualified===false,'Exact imported bootstrap plan required');
 freshNeed(bootstrapPlan.campaignNamespace==='isolated-capella-'+domain.campaignId&&/^[0-9a-f]{64}$/.test(domain.campaignId),'Fresh campaign namespace required');
 freshNeed(bootstrapPlan.bootstrapDutyPolicy?.minimumNewProposalSlot===2&&bootstrapPlan.bootstrapDutyPolicy.maximumPresignedProposalSlot===1&&bootstrapPlan.bootstrapDutyPolicy.driverMayRegeneratePresignedHistory===false,'Presigned history floor required');
 freshNeed(bootstrapPlan.independentPrimaryCommitmentsVerified===65&&bootstrapPlan.independentWitnessCommitmentsVerified===65&&bootstrapPlan.externalAuditReportsAccepted?.publicBindings===true&&bootstrapPlan.externalAuditReportsAccepted?.blsCryptography===true&&bootstrapPlan.externalAuditReportsAccepted?.offlineJournalImport===true,'Independent public/crypto/import audits required');
 freshNeed(Array.isArray(nodes)&&nodes.length===2&&Array.isArray(envs)&&envs.length===2&&Array.isArray(topology)&&topology.length===3,'Exact exported inputs and three owners required');
 const allKeys=new Map(),allEnv=new Map();
 for(let n=0;n<2;n++){
  freshNeed(nodes[n]?.capella&&!nodes[n].bellatrix&&!nodes[n].altair&&!nodes[n].bft&&nodes[n].capella.validator?.keys,'Exclusive Capella export required');
  const configured=new Set();
  for(const k of nodes[n].capella.validator.keys){
   const match=/^NANO_TEST_CAPELLA_NETWORK_KEY_(0|[1-9][0-9]?)$/.exec(k.keyEnv);freshNeed(match,'Fresh keyEnv identity required');const i=Number(match[1]);
   freshNeed(i<65&&!allKeys.has(i)&&!configured.has(k.keyEnv)&&(i===64?k.index===null:k.index==='0x'+i.toString(16))&&k.restart===null,'Export key identity/ownership differs');
   freshNeed(typeof envs[n]?.[k.keyEnv]==='string'&&/^0x[0-9a-f]{64}$/.test(envs[n][k.keyEnv]),'Fresh scalar encoding required');configured.add(k.keyEnv);allKeys.set(i,k);allEnv.set(k.keyEnv,envs[n][k.keyEnv]);
  }
  freshNeed(Object.keys(envs[n]).every(k=>k==='NANO_TEST_NETWORK_TRACE'&&envs[n][k]==='1'||configured.has(k)),'Unexpected export environment name');
 }
 freshNeed(allKeys.size===65&&allEnv.size===65,'Every fresh key must have exactly one exported owner');
 const assigned=new Set(),result=[];
 for(let n=0;n<3;n++){
  const t=topology[n],assignment=bootstrapPlan.owners?.[n];freshNeed(t?.name===ownerNames[n]&&assignment?.owner===t.name&&t.mode===(n===0?'native':'docker')&&typeof t.host==='string'&&t.host,'Ordered owner topology required');
  freshNeed(Number.isInteger(t.rpcPort)&&t.rpcPort>=1024&&t.rpcPort<=65535&&Number.isInteger(t.p2pPort)&&t.p2pPort>=1024&&t.p2pPort<=65535,'Fresh ports required');
  const expected=Array.from({length:n===2?33:16},(_,i)=>i+(n===0?0:n===1?16:32));freshNeed(assignment.keys?.length===expected.length,'Exact disjoint 16/16/33 assignment required');
  const directory=freshAbsolute(t.runtimeDirectory),config=t.mode==='docker'?'/config':directory;
  const dataBase=t.mode==='docker'?'/var/lib/nano-data':join(directory,'data'),witnessBase=t.mode==='docker'?'/var/lib/nano-witness':join(directory,'witness');
  const dataRoot=join(dataBase,bootstrapPlan.campaignNamespace,t.name),witnessRoot=join(witnessBase,bootstrapPlan.campaignNamespace,t.name);
  const node=structuredClone(nodes[0]);node.rpc_host='127.0.0.1';node.rpc_port=t.rpcPort;node.listen_addr=`/ip4/0.0.0.0/tcp/${t.p2pPort}`;
  node.p2p_binary=t.mode==='docker'?'/network-build/nano-p2p':freshAbsolute(t.p2pBinary);node.p2p_identity=join(dataRoot,'peer.key');node.bootnodes=[];
  node.capella.profile.file=join(config,'profile.json');node.capella.initial.file=join(config,'initial.json');node.capella.recoveryWitness=join(witnessRoot,'chain-witness');
  node.capella.validator.keys=[];const environment={},signerIdentities=[];
  for(let k=0;k<expected.length;k++){
   const i=expected[k],bound=assignment.keys[k],pub=domain.publicKeys[i];freshNeed(bound.publicKeyIndex===i&&pub.index===i&&bound.publicKey===pub.publicKey&&!assigned.has(i),'Imported public identity/ownership differs');assigned.add(i);
   const runtime=bound.runtimeKey;freshNeed(runtime?.keyEnv==='NANO_TEST_CAPELLA_NETWORK_KEY_'+i&&(i===64?runtime.index===null:runtime.index==='0x'+i.toString(16))&&bound.status===(i===64?'pending-deposit':'registered'),'Pending/registered runtime identity differs');freshToken(runtime.restart);freshToken(bound.expectedWitnessCommitment);freshNeed(runtime.restart.digest!==bound.expectedWitnessCommitment.digest,'Primary and witness roles must be distinct');
   const key=structuredClone(runtime);key.journal=join(dataRoot,'journal-'+i);key.recoveryWitness=join(witnessRoot,'witness-'+i);node.capella.validator.keys.push(key);environment[key.keyEnv]=allEnv.get(key.keyEnv);signerIdentities.push({keyEnv:key.keyEnv,publicKey:bound.publicKey});
  }
  result.push({name:t.name,mode:t.mode,host:t.host,directory,node,environment,indices:expected,signerIdentities,dataRoot,witnessRoot,stagedDataRoot:join(directory,'data',bootstrapPlan.campaignNamespace,t.name),stagedWitnessRoot:join(directory,'witness',bootstrapPlan.campaignNamespace,t.name)});
 }
 freshNeed(assigned.size===65&&new Set(topology.map(t=>t.host)).size===2,'Three runtimes on exactly two physical hosts');
 return {format:'nano-fresh-capella-preparation-model-v1',campaignId:domain.campaignId,campaignNamespace:bootstrapPlan.campaignNamespace,nodes:result};
}

const requiredFreshPrograms={
 'mac-native':['nano','p2p','exporter','bootstrapImport','bootstrapAudit','stoppedCapture'],
 'mac-docker':['nano','p2p','bootstrapAudit','stoppedCapture'],
 'vps-docker':['nano','p2p','bootstrapAudit','stoppedCapture']
};
function exactKeys(v,keys,label){freshNeed(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join('\0')===[...keys].sort().join('\0'),label+' exact fields');}
const canonicalFresh=v=>Array.isArray(v)?'['+v.map(canonicalFresh).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonicalFresh(v[k])).join(',')+'}':JSON.stringify(v);
function freshArtifact(v){exactKeys(v,['path','sha256','bytes'],'Artifact');freshAbsolute(v.path);freshNeed(/^[0-9a-f]{64}$/.test(v.sha256)&&Number.isSafeInteger(v.bytes)&&v.bytes>0&&v.bytes<=1073741824,'Artifact bytes/SHA');}
// This is an externally SHA-approved ROOT campaign authority. It is never
// manufactured from the build bindings or genesis that it is supposed to admit.
export function validateFreshCampaignSpec(value,{target,sourceBaseRevision}){
 const spec=structuredClone(value);exactKeys(spec,['format','sourceBaseRevision','overlayManifest','owners','fixture','clock','backupRestore','productionQualified'],'Fresh spec');
 freshNeed(spec.format==='nano-fresh-capella-three-owner-campaign-v1'&&spec.sourceBaseRevision===sourceBaseRevision&&/^[0-9a-f]{40}$/.test(sourceBaseRevision)&&spec.productionQualified===false&&typeof spec.backupRestore==='boolean','Fresh source authority');freshArtifact(spec.overlayManifest);
 exactKeys(spec.owners,ownerNames,'Owners');
 for(const name of ownerNames){
  const a=spec.owners[name];exactKeys(a,['host','kind','image','uid','gid','coreBuildRoot','networkBuildRoot','coreArchive','programs','admission'],'Owner');
  freshNeed(a.host===(name==='vps-docker'?target:'mac')&&a.kind===(name==='mac-native'?'native-macos-arm64':name==='mac-docker'?'linux-arm64':'linux-amd64'),'Exact three runtime/two physical host identity');freshAbsolute(a.coreBuildRoot);freshAbsolute(a.networkBuildRoot);freshNeed(a.coreBuildRoot!==a.networkBuildRoot&&!a.coreBuildRoot.startsWith(a.networkBuildRoot+'/')&&!a.networkBuildRoot.startsWith(a.coreBuildRoot+'/'),'Disjoint current code roots');
  freshNeed(a.uid===(name==='vps-docker'?1000:501)&&a.gid===(name==='vps-docker'?1000:20),'Actual current public build owners');
  freshNeed(a.image===(name==='mac-native'?null:name==='mac-docker'?'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'),'Existing immutable dependency image only');
  freshArtifact(a.coreArchive);freshArtifact(a.admission);exactKeys(a.programs,requiredFreshPrograms[name],'Required programs');for(const z of Object.values(a.programs))freshArtifact(z);
  const expectedCore=name==='mac-native'?a.coreBuildRoot+'/libnano_core.a':'/outputs/libnano_core.a';freshNeed(a.coreArchive.path===expectedCore,'Core archive must be selected root');
  for(const [role,z] of Object.entries(a.programs)){
   const leaf={nano:'nano',p2p:'nano-p2p',exporter:'nano-capella-transition-tests',bootstrapImport:'nano-capella-isolated-bootstrap-import',bootstrapAudit:'nano-capella-isolated-bootstrap-audit',stoppedCapture:'nano-capella-isolated-stopped-capture'}[role];
   freshNeed(z.path===(name==='mac-native'?(role==='p2p'?a.networkBuildRoot:a.coreBuildRoot):role==='p2p'?'/network-build':'/outputs')+'/'+leaf,'Program belongs to admitted current root');
  }
 }
 exactKeys(spec.fixture,['seedFile','ledgerDirectory','legacyPublicIdentity'],'Fixture');freshAbsolute(spec.fixture.seedFile);freshAbsolute(spec.fixture.ledgerDirectory);freshArtifact(spec.fixture.legacyPublicIdentity);freshNeed(!spec.fixture.seedFile.startsWith(spec.fixture.ledgerDirectory+'/'),'Seed separate from persistent ledger');
 exactKeys(spec.clock,['maximumStartupElapsedMs','maximumDowntimeMs','maximumSteps'],'Clock');for(const x of Object.values(spec.clock))freshNeed(typeof x==='string'&&/^0x[1-9a-f][0-9a-f]*$/.test(x)&&BigInt(x)<=(1n<<64n)-1n,'Finite explicit canonical fresh startup budget');
 return spec;
}
export function validateFreshOwnerAdmission(admission,spec,name){
 const a=structuredClone(admission),owner=spec.owners[name];exactKeys(a,['format','owner','sourceBaseRevision','overlayManifestSha256','coreObjects','coreArchive','programs','canonicalCmake','currentCompilerMmdArchiveClosureVerified','sdkAuthority','actualLinksExact','actualBuildExitCode','productionQualified'],'ROOT owner admission');
 freshNeed(a.format==='nano-root-fresh-capella-runtime-assets-admission-v1'&&a.owner===name&&a.sourceBaseRevision===spec.sourceBaseRevision&&a.overlayManifestSha256===spec.overlayManifest.sha256&&a.coreObjects===143&&a.canonicalCmake===true&&a.currentCompilerMmdArchiveClosureVerified===true&&a.sdkAuthority===(name==='mac-native'?'resolved-file-pins':'immutable-image')&&a.actualLinksExact===true&&a.actualBuildExitCode===0&&a.productionQualified===false,'Actual overlaid build Root admission required');
 freshNeed(canonicalFresh(a.coreArchive)===canonicalFresh(owner.coreArchive)&&canonicalFresh(a.programs)===canonicalFresh(owner.programs),'Admitted exact code artifacts differ');return a;
}
// This pure check applies only to the newly imported synthetic bootstrap,
// before the first runtime starts. Historical inputs and stopped captures keep
// their existing strict fences. The producer records eight stat fields, not
// nlink: no unrecorded material or filesystem inspection is inferred here.
export function verifyFreshBootstrapTransportInventory(before,after){
 const u64=(1n<<64n)-1n,i64=(1n<<63n)-1n;
 const rootFields=['format','root','rootDevice','rootInode','rootUid','rootGid','rootMode','rootMtime','rootCtime','entries'];
 const entryFields=['path','kind','device','inode','uid','gid','mode','bytes','mtime','ctime','sha256'];
 const unsigned=(v,label)=>{freshNeed(typeof v==='string'&&/^(0|[1-9][0-9]*)$/.test(v)&&BigInt(v)<=u64,label+' canonical uint64');};
 const number=(v,max,label)=>freshNeed(Number.isSafeInteger(v)&&v>=0&&v<=max,label+' nonnegative bounded integer');
 const time=(v,label)=>{freshNeed(typeof v==='string','Inventory '+label+' time string');const m=/^(-?(?:0|[1-9][0-9]*)):(0|[1-9][0-9]*)$/.exec(v);freshNeed(m&&m[1]!=='-0'&&BigInt(m[1])>=-i64-1n&&BigInt(m[1])<=i64&&BigInt(m[2])<=999999999n,'Inventory '+label+' canonical timespec');};
 const relative=p=>typeof p==='string'&&p.length>0&&!p.startsWith('/')&&!/[\0\r\n]/.test(p)&&p.split('/').every(x=>x.length>0&&x!=='.'&&x!=='..');
 const inspect=j=>{
  exactKeys(j,rootFields,'Bootstrap inventory');freshNeed(j.format==='nano-isolated-bootstrap-file-inventory-v1','Exact bootstrap inventory format');
  freshNeed(typeof j.root==='string'&&j.root.startsWith('/')&&j.root!=='/'&&relative(j.root.slice(1)),'Canonical bootstrap root');
  unsigned(j.rootDevice,'Root device');unsigned(j.rootInode,'Root inode');number(j.rootUid,4294967295,'Root uid');number(j.rootGid,4294967295,'Root gid');number(j.rootMode,4095,'Root mode');time(j.rootMtime,'root mtime');time(j.rootCtime,'root ctime');
  freshNeed(Array.isArray(j.entries)&&j.entries.length>0&&j.entries.length<=10000,'Bounded bootstrap entries');
  const entries=new Map(),identities=new Set([j.rootDevice+':'+j.rootInode]);
  for(const e of j.entries){
   exactKeys(e,entryFields,'Bootstrap entry');freshNeed(relative(e.path)&&!entries.has(e.path),'Unique canonical bootstrap entry');freshNeed(e.kind==='file'||e.kind==='directory','Regular file/directory entry required');
   unsigned(e.device,'Entry device');unsigned(e.inode,'Entry inode');number(e.uid,4294967295,'Entry uid');number(e.gid,4294967295,'Entry gid');number(e.mode,4095,'Entry mode');number(e.bytes,Number.MAX_SAFE_INTEGER,'Entry bytes');time(e.mtime,'entry mtime');time(e.ctime,'entry ctime');
   freshNeed(e.kind==='file'?typeof e.sha256==='string'&&/^[0-9a-f]{64}$/.test(e.sha256)&&e.bytes<=512*1024*1024:e.sha256===null,'Recorded regular-file SHA and producer byte bound');
   const identity=e.device+':'+e.inode;freshNeed(!identities.has(identity),'Aliased bootstrap root/entry identities refused');identities.add(identity);entries.set(e.path,e);
  }
  for(const path of entries.keys()){const at=path.lastIndexOf('/');if(at>=0)freshNeed(entries.get(path.slice(0,at))?.kind==='directory','Every bootstrap ancestor must be a recorded directory');}
  return entries;
 };
 const old=before,current=after,oldEntries=inspect(old),currentEntries=inspect(current);
 for(const key of rootFields)if(!['entries','rootMtime','rootCtime'].includes(key))freshNeed(old[key]===current[key],'Bootstrap root recorded identity/ownership changed');
 freshNeed(oldEntries.size===currentEntries.size,'Bootstrap namespace size changed');let filesVerified=0,directoriesVerified=0,directoryTimeChanges=0;
 for(const [path,e] of oldEntries){
  const now=currentEntries.get(path);freshNeed(now,'Bootstrap namespace entry removed or replaced');
  if(e.kind==='file'){freshNeed(canonicalFresh(e)===canonicalFresh(now),'Bootstrap file recorded material or SHA changed');filesVerified++;}
  else {for(const key of entryFields)if(key!=='mtime'&&key!=='ctime')freshNeed(e[key]===now[key],'Bootstrap directory non-time material changed');directoriesVerified++;if(e.mtime!==now.mtime||e.ctime!==now.ctime)directoryTimeChanges++;}
 }
 return {format:'nano-fresh-bootstrap-transport-inventory-check-v1',passed:true,sourceEntriesVerified:oldEntries.size,filesVerified,directoriesVerified,directoryTimeChanges,rootTimeChanges:Number(old.rootMtime!==current.rootMtime)+Number(old.rootCtime!==current.rootCtime),allRecordedFileMaterialAndSha256Unchanged:true,namespaceAndRecordedIdentitiesUnchanged:true,hardAliasIdentitiesRefused:true,runtimeStarted:false,productionQualified:false};
}
async function noAliasTree(root){
 const s=await lstat(root);freshNeed(!s.isSymbolicLink()&&(s.isDirectory()||s.isFile())&&(!s.isFile()||s.nlink===1),'Fresh source must have independent regular files/directories');
 if(s.isDirectory()){const {readdir}=await import('node:fs/promises');for(const name of await readdir(root))await noAliasTree(join(root,name));}
}
export async function prepareFresh(input,output,topology,bootstrapPlan){
 const read=async p=>JSON.parse(await readFile(p,'utf8'));
 const domain=await read(join(input,'isolated-domain.json')),nodes=await Promise.all([0,1].map(i=>read(join(input,`node-${i}.json`)))),envs=await Promise.all([0,1].map(i=>read(join(input,`env-${i}.json`))));
 const model=buildFreshNodePlan({domain,nodes,envs,topology,bootstrapPlan});await mkdir(output,{mode:0o700});
 const manifest={format:'nano-fresh-capella-prepared-three-owner-v1',fork:'capella',campaignId:model.campaignId,campaignNamespace:model.campaignNamespace,physicalHosts:2,syntheticValidators:64,pendingSyntheticValidators:1,networkQualified:false,nodes:[]};
 for(const t of model.nodes){
  freshNeed(resolve(join(output,t.name))===t.directory,'Topology runtime directory must be its exact new output child');await mkdir(t.directory,{mode:0o700});
  for(const name of ['genesis.json','profile.json','initial.json','isolated-domain.json'])await copyFile(join(input,name),join(t.directory,name));
  await mkdir(t.stagedDataRoot,{recursive:true,mode:0o700});await mkdir(t.stagedWitnessRoot,{recursive:true,mode:0o700});
  const owner=bootstrapPlan.owners.find(x=>x.owner===t.name);
  for(const bound of owner.keys){
   const i=bound.publicKeyIndex;for(const [role,base,leaf] of [['primary',t.stagedDataRoot,'journal-'],['witness',t.stagedWitnessRoot,'witness-']]){
    const source=freshAbsolute(bound.source[role]);freshNeed(source.endsWith('/'+role+'-'+i)&&await realpath(source)===source,'Exact imported journal source required');await noAliasTree(source);await cp(source,join(base,leaf+i),{recursive:true,errorOnExist:true,force:false,preserveTimestamps:true});
   }
  }
  await writeFile(join(t.directory,'node.json'),JSON.stringify(t.node,null,2)+'\n',{flag:'wx',mode:0o600});await writeFile(join(t.directory,'fixture.env'),Object.entries(t.environment).map(([k,v])=>k+'='+v).join('\n')+'\n',{flag:'wx',mode:0o600});
  manifest.nodes.push({name:t.name,mode:t.mode,host:t.host,indices:t.indices,signerIdentities:t.signerIdentities,configuration:t.directory,dataRoot:t.dataRoot,witnessRoot:t.witnessRoot,stagedDataRoot:t.stagedDataRoot,stagedWitnessRoot:t.stagedWitnessRoot});
 }
 await writeFile(join(output,'manifest.json'),JSON.stringify(manifest,null,2)+'\n',{flag:'wx',mode:0o600});return manifest;
}
async function main(){
 const args=process.argv.slice(2);
 if(args.length===7&&args[3]==='--fresh-bootstrap'&&args[5]==='--fresh-bootstrap-sha'){
  const [input,output,topologyFile,,planFile,,expected]=args;freshNeed(/^[0-9a-f]{64}$/.test(expected),'Explicit bootstrap plan SHA required');const bytes=await readFile(planFile);freshNeed(bytes.length<=16*1024*1024&&sha256(bytes)===expected,'Bootstrap plan bytes pin');const manifest=await prepareFresh(resolve(input),resolve(output),JSON.parse(await readFile(topologyFile,'utf8')),JSON.parse(bytes));console.log(JSON.stringify(manifest));return;
 }
 await legacyMain();
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url))await main();
