// New injected parser/transition and actual-source integration cases only.
// No Docker, SSH, shell, keys, DB, real process or campaign controls are used.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {dockerRuntimeInspectTemplate,dockerRuntimeNetworkInspectTemplate,parseDockerCreatedCid,validateDockerRuntimeInspection,validateDockerRuntimeIdentityForCleanup,launchDockerRuntimeNoRestart} from './docker-runtime-restart-guard.mjs';

const cid='a'.repeat(64),otherCid='b'.repeat(64),uuid='11111111-2222-4333-8444-555555555555',revision='c'.repeat(40),image='sha256:'+'d'.repeat(64);
const binding={name:'nano-qualification-mac-'+uuid,image,uuid,revision},bound={...binding,cid};
const inspect=(running=false)=>({Id:cid,Name:'/'+binding.name,Image:image,Config:{Labels:{'nano.fresh.campaign':uuid,'nano.qualification':'multihost','org.opencontainers.image.revision':revision}},State:{Running:running,Restarting:false,Paused:false,Dead:false,Pid:running?42:0,Status:running?'running':'created'},HostConfig:{RestartPolicy:{Name:'no',MaximumRetryCount:0}}});
const wire=value=>JSON.stringify(value);
const args=()=>['create','--restart=no','--name',binding.name,'--label','nano.fresh.campaign='+uuid,image,'/synthetic/nano','run'];

function parserCases(){
 assert(!dockerRuntimeInspectTemplate.includes('{{json .}}'));
 assert(!dockerRuntimeInspectTemplate.includes('.Config.Env'));
 assert(!dockerRuntimeInspectTemplate.includes('.Mounts'));
 assert(dockerRuntimeNetworkInspectTemplate.startsWith(dockerRuntimeInspectTemplate.slice(0,-1)));assert(dockerRuntimeNetworkInspectTemplate.includes('{{json .NetworkSettings.Networks}}'));assert(!dockerRuntimeNetworkInspectTemplate.includes('.Config.Env'));assert(!dockerRuntimeNetworkInspectTemplate.includes('.Mounts'));
 for(const key of ['.Id','.Name','.Image','.State.Running','.State.Restarting','.State.Paused','.State.Dead','.State.Pid','.State.Status','.HostConfig.RestartPolicy.Name','.HostConfig.RestartPolicy.MaximumRetryCount'])assert(dockerRuntimeInspectTemplate.includes(key));
 const before=validateDockerRuntimeInspection(wire(inspect()),bound,{phase:'before-start'});
 assert.equal(before.Id,cid);assert(Object.isFrozen(before));assert(Object.isFrozen(before.HostConfig.RestartPolicy));
 const exited=inspect();exited.State.Status='exited';assert.throws(()=>validateDockerRuntimeInspection(wire(exited),bound,{phase:'before-start'}),/phase-consistent/);assert.equal(validateDockerRuntimeInspection(wire(exited),bound,{phase:'either'}).State.Pid,0);
 assert.equal(validateDockerRuntimeInspection(wire(inspect(true)),bound,{phase:'after-start'}).State.Pid,42);
 const edits=[
  v=>v.HostConfig.RestartPolicy.Name='always',v=>v.HostConfig.RestartPolicy.Name='unless-stopped',v=>v.HostConfig.RestartPolicy.Name='on-failure',
  v=>v.HostConfig.RestartPolicy.MaximumRetryCount=1,v=>v.HostConfig.RestartPolicy.MaximumRetryCount='0',v=>delete v.HostConfig.RestartPolicy,
  v=>v.Id=otherCid,v=>v.Name='/foreign',v=>v.Image='latest',v=>v.Config.Labels['nano.fresh.campaign']='foreign',v=>v.Config.Labels['nano.qualification']='foreign',v=>v.Config.Labels['org.opencontainers.image.revision']='0'.repeat(40),
  v=>v.Config.Env=['SENSITIVE NEVER ACCEPTED'],v=>v.Mounts=[],v=>delete v.State.Pid,v=>v.State.Pid='0',v=>v.State.Restarting=true,v=>v.State.Paused=true,v=>v.State.Dead=true,v=>v.State.Status='removing',v=>v.State.Pid=1,
 ];
 for(const edit of edits){const value=inspect();edit(value);assert.throws(()=>validateDockerRuntimeInspection(wire(value),bound,{phase:'before-start'}),/Docker runtime restart guard/);}
 assert.throws(()=>validateDockerRuntimeInspection(wire(inspect(true)),bound,{phase:'before-start'}),/phase-consistent/);
 assert.throws(()=>validateDockerRuntimeInspection(wire(inspect()),bound,{phase:'after-start'}),/phase-consistent/);
 assert.throws(()=>validateDockerRuntimeInspection('not JSON',bound),/projected inspection JSON/);
 const cleanup=inspect();cleanup.HostConfig.RestartPolicy.Name='always';cleanup.State.Restarting=true;cleanup.State.Pid='incoherent';assert.equal(validateDockerRuntimeIdentityForCleanup(wire(cleanup),bound).Id,cid);cleanup.Id=otherCid;assert.throws(()=>validateDockerRuntimeIdentityForCleanup(wire(cleanup),bound),/binding/);
 assert.equal(parseDockerCreatedCid(cid),cid);assert.equal(parseDockerCreatedCid(cid+'\n'),cid);
 for(const text of ['',cid.slice(0,12),cid+'\n'+otherCid,cid+'\n\n',' '+cid,cid+' ',cid+'\r\n',undefined])assert.throws(()=>parseDockerCreatedCid(text),/one full created CID/);
}

