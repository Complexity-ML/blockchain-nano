import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {fixtureStake,majorityIslandPolicy,majorityCutDuration} from './majority-island.mjs';
import {partitionRemote} from './partition.mjs';
const root='0x'+'a'.repeat(64),names=['mac-native','mac-docker','vps-docker'];
function fixture(){
 const state=Buffer.alloc(176+64*64+100+64*121),hist=176+64*64,begin=hist+100;
 state.writeUInt32LE(begin,hist+88);state.writeUInt32LE(state.length,hist+92);
 for(let i=0;i<64;i++){state.writeBigUInt64LE(32n,begin+i*121+80);state.writeBigUInt64LE(2n**64n-1n,begin+i*121+105);}
 const profile={format:'nano-capella-profile-v1',common:[{stateLimits:['0x40'],ffg:['0x8']}]};
 const initial={anchor:root,blocks:[[root,[],['0x'+state.toString('hex')]]]};
 const manifest={fork:'capella',syntheticValidators:64,nodes:names.map((name,n)=>({name,indices:Array.from({length:64},(_,i)=>i).filter(i=>i%3===n).map(String)}))};
 const args=()=>{const profileBytes=Buffer.from(JSON.stringify(profile)),initialBytes=Buffer.from(JSON.stringify(initial));const sha=b=>'0x'+createHash('sha256').update(b).digest('hex');return {profileBytes,initialBytes,configuration:{profile:{sha256:sha(profileBytes)},initial:{sha256:sha(initialBytes)}},manifest};};
 return {state,begin,initial,manifest,args,update:()=>{initial.blocks[0][2][0]='0x'+state.toString('hex');}};
}
test('stake comes from hash-bound SSZ effective balances and exact ownership',()=>{
 const f=fixture(),stake=fixtureStake(f.args());assert.equal(stake.majorityStake,'1376');assert.equal(stake.minorityStake,'672');
});
test('43 keys with insufficient stake cannot create a majority',()=>{
 const f=fixture();for(let i=2;i<64;i+=3)f.state.writeBigUInt64LE(64n,f.begin+i*121+80);f.update();assert.throws(()=>fixtureStake(f.args()),/strict majority/);
});
for(const mode of ['hash','duplicate','inactive','slashed','bounds'])test(mode+' fixture rejects before network mutation',()=>{
 const f=fixture();if(mode==='duplicate')f.manifest.nodes[1].indices[0]='0';
 if(mode==='inactive')f.state.writeBigUInt64LE(1n,f.begin+97);
 if(mode==='slashed')f.state[f.begin+88]=1;
 if(mode==='bounds')f.state.writeUInt32LE(0,176+64*64+92);
 f.update();const args=f.args();if(mode==='hash')args.configuration.initial.sha256='0x'+'0'.repeat(64);
 assert.throws(()=>fixtureStake(args));
});
function policyFixture(){
 const events=[],stake=fixtureStake(fixture().args()),peerIds={'mac-native':'native','mac-docker':'docker'};
 const policy=majorityIslandPolicy({stake,peerIds,record:async e=>events.push(e)});
 const rows=(majority='2',minority='2',isolated=false)=>names.map((name,i)=>({name,peers:isolated&&i===2?'0x0':'0x2',peerId:i===0?'native':i===1?'docker':'vps',connectedPeerIds:i===0?['docker']:i===1?['native']:[],progress:{headSlot:'32'},checkpoint:{epoch:i===2?minority:majority,root}}));
 return {events,policy,rows};
}
test('majority must finalize beyond conservative received-commitment ceiling',async()=>{
 const f=policyFixture();await f.policy.before(f.rows());await f.policy.observe(f.rows('5','3',true));await assert.rejects(f.policy.finish(),/No majority finality/);
 await f.policy.observe(f.rows('6','3',true));await f.policy.finish();assert.equal(f.policy.recoveryFloor(),6n);
});
test('minority cannot raise its own ceiling on later reads',async()=>{
 const f=policyFixture();await f.policy.before(f.rows());await f.policy.observe(f.rows('2','3',true));const rows=f.rows('6','6',true);rows[2].progress.headSlot='80';await assert.rejects(f.policy.observe(rows),/Minority finalized/);
});
test('peer count alone does not prove a connected majority',async()=>{
 const f=policyFixture();await f.policy.before(f.rows());const rows=f.rows('6','3',true);rows[0].connectedPeerIds=['vps'];await assert.rejects(f.policy.observe(rows),/Direct majority link/);
});
for(const mode of ['no-progress','minority','pass'])test(mode+' integration restores IP and requires newer common finality',async()=>{
 const f=policyFixture();let attached=true,cut=false,time=0;const container='nano-qualification-vps-test',network=container+'-net',revision='a'.repeat(40),events=[];
 const docker=async args=>{if(args[0]==='inspect')return JSON.stringify({State:{Running:true},Config:{Labels:{'nano.qualification':'multihost','org.opencontainers.image.revision':revision}},NetworkSettings:{Networks:attached?{[network]:{IPAddress:'172.30.0.2'}}:{}}});if(args[1]==='disconnect'){attached=false;cut=true;}if(args[1]==='connect')attached=true;};
 const sample=async()=>!cut?f.rows():!attached?f.rows(mode==='no-progress'?'5':'6',mode==='minority'?'6':'3',true):f.rows('7','7');
 const call=partitionRemote({docker,container,network,revision,sample,record:async e=>events.push(e),policy:f.policy,clock:()=>time,wait:async ms=>{time+=ms;},cutMs:10000,recoveryMs:10000});
 if(mode==='pass'){await call;assert.equal(events.at(-1).event,'partition-recovery-bound');}else await assert.rejects(call,/No majority finality|Minority finalized/);
 assert.equal(attached,true);
});

test('longer sustained cut is explicit and never changes recovery budget',()=>{
 assert.equal(majorityCutDuration(),192000);assert.equal(majorityCutDuration('384000'),384000);
});
test('invalid or unbounded majority cut refuses before execution',()=>{
 for(const value of ['0','191999','600001','Infinity','1e9','-1','192000,384000'])assert.throws(()=>majorityCutDuration(value));
});
