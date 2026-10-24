// Source-extracted control functions only. No program, RPC, key, DB or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {join} from 'node:path';
const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8');
function section(start,end){assert.equal(source.split(start).length,2,'Unique actual source boundary');const at=source.indexOf(start),until=source.indexOf(end,at+start.length);assert(until>at);return source.slice(at,until);}
const publishers=section(' async function publishOperatorOwnership(){','\n const safe=name=>');
const ownershipStateDeclaration=section(' let operatorOwnershipReady=','\n const exec=');
const ownedRunsDeclaration=section(' const ownedRuns=','\n function exactOwnedCleanup(');
const execDeclaration=section(' const exec=(program,argv,options={})=>','\n const need=');
const dockerDeclaration=section(' const docker=async(a,remote=false,timeout=30000,input=null)=>{','\n const uuid=');
const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
const need=(ok,message)=>{if(!ok)throw Error(message);};

function fixture({legacy=false,blockHandoff=false,failAt=null,publicationError=Error('known handoff failure'),interrupted=null,budgetError=null}={}){
 const events=[],writes=new Map(),actors=[],uuid='11111111-2222-4333-8444-555555555555',label='nano.fresh.campaign='+uuid;
 const containers={'mac-docker':'nano-qualification-mac-'+uuid,'vps-docker':'nano-qualification-vps-'+uuid},target='ubuntu@127.0.0.1',output='/synthetic/output';
 const spec={overlayManifest:{sha256:'a'.repeat(64)},lifecycle:{deadlines:{maximumRunMs:10000}},fixture:{seedFile:'SENSITIVE seed locator',ledgerDirectory:'SENSITIVE ledger locator'},owners:{'mac-native':{programs:{nano:{path:'/synthetic/software/nano',sha256:'b'.repeat(64),bytes:123}}},'mac-docker':{image:'sha256:'+'c'.repeat(64)},'vps-docker':{image:'sha256:'+'d'.repeat(64)}}};
 let release,atHandoff;const barrier=new Promise(resolve=>{release=resolve;}),handoffEntered=new Promise(resolve=>{atHandoff=resolve;});
 const hashHarness=async path=>{assert.equal(path,'/synthetic/source');events.push('hash-harness');return {'run.mjs':'e'.repeat(64)};};
 async function save(path,value){
  const leaf=path.split('/').at(-1);events.push('save-start:'+leaf);assert.equal(writes.has(path),false,'New operator records are exclusive');
  if(leaf==='operator-ownership.json'){atHandoff();if(blockHandoff)await barrier;}
  if(leaf===failAt)throw publicationError;
  const bytes=Buffer.from(JSON.stringify(value,null,2)+'\n');writes.set(path,{value:structuredClone(value),bytes});events.push('save-finish:'+leaf);return bytes;
 }
 const recordActor=(kind,program,args,options)=>{events.push('actor:'+kind);actors.push({kind,program,args,options});return Promise.resolve({stdout:'known output',stderr:''});};
 const commands=legacy?null:{exec:(...args)=>recordActor('command',...args),execOwnedCleanup:(...args)=>recordActor('cleanup',...args)};
 const deps={need,save,hashHarness,join,output,harnessRoot:'/synthetic/source',sha,uuid,label,process:{pid:500001},revision:'f'.repeat(40),specSha:'1'.repeat(64),spec,target,remoteRoot:'/home/ubuntu/nano-fresh-'+uuid,containers,owner:name=>spec.owners[name],campaignCommands:commands,campaignBudget:legacy?null:{check(){events.push('budget-check');if(budgetError)throw budgetError;}},interrupted,campaignCloseoutReserve:1000,
  legacyExec:(...args)=>recordActor('legacy',...args),exactOwnedCleanup:args=>args[0]==='inspect',quote:String,sshArgs:[],sh:async()=>{events.push('actor:ssh');actors.push({kind:'ssh'});return 'known output';},dockerInput:async()=>{events.push('actor:stdin');actors.push({kind:'stdin'});return Buffer.from('known output');}};
 const code=ownershipStateDeclaration+ownedRunsDeclaration+execDeclaration+dockerDeclaration+publishers+";return {publish:publishOperatorOwnership,birth:publishOperatorProcess,exec,docker,state:()=>({ready:operatorOwnershipReady,attempted:operatorOwnershipAttempted,pin:operatorOwnershipPin,sequence:operatorProcessSequence,ownedRuns:[...ownedRuns]})};";
 const controller=new Function(...Object.keys(deps),code)(...Object.values(deps));
 return {...controller,events,writes,actors,release,handoffEntered,uuid,label,containers,spec,target,output,publicationError};
}

