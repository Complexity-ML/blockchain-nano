import test from 'node:test';
import assert from 'node:assert/strict';
import {smokeCandidateRuntime} from './check-candidate.mjs';

const IMAGE='sha256:'+'a'.repeat(64),LABEL='org.nanobees.argument-smoke.owner';
const ROLES=['nano','nano-p2p','nano-storage-maintenance','nano-bootstrap-archive-maintenance','nano-capella-duty-readiness'];
function terminal(name){
 if(name==='nano')return {code:0,stdout:'Nano local chain tools\nUsage: nano <init|status|audit>\n',stderr:''};
 if(name==='nano-p2p')return {code:1,stdout:'',stderr:'Usage: nano-p2p arguments\n'};
 if(name==='nano-storage-maintenance')return {code:1,stdout:'',stderr:'Usage: compact-owned-copy arguments\n'};
 const body=name==='nano-bootstrap-archive-maintenance'?{completed:false,refusedStage:'arguments',runtimeStarted:false,productionQualified:false}:{status:'REFUSED_NO_AUTOMATIC_RETRY',refusedStage:'request',reason:'validation_or_io_refused',probeCompleted:false,keysLoaded:false,runtimeStarted:false};
 return {code:1,stdout:JSON.stringify(body)+'\n',stderr:''};
}
function fixture({user='10001:10001',volumes=null,lie=null,mode=null,absentStdout=''}={}){
 const state={calls:[],containers:new Map(),runs:[],removed:[],foreign:{Id:'f'.repeat(64),Name:'/unrelated-foreign',Image:IMAGE,Config:{Labels:{[LABEL]:'unrelated-owner'}}}};
 const foreignBefore=structuredClone(state.foreign),timeout=Object.assign(Error('synthetic Docker run timeout'),{name:'TimeoutError'});
 const ok=(stdout='')=>({code:0,stdout,stderr:''});
 const invoke=async argv=>{
  const a=[...argv];state.calls.push(a);
  if(a[0]==='image'&&a[1]==='inspect')return ok(JSON.stringify({Id:IMAGE,Os:'linux',Architecture:'arm64',Config:{User:user,Volumes:volumes}}));
  if(a[0]==='container'&&a[1]==='inspect'){
   const found=state.containers.get(a[2]);return found?ok(JSON.stringify(found)):{code:1,stdout:absentStdout,stderr:'Error response from daemon: No such container: '+a[2]+'\n'};
  }
  if(a[0]==='run'){
   const name=a[a.indexOf('--name')+1],assignment=a[a.indexOf('--label')+1],separator=assignment.indexOf('=');assert.equal(assignment.slice(0,separator),LABEL);const owner=assignment.slice(separator+1);
   const binary=a.find(x=>x.startsWith('/usr/local/bin/')),role=binary?.slice('/usr/local/bin/'.length);assert.ok(ROLES.includes(role));
   const cid=(state.runs.length+1).toString(16).padStart(64,'0'),record={Id:cid,Name:'/'+name,Image:IMAGE,Config:{Labels:{[LABEL]:owner}}};state.runs.push({name,owner,cid,role,args:a});state.containers.set(name,record);
   if(mode==='timeout-owned'||mode==='timeout-rm-refused')throw timeout;
   if(mode==='timeout-foreign'){record.Config.Labels[LABEL]='foreign-owner';throw timeout;}
   state.containers.delete(name);const result=terminal(role);
   if(lie&&role===lie.role){const body=JSON.parse(result.stdout);body[lie.field]=lie.value;result.stdout=JSON.stringify(body)+'\n';}
   return result;
  }
  if(a[0]==='container'&&a[1]==='rm'){
   assert.deepEqual(a.slice(0,3),['container','rm','--force']);const entry=[...state.containers.entries()].find(([,c])=>c.Id===a[3]);assert.ok(entry,'cleanup must address the held owned CID');state.removed.push(a[3]);
   if(mode==='timeout-rm-refused')return {code:1,stdout:'',stderr:'synthetic remove refusal\n'};
   state.containers.delete(entry[0]);return ok(a[3]+'\n');
  }
  throw Error('Unexpected Docker command in smoke fixture: '+a[0]);
 };
 return {state,timeout,foreignBefore,invoke,run:()=>smokeCandidateRuntime(IMAGE,{invoke})};
}