function fixture({beforeEdit,afterEdit,publishError,gateError,createReply=cid+'\n',startReply=cid+'\n',lateCreate=false,gate}={}){
 const events=[],receipts=[],remembered=[];let running=false,created=false,cancelled=false;
 const check=()=>{events.push('check');if(cancelled)throw Error('injected deadline');};
 const docker=async argv=>{
  events.push(argv[0]);
  if(argv[0]==='create'){assert.deepEqual(argv,args());created=true;if(lateCreate)cancelled=true;return createReply;}
  if(argv[0]==='inspect'){assert(created);assert.deepEqual(argv,['inspect',cid,'--format',dockerRuntimeInspectTemplate]);const value=inspect(running);(running?afterEdit:beforeEdit)?.(value);return wire(value);}
  assert.deepEqual(argv,['start',cid]);assert.equal(remembered.length,1);running=true;return startReply;
 };
 const options={binding,createArgs:args(),docker,check,
  remember:value=>{events.push('remember');remembered.push(value);},
  publish:async receipt=>{events.push('publish:'+receipt.phase);if(publishError)throw publishError;receipts.push(receipt);},
  launchGate:gate??(async start=>{events.push('retained-copy-fence');if(gateError)throw gateError;events.push('clock-gate');return start();}),
  onStartAttempt:()=>{events.push('start-intent');},
 };
 return {options,events,receipts,remembered,run:()=>launchDockerRuntimeNoRestart(options)};
}

async function cleanTransitionCase(){
 const f=fixture(),value=await f.run();
 assert.equal(value.binding.cid,cid);assert.equal(f.remembered[0].cid,cid);
 const order=f.events.filter(x=>x!=='check');assert.deepEqual(order,['create','remember','inspect','publish:before-start','retained-copy-fence','clock-gate','start-intent','start','inspect','publish:after-start']);
 assert.equal(f.receipts.length,2);for(const r of f.receipts){assert.equal(r.cid,cid);assert.deepEqual(r.restartPolicy,{Name:'no',MaximumRetryCount:0});assert.equal(r.automaticRestartAllowed,false);assert.equal(r.productionQualified,false);assert.equal(r.projectedFieldsOnly,true);assert(!JSON.stringify(r).includes('Env'));}
}

async function beforeStartRefusalsCase(){
 for(const options of [{beforeEdit:v=>v.HostConfig.RestartPolicy.Name='always'},{beforeEdit:v=>v.State.Status='exited'},{beforeEdit:v=>v.Id=otherCid},{publishError:Error('publication refused')},{gateError:Error('retained copy moved')},{createReply:'short'}]){
  const f=fixture(options);await assert.rejects(f.run());assert(!f.events.includes('start'));assert(!f.events.includes('start-intent'));
 }
 const expired=fixture({lateCreate:true});await assert.rejects(expired.run(),/injected deadline/);assert.equal(expired.remembered.length,1,'Late actual create CID remains owned for closeout');assert(!expired.events.includes('start'));
}

