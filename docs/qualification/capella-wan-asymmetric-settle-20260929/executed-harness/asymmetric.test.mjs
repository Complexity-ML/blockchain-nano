import {test} from 'node:test';
import assert from 'node:assert/strict';
import {asymmetricRemote,createRemoteTc} from './asymmetric.mjs';
function fixture(mode='pass'){
 let time=0,active=mode==='existing',applied=false;const events=[],calls=[];
 const tc=async args=>{
  calls.push(args);
  if(args.includes('show'))return JSON.stringify(active?[{kind:'netem',handle:'7a00:',packets:mode==='no-packets'?0:42}]:[]);
  if(args.includes('add')){active=true;applied=true;if(mode==='lost-response')throw Error('SSH response lost');}
  if(args.includes('del')){if(mode==='cleanup-fails')throw Error('tc cleanup failed');active=false;}
 };
 const sample=async()=>{
  if(applied&&mode==='rpc-fails')throw Error('RPC failed');
  return ['mac-native','mac-docker','vps-docker'].map((name,i)=>({name,peers:'0x1',checkpoint:{epoch:applied&&mode==='regression'?'1':time>=6000&&mode!=='stalled'?'4':'2',root:'0x'+(applied&&mode==='conflict'&&i===2?'b':'a').repeat(64)}}));
 };
 return {args:{tc,sample,record:async e=>events.push(e),clock:()=>time,wait:async ms=>{time+=ms;},holdMs:6000,progressMs:6000},events,calls,active:()=>active};
}
test('delay carries packets and advances common finality before restoration',async()=>{
 const f=fixture();await asymmetricRemote(f.args);assert.equal(f.active(),false);
 assert.equal(f.events.at(-2).event,'asymmetric-delay-finality');assert.equal(f.events.at(-1).event,'asymmetric-delay-restored');
});
for(const [mode,error] of [['lost-response',/SSH response/],['rpc-fails',/RPC failed/],['regression',/regressed/],['conflict',/Conflicting/],['stalled',/deadline/],['no-packets',/No packets/]])test(mode+' fails and removes the delay',async()=>{
 const f=fixture(mode);await assert.rejects(asymmetricRemote(f.args),error);assert.equal(f.active(),false);
});
test('existing delay is untouched',async()=>{
 const f=fixture('existing');await assert.rejects(asymmetricRemote(f.args),/pre-existing/);assert.equal(f.active(),true);assert.equal(f.calls.length,1);
});
test('cleanup failure cannot pass',async()=>{
 const f=fixture('cleanup-fails');await assert.rejects(asymmetricRemote(f.args),/cleanup failed/);assert.equal(f.active(),true);
});
function remoteFixture(){
 const container='nano-qualification-vps-test',network=container+'-net',revision='a'.repeat(40),calls=[];
 const state={Id:'test-id',State:{Running:true,Paused:false,Pid:1234},Config:{Labels:{'nano.qualification':'multihost','org.opencontainers.image.revision':revision}},HostConfig:{NetworkMode:network},NetworkSettings:{Networks:{[network]:{IPAddress:'172.22.0.2'}}}};
 const env={sameNs:false};
 const sh=async args=>{calls.push(args);if(args.includes('readlink'))return args.at(-1)==='/proc/1/ns/net'||env.sameNs?'net:[1]':'net:[2]';return '[]';};
 return {args:{docker:async()=>JSON.stringify(state),sh,container,network,revision},state,env,calls};
}
test('remote tc command only enters validated container namespace',async()=>{
 const f=remoteFixture(),tc=await createRemoteTc(f.args);await tc(['-j','qdisc','show','dev','eth0']);
 assert.deepEqual(f.calls.at(-1),['sudo','-n','nsenter','-t','1234','-n','/usr/sbin/tc','-j','qdisc','show','dev','eth0']);
});
for(const mode of ['host','label','paused','extra-network'])test('unsafe target '+mode+' refuses tc',async()=>{
 const f=remoteFixture();
 if(mode==='host')f.env.sameNs=true;
 if(mode==='label')f.state.Config.Labels['nano.qualification']='other';
 if(mode==='paused')f.state.State.Paused=true;
 if(mode==='extra-network')f.state.NetworkSettings.Networks.extra={};
 await assert.rejects(createRemoteTc(f.args),/Unsafe|provenance/);assert.ok(f.calls.every(c=>!c.includes('nsenter')));
});
test('container restart cannot redirect subsequent tc calls',async()=>{
 const f=remoteFixture(),tc=await createRemoteTc(f.args);f.state.State.Pid=5678;
 await assert.rejects(tc(['qdisc','add']),/identity changed/);assert.ok(f.calls.every(c=>!c.includes('nsenter')));
});