test('C8 ownership handoff binds reviewed intent and gates every command until both known records finish publication',async()=>{
 // Bind the real driver call sites too: a separately correct publisher does
 // not prove that it precedes actors or that birth awaits preserve listeners.
 const freshAt=source.indexOf('async function runFreshCampaign(args){');assert(freshAt>=0);
 const bodyAt=source.indexOf('\n try{\n  if(interrupted)throw interrupted;',freshAt);assert(bodyAt>freshAt);
 const publishAt=source.indexOf('await publishOperatorOwnership();',bodyAt),softwareAt=source.indexOf('initialSoftware=await software();',bodyAt);
 assert(publishAt>bodyAt&&softwareAt>publishAt,'Actual driver must publish ownership before its first software actor');
 for(const [marker,listener]of [["initialRuntimeLaunch('mac-native',async()=>","native.on('error'"],["lifecycleNativeLog=logPath;let error;try{","native.once('error'"],["const tunnelFd=openSync","tunnel.on('error'"]]){
  const at=source.indexOf(marker,freshAt);assert(at>=freshAt&&source.indexOf(marker,at+marker.length)===-1,'One actual persistent birth site');
  const line=source.slice(at,source.indexOf('\n',at)),errorAt=line.indexOf(listener),birthAt=line.indexOf('await publishOperatorProcess(');
  assert(errorAt>=0&&birthAt>errorAt,'Install actual error listener before the first birth publication await');assert(/}finally\{closeSync\((?:fd|tunnelFd)\);}/.test(line),'Actual parent log FD still closes in finally');
 }
 const f=fixture({blockHandoff:true}),pending=f.publish();await f.handoffEntered;
 assert.equal(f.state().ready,false);assert.throws(()=>f.exec('/synthetic/never',[]),/Durable operator ownership handoff required/);
 await assert.rejects(f.docker(['inspect',f.containers['mac-docker'],'--format','{{json .}}']),/Durable operator ownership handoff required/);
 assert.equal(f.actors.length,0);assert.equal(f.state().ownedRuns.length,0);
 f.release();const pin=await pending,handoff=f.writes.get(f.output+'/operator-ownership.json'),harness=f.writes.get(f.output+'/harness.json'),v=handoff.value;
 assert.equal(f.state().ready,true);assert(Object.isFrozen(pin));assert.equal(pin.sha256,sha(handoff.bytes));assert.equal(pin.bytes,handoff.bytes.length);
 assert.equal(v.format,'nano-fresh-c8-operator-ownership-v1');assert.equal(v.phase,'pre-effect-intent');assert.equal(v.operatorUuid,f.uuid);assert.equal(v.label,f.label);assert.equal(v.controllerPid,500001);
 assert.equal(v.sourceBaseRevision,'f'.repeat(40));assert.equal(v.specSha256,'1'.repeat(64));assert.equal(v.overlayManifestSha256,f.spec.overlayManifest.sha256);
 assert.deepEqual(v.harness,{path:'harness.json',sha256:sha(harness.bytes),bytes:harness.bytes.length});assert.equal(v.remoteTarget,f.target);assert.equal(v.remoteRoot,'/home/ubuntu/nano-fresh-'+f.uuid);
 assert.deepEqual(v.dockerOwners.map(o=>[o.owner,o.host,o.name,o.image]),[['mac-docker','mac',f.containers['mac-docker'],f.spec.owners['mac-docker'].image],['vps-docker',f.target,f.containers['vps-docker'],f.spec.owners['vps-docker'].image]]);
 for(const owner of v.dockerOwners)assert.deepEqual(owner.operationNamePrefixes,['nano-qualification-op-'+f.uuid+'-',owner.name+'-']);
 assert.deepEqual(v.nativeProgram,f.spec.owners['mac-native'].programs.nano);assert.equal(v.maximumRunMs,10000);assert.equal(v.closeoutReserveMs,1000);assert.equal(v.externalObservationRequired,true);assert.equal(v.campaignAuthorizedByThisRecord,false);assert.equal(v.productionQualified,false);
 assert.equal(JSON.stringify(v).includes('SENSITIVE'),false);
 await f.exec('/synthetic/known',[]);await f.docker(['run','--rm','synthetic-image']);
 assert.equal(f.state().ownedRuns.length,1);assert(f.events.indexOf('save-finish:operator-ownership.json')<f.events.indexOf('actor:command'));
 await assert.rejects(f.publish(),/single publication attempt/);assert.equal(f.writes.size,2);
});