async function afterStartRefusalsCase(){
 for(const options of [{afterEdit:v=>v.HostConfig.RestartPolicy.Name='always'},{afterEdit:v=>v.Id=otherCid},{afterEdit:v=>v.State.Restarting=true},{startReply:'short'}]){
  const f=fixture(options);await assert.rejects(f.run());assert(f.events.includes('start-intent'));assert.equal(f.events.filter(x=>x==='start').length,1);assert.equal(f.receipts.length,1);
 }
}

async function createArgvAndOneStartCase(){
 for(const edit of [a=>a.splice(1,1),a=>a[1]='--restart=always',a=>a.splice(1,0,'--restart=no'),a=>a.splice(1,0,'--restart','no'),a=>a[0]='run',a=>a.splice(1,0,'--rm'),a=>a[3]='foreign']){
  const f=fixture();edit(f.options.createArgs);await assert.rejects(f.run());assert.equal(f.events.filter(x=>x==='create').length,0);
 }
 const repeated=fixture({gate:async start=>{await start();await start();}});await assert.rejects(repeated.run(),/one explicit start attempt/);assert.equal(repeated.events.filter(x=>x==='start').length,1);
 const omitted=fixture({gate:async()=>{}});await assert.rejects(omitted.run(),/launch gate must invoke/);assert(!omitted.events.includes('start'));
}

async function pendingStartJoinCase(){
 for(const gateThrows of [false,true]){
  const f=fixture(),ordinaryDocker=f.options.docker,initial=Error('injected gate failure');let announce,release,settled=false;
  const called=new Promise(resolve=>{announce=resolve;}),pending=new Promise(resolve=>{release=resolve;});
  f.options.docker=async argv=>{const result=await ordinaryDocker(argv);if(argv[0]==='start'){announce();return pending;}return result;};
  f.options.launchGate=start=>{void start();if(gateThrows)throw initial;};
  const result=f.run();result.then(()=>{settled=true;},()=>{settled=true;});await called;await Promise.resolve();assert.equal(settled,false,'Guard owns and joins START even if its gate throws or omits await');release(cid+'\n');
  if(gateThrows)await assert.rejects(result,error=>error===initial);else await result;
 }
 const f=fixture(),ordinaryDocker=f.options.docker,initial=Error('primary gate error'),startFailure=Error('secondary start failure');let announce,rejectStart;
 const called=new Promise(resolve=>{announce=resolve;}),pending=new Promise((resolve,reject)=>{rejectStart=reject;});
 f.options.docker=async argv=>{const value=await ordinaryDocker(argv);if(argv[0]==='start'){announce();return pending;}return value;};
 f.options.launchGate=start=>{void start();throw initial;};const result=f.run();result.catch(()=>{});await called;rejectStart(startFailure);await assert.rejects(result,error=>error===initial);assert.equal(initial.dockerRuntimeStartSecondaryFailure,'Owned explicit Docker start also failed while joining');
 let delayed;const late=fixture({gate:start=>{delayed=start;}});await assert.rejects(late.run(),/launch gate must invoke/);assert.throws(()=>delayed(),/no longer admitted/);assert(!late.events.includes('start'));
}

const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8');
function actualSection(start,end){assert.equal(source.split(start).length,2,'Actual source marker unique');const at=source.indexOf(start),last=source.indexOf(end,at+start.length);assert(last>at);return source.slice(at,last);}
const actualStart=actualSection(' async function startDocker(name,','\n async function captureOwner(');

function actualFixture({initial=false,policy='no',failFence=false,afterBadPolicy=false,phase='work'}={}){
 const events=[],runtimeBindings=new Map(),started=new Set(),saved=[];let running=false;
 const docker=async(argv,remote)=>{
  events.push(argv[0]);assert.equal(remote,false);
  if(argv[0]==='ps')return '';
  if(argv[0]==='create'){assert.equal(argv[1],'--restart=no');assert(!argv.includes('-d'));return cid+'\n';}
  if(argv[0]==='inspect'){assert.deepEqual(argv,['inspect',cid,'--format',dockerRuntimeInspectTemplate]);const value=inspect(running);value.HostConfig.RestartPolicy.Name=running&&afterBadPolicy?'always':policy;return wire(value);}
  assert.deepEqual(argv,['start',cid]);assert(started.has('mac-docker'),'Actual integration records start intent before actor');running=true;return cid+'\n';
 };
 const deps={need:(ok,msg)=>{if(!ok)throw Error(msg);},interrupted:null,campaignBudget:{phase,check(){events.push('budget');}},docker,containers:{'mac-docker':binding.name},owner:()=>({image,uid:501,gid:20,programs:{nano:{path:'/synthetic/nano'}}}),uuid,revision,label:'nano.fresh.campaign='+uuid,mounts:()=>[],spec:{},configMount:()=>[],dataMount:()=>[],configAt:()=>'/synthetic/config',rootFor:()=>'/synthetic/data',runtimeBindings,launchDockerRuntimeNoRestart,save:async(path,value)=>{saved.push({path,value});events.push('save:'+value.phase);},join:(...x)=>x.join('/'),output:'/synthetic/output',runtimePolicySequence:0,initialRuntimeLaunch:async(name,start)=>{assert.equal(name,'mac-docker');events.push('initial-clock');return start();},started};
 const startDocker=Function(...Object.keys(deps),actualStart+'\nreturn startDocker;')(...Object.values(deps));
 return {events,runtimeBindings,started,saved,run:()=>startDocker('mac-docker',null,null,initial,async()=>{events.push('fence');if(failFence)throw Error('injected changed restored copy');})};
}