test('runtime smoke covers the five argument-only roles with isolated flags and owned closures',async()=>{
 const f=fixture(),r=await f.run();assert.equal(r.format,'nano-candidate-runtime-arguments-v1');assert.equal(r.argumentOnlyVerified,true);assert.equal(r.image,IMAGE);assert.equal(r.architecture,'arm64');assert.equal(r.executablesChecked,5);assert.deepEqual(r.records.map(v=>v.name),ROLES);assert.deepEqual(r.records.map(v=>v.exitCode),[0,1,1,1,1]);assert.ok(r.records.every(v=>v.argumentOnlyOracleVerified===true));
 assert.equal(r.supervisorEnvironmentUnset,true);assert.equal(r.dataArgumentsProvided,false);assert.equal(r.dataMountsProvided,false);assert.equal(r.readOnlyRootfsConfigured,true);assert.equal(r.networkMode,'none');assert.equal(r.runtimeDutiesAuthorized,false);assert.equal(r.productionQualified,false);
 assert.equal(f.state.runs.length,5);assert.equal(new Set(f.state.runs.map(v=>v.name)).size,5);assert.ok(f.state.runs.every(v=>/^[0-9a-f]{32}$/.test(v.owner)));
 for(const {args:a,role}of f.state.runs){
  for(const flag of ['--rm','--pull=never','--no-healthcheck','--read-only'])assert.ok(a.includes(flag));
  for(const [flag,value]of [['--network','none'],['--user','10001:10001'],['--cap-drop','ALL'],['--security-opt','no-new-privileges'],['--memory','512m'],['--cpus','1'],['--pids-limit','32'],['--workdir','/'],['--entrypoint','/usr/bin/env']])assert.equal(a[a.indexOf(flag)+1],value);
  assert.equal(a.some(v=>['--mount','--volume','-v','--publish','-p'].includes(v)),false);assert.deepEqual(a.slice(a.indexOf(IMAGE)+1),['-u','NANO_SUPERVISOR_PID','/usr/local/bin/'+role,...(role==='nano'?['--help']:[])]);
 }
 assert.equal(r.closures.length,5);assert.deepEqual(r.closures.map(v=>v.name),f.state.runs.map(v=>v.name));assert.ok(r.closures.every(v=>v.ownedContainerAbsent===true));assert.equal(f.state.containers.size,0);assert.deepEqual(f.state.foreign,f.foreignBefore);
});

test('runtime smoke refuses root-user images or declared data volumes before any RUN',async()=>{
 for(const scenario of [{options:{user:'0:0'},reason:'Candidate Linux image/user identity differs'},{options:{volumes:{'/var/lib/nano':{}}},reason:'Argument-only smoke refuses automatic image volumes'}]){
  const f=fixture(scenario.options);await assert.rejects(f.run(),error=>error.message===scenario.reason);assert.equal(f.state.calls.length,1);assert.deepEqual(f.state.calls[0],['image','inspect',IMAGE,'--format','{{json .}}']);assert.equal(f.state.runs.length,0);assert.equal(f.state.removed.length,0);assert.deepEqual(f.state.foreign,f.foreignBefore);
 }
});

test('runtime smoke refuses wrong types or lying maintenance started/key flags',async()=>{
 for(const lie of [{role:'nano-capella-duty-readiness',field:'keysLoaded',value:0},{role:'nano-capella-duty-readiness',field:'keysLoaded',value:true},{role:'nano-bootstrap-archive-maintenance',field:'runtimeStarted',value:'false'},{role:'nano-bootstrap-archive-maintenance',field:'runtimeStarted',value:true}]){
  const f=fixture({lie});await assert.rejects(f.run(),error=>error.message==='Typed maintenance refusal field differs: '+lie.field&&error.secondaryFailures.length===0);assert.equal(f.state.runs.at(-1).role,lie.role);assert.equal(f.state.containers.size,0);assert.deepEqual(f.state.foreign,f.foreignBefore);
 }
});

test('runtime timeout preserves the first error and cleans only a verified owned CID',async()=>{
 const owned=fixture({mode:'timeout-owned'});await assert.rejects(owned.run(),error=>error===owned.timeout&&error.secondaryFailures.length===0);assert.equal(owned.state.runs.length,1);assert.deepEqual(owned.state.removed,[owned.state.runs[0].cid]);assert.equal(owned.state.containers.size,0);assert.deepEqual(owned.state.foreign,owned.foreignBefore);
 const foreign=fixture({mode:'timeout-foreign'});await assert.rejects(foreign.run(),error=>error===foreign.timeout&&error.secondaryFailures.length===1&&error.secondaryFailures[0].stage==='owned-cleanup'&&error.secondaryFailures[0].message==='Smoke container ownership differs');assert.equal(foreign.state.removed.length,0);assert.equal(foreign.state.containers.size,1);assert.equal([...foreign.state.containers.values()][0].Config.Labels[LABEL],'foreign-owner');assert.deepEqual(foreign.state.foreign,foreign.foreignBefore);
 const refused=fixture({mode:'timeout-rm-refused'});await assert.rejects(refused.run(),error=>error===refused.timeout&&error.secondaryFailures.length===1&&error.secondaryFailures[0].stage==='owned-cleanup'&&error.secondaryFailures[0].message==='Owned smoke container cleanup refused');assert.deepEqual(refused.state.removed,[refused.state.runs[0].cid]);assert.equal(refused.state.containers.size,1);assert.deepEqual(refused.state.foreign,refused.foreignBefore);
});

test('runtime absent-container inspection accepts only whitespace stdout with the exact refusal protocol',async()=>{
 for(const absentStdout of ['\n','\r\n',' \t\r\n']){
  const f=fixture({absentStdout}),r=await f.run();assert.equal(r.argumentOnlyVerified,true);assert.equal(r.executablesChecked,5);assert.equal(r.closures.length,5);assert.ok(r.closures.every(v=>v.ownedContainerAbsent===true));assert.equal(f.state.runs.length,5);assert.equal(f.state.containers.size,0);assert.deepEqual(f.state.foreign,f.foreignBefore);
 }
 for(const absentStdout of ['{}','\n{}\r\n','not empty\n']){
  const f=fixture({absentStdout});await assert.rejects(f.run(),error=>error.message==='Owned smoke container inspection refused'&&error.secondaryFailures.length===0);assert.equal(f.state.calls.length,2);assert.equal(f.state.runs.length,0);assert.equal(f.state.removed.length,0);assert.equal(f.state.containers.size,0);assert.deepEqual(f.state.foreign,f.foreignBefore);
 }
});