test('failed handoff publication or interrupted admission cannot create an actor, retarget records, or gate legacy modes',async()=>{
 for(const failAt of ['harness.json','operator-ownership.json']){
  const error=Error('known publication failure'),f=fixture({failAt,publicationError:error});await assert.rejects(f.publish(),caught=>caught===error);
  assert.equal(f.state().ready,false);assert.equal(f.state().pin,null);assert.equal(f.state().attempted,true);
  assert.throws(()=>f.exec('/synthetic/never',[]),/Durable operator ownership handoff required/);await assert.rejects(f.docker(['inspect','known-owner','--format','{{json .}}']),/Durable operator ownership handoff required/);
  const count=f.events.length;await assert.rejects(f.publish(),/single publication attempt/);assert.equal(f.events.length,count);assert.equal(f.actors.length,0);assert.equal(f.state().ownedRuns.length,0);
 }
 const interruption=Error('known pre-effect interruption'),cancelled=fixture({interrupted:interruption});await assert.rejects(cancelled.publish(),error=>error===interruption);assert.equal(cancelled.state().ready,false);assert.throws(()=>cancelled.exec('/synthetic/never',[]),/Durable operator ownership handoff required/);assert.equal(cancelled.actors.length,0);
 const deadline=Error('known admission deadline'),expired=fixture({budgetError:deadline});await assert.rejects(expired.publish(),error=>error===deadline);assert.equal(expired.state().ready,false);assert.equal(expired.state().pin,null);assert.equal(expired.state().attempted,true);assert.throws(()=>expired.exec('/synthetic/never',[]),/Durable operator ownership handoff required/);assert.equal(expired.actors.length,0);
 const legacy=fixture({legacy:true});assert.equal(await legacy.publish(),null);assert.equal(await legacy.birth('not-used',{}),null);assert.equal(legacy.writes.size,0);await legacy.exec('/synthetic/legacy',[]);assert.equal(legacy.actors[0].kind,'legacy');
});

test('post-spawn PID receipts bind one handoff and distinct generations without claiming a live or atomic process observation',async()=>{
 const f=fixture();await assert.rejects(f.birth('mac-native',{pid:600001}),/Durable operator ownership handoff required/);await f.publish();
 for(const pid of [undefined,0,1,-1,28722,500001,1.5,Number.MAX_SAFE_INTEGER+1])await assert.rejects(f.birth('mac-native',{pid}),/Actual owned child PID required/);
 await assert.rejects(f.birth('foreign-role',{pid:600001}),/Actual owned child PID required/);assert.equal(f.state().sequence,0);assert.equal(f.writes.size,2);
 for(const [role,pid]of [['mac-native',600001],['mac-native',600002],['ssh-tunnel',600003]])await f.birth(role,{pid});
 assert.equal(f.state().sequence,3);
 for(let sequence=1;sequence<=3;++sequence){const row=f.writes.get(f.output+'/operator-process-'+sequence+'.json').value;
  assert.equal(row.operatorUuid,f.uuid);assert.equal(row.controllerPid,500001);assert.equal(row.ownershipSha256,f.state().pin.sha256);assert.equal(row.ownershipBytes,f.state().pin.bytes);assert.equal(row.sequence,sequence);assert.equal(row.pid,600000+sequence);assert.equal(row.expectedPgid,row.pid);
  assert.equal(row.role,sequence===3?'ssh-tunnel':'mac-native');assert.equal(row.program,sequence===3?'ssh':'/synthetic/software/nano');assert.equal(row.publication,'post-spawn-not-atomic');assert.equal(row.liveProcessObserved,false);assert.equal(row.campaignAuthorizedByThisRecord,false);assert.equal(row.productionQualified,false);
 }
 assert.equal(f.actors.length,0);
});