async function actualStartIntegrationCase(){
 const f=actualFixture({initial:true});await f.run();assert(f.events.indexOf('create')<f.events.indexOf('inspect'));assert(f.events.indexOf('save:before-start')<f.events.indexOf('fence'));assert(f.events.indexOf('fence')<f.events.indexOf('initial-clock'));assert(f.events.indexOf('initial-clock')<f.events.indexOf('start'));assert.equal(f.runtimeBindings.get('mac-docker').cid,cid);assert.equal(f.saved.length,2);
 const refused=actualFixture({failFence:true});await assert.rejects(refused.run(),/changed restored copy/);assert(!refused.events.includes('start'));assert.equal(refused.started.size,0);
 const bad=actualFixture({afterBadPolicy:true});await assert.rejects(bad.run(),/restart policy/);assert(bad.started.has('mac-docker'),'Cleanup still owns an actor whose post-inspection failed');
 const closing=actualFixture({phase:'closeout'});await assert.rejects(closing.run(),/unavailable during closeout/);assert.equal(closing.events.filter(x=>x==='create').length,0);
}

async function actualStopAfterPolicyDriftCase(){
 const calls=[];let running=true;
 const docker=async(argv,remote)=>{assert.equal(remote,false);calls.push(argv);
  if(argv[0]==='stop'){assert.deepEqual(argv,['stop','--time','20',cid]);running=false;return cid+'\n';}
  assert.equal(argv[0],'inspect');assert.equal(argv[1],cid);
  if(argv[3]==='{{json .State}}')return wire({Running:running,ExitCode:0,OOMKilled:false});
  assert.equal(argv[3],dockerRuntimeInspectTemplate);const value=inspect(running);value.HostConfig.RestartPolicy.Name='always';return wire(value);
 };
 const need=(ok,message)=>{if(!ok)throw Error(message);},runtimeBindings=new Map([['mac-docker',bound]]);
 const ownedCode=actualSection(' async function ownedRuntime(name,','\n async function macBootstrapPrepare(');
 const ownership=Function('need','runtimeBindings','docker','dockerRuntimeInspectTemplate','validateDockerRuntimeInspection','validateDockerRuntimeIdentityForCleanup',ownedCode+'\nreturn {ownedRuntime,ownedRuntimeForStop};')(need,runtimeBindings,docker,dockerRuntimeInspectTemplate,validateDockerRuntimeInspection,validateDockerRuntimeIdentityForCleanup);
 const stoppedCode=actualSection(' async function stopped(name,prefixHint=null){','\n async function request(');
 const deps={need,...ownership,docker,logs:async()=>'{"event":"synthetic-stopped"}',save:async()=>{},join:(...x)=>x.join('/'),output:'/synthetic/output',stoppedReceipt:()=>({event:'synthetic-stopped'}),rememberLifecycleStop:(name,value)=>value};
 const stopped=Function(...Object.keys(deps),stoppedCode+'\nreturn stopped;')(...Object.values(deps));
 await assert.rejects(stopped('mac-docker','synthetic-stop'),/restart policy/);assert.equal(running,false);assert(calls.some(x=>x[0]==='stop'),'Policy drift cannot bypass the owned CID stop, and still remains FAILED');
}

async function actualStoppedNetworkProjectionCase(){
 const network='synthetic-partition',networkBinding={...bound,name:'nano-qualification-vps-'+uuid};
 const code=actualSection(' async function stoppedRuntimeNetworkMembership(name){','\n async function macBootstrapPrepare(');
 const make=edit=>{const calls=[],docker=async(argv,remote)=>{calls.push(argv);assert.equal(remote,true);assert.deepEqual(argv,['inspect',cid,'--format',dockerRuntimeNetworkInspectTemplate]);const value=inspect();value.Name='/'+networkBinding.name;value.State.Status='exited';value.HostConfig.RestartPolicy.Name='always';value.NetworkSettings={Networks:{[network]:{NetworkID:'synthetic'}}};edit?.(value);return wire(value);};
  const deps={need:(ok,msg)=>{if(!ok)throw Error(msg);},runtimeBindings:new Map([['vps-docker',networkBinding]]),docker,dockerRuntimeNetworkInspectTemplate,validateDockerRuntimeIdentityForCleanup};
  return {calls,run:()=>Function(...Object.keys(deps),code+'\nreturn stoppedRuntimeNetworkMembership;')(...Object.values(deps))('vps-docker')};};
 const good=make();assert.deepEqual(await good.run(),{cid,networks:{[network]:{NetworkID:'synthetic'}}});assert.equal(good.calls.length,1);
 for(const edit of [v=>v.Id=otherCid,v=>v.Config.Labels['nano.fresh.campaign']='foreign',v=>v.State.Running=true,v=>v.State.Restarting=true,v=>v.State.Pid=1,v=>v.Config.Env=['NEVER READ']])await assert.rejects(make(edit).run());
}

function actualPartitionCleanupOrderCase(){
 const cleanup=actualSection("  if(partitionNetworkCreated)await terminalStage(","\n  await terminalStage('post-cleanup-linux-fence'");
 for(const marker of ['network.Labels',"stoppedRuntimeNetworkMembership('vps-docker')","['network','disconnect',partitionNetwork,stopped.cid]"])assert.equal(cleanup.split(marker).length,2,'Actual cleanup ordering marker must exist exactly once');
 assert(cleanup.indexOf('network.Labels')<cleanup.indexOf("stoppedRuntimeNetworkMembership('vps-docker')"));assert(cleanup.indexOf('stoppedRuntimeNetworkMembership')<cleanup.indexOf("['network','disconnect',partitionNetwork,stopped.cid]"));assert(!cleanup.includes("['inspect',found"),'No full runtime inspection through the mutable name');
 const whitelist=actualSection(' function exactOwnedCleanup(argv,remote){','\n const docker=');assert(whitelist.includes('dockerRuntimeNetworkInspectTemplate')&&whitelist.includes('binding.cid'));
}

function actualOwnershipHandoffCase(){
 const fresh=source.indexOf('async function runFreshCampaign(args){'),publish=source.indexOf('await publishOperatorOwnership();',fresh),software=source.indexOf('initialSoftware=await software();',fresh);assert(fresh>=0&&publish>fresh&&software>publish);
 const wrapper=actualSection(' const docker=async(a,remote=false,timeout=30000,input=null)=>{','\n const uuid=');
 for(const marker of ['need(operatorOwnershipReady',"a[0]==='create'"])assert.equal(wrapper.split(marker).length,2,'Actual ownership ordering marker must exist exactly once');
 assert(wrapper.indexOf('need(operatorOwnershipReady')<wrapper.indexOf("a[0]==='create'"));assert(wrapper.includes('ownedRuns.push({name,remote})'));
 const cleanup=actualSection(' function exactOwnedCleanup(argv,remote){','\n const docker=');assert(cleanup.includes('runtimeBindings'));assert(cleanup.includes('binding.cid'));assert(cleanup.includes('dockerRuntimeInspectTemplate'));assert(!cleanup.includes("argv[0]==='start'"));
}

test('Docker runtime no-restart parser rejects policy/state/identity/projection/CID violations',()=>{parserCases();actualOwnershipHandoffCase();actualPartitionCleanupOrderCase();});
test('Docker runtime guarded create→inspect→fence/clock→CID start→inspect transitions and actual run integration',async()=>{
 for(const run of [cleanTransitionCase,beforeStartRefusalsCase,afterStartRefusalsCase,createArgvAndOneStartCase,pendingStartJoinCase,actualStartIntegrationCase,actualStopAfterPolicyDriftCase,actualStoppedNetworkProjectionCase])await run();
});
