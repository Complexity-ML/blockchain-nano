// Pure transport/authority models only. These bytes are not signed consensus
// fixtures; actual signed-state rotation and aggregate checks belong to --fresh.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createRotationCapture,freshRotationParameters,freshRotationOwnership,freshPartitionFinalityObserver,rotationNodes,waitRotationWindow} from './rotation.mjs';
import {partitionRemote} from './partition.mjs';
const root=n=>'0x'+n.toString(16).padStart(64,'0');
const profile=()=>({format:'nano-capella-profile-v1',common:[{ffg:['0x8'],epochSync:['0x8','0x8']},'0x1770',null,null,null,null,null,'0x6',null,null,null,null]});
const encoded=x=>Buffer.from(JSON.stringify(x));
test('fresh boundary/cut come from the pinned profile and never the old literal128',()=>{
 const p=freshRotationParameters(encoded(profile()));assert.deepEqual(p,{slotsPerEpoch:8,epochsPerPeriod:8,periodSlots:64,boundarySlot:128,leadSlots:8,slotMs:6000,cutMs:102000});
 const alternate=profile();alternate.common[0].epochSync[1]='0x10';assert.equal(freshRotationParameters(encoded(alternate)).boundarySlot,256);
 for(const mutate of [x=>x.common[0].epochSync[0]='0x4',x=>x.common[7]='0x5',x=>x.common[0].ffg[0]='0x08']){const x=profile();mutate(x);assert.throws(()=>freshRotationParameters(encoded(x)));}
});
test('a reviewed non-default window is fixed and cannot silently move after a miss',async()=>{
 const heads=slot=>rotationNodes.map(name=>({name,root:root(slot),slot}));
 assert.equal((await waitRotationWindow({sample:async()=>heads(240),record:async()=>{},boundary:256,lead:16}))[0].slot,240);
 await assert.rejects(waitRotationWindow({sample:async()=>heads(256),record:async()=>{},boundary:256,lead:16}),/missed/);
});
function identityFixture(){
 const domain={format:'nano-isolated-capella-fixture-domain-v1',synthetic:true,productionQualified:false,publicKeys:Array.from({length:65},(_,index)=>({index,publicKey:'0x'+(index+1).toString(16).padStart(96,'0')}))};
 const prepared={nodes:rotationNodes.map((name,n)=>{const indices=Array.from({length:n===2?33:16},(_,i)=>i+(n===0?0:n===1?16:32));return {name,indices,signerIdentities:indices.map(i=>({keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_'+i,publicKey:domain.publicKeys[i].publicKey}))};})};return {domain,prepared};
}
test('all65 ownership rows bind explicit public identities including pending key64',()=>{
 const {domain,prepared}=identityFixture(),rows=freshRotationOwnership(domain,prepared);assert.equal(rows.length,65);assert.deepEqual(rows[64],{publicKey:domain.publicKeys[64].publicKey,keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_64',owner:'vps-docker'});
 for(const mutate of [x=>x.nodes[2].indices[32]=0,x=>x.nodes[0].signerIdentities[0].publicKey=domain.publicKeys[1].publicKey,x=>x.nodes.reverse()]){const wrong=structuredClone(prepared);mutate(wrong);assert.throws(()=>freshRotationOwnership(domain,wrong));}
});
const header=r=>({execution_optimistic:false,finalized:true,data:{canonical:true,root:r}});
const initial=()=>({knownCompletedCheckpoints:[{epoch:'3',root:root(3)}],perOwnerPreviousEpoch:Object.fromEntries(rotationNodes.map(name=>[name,'3']))});
function reader(state,first,second=first){let n=0;return async path=>path.includes('/states/')?{data:{finalized:state}}:++n===1?first:second;}
test('fault monitor accepts the authenticated completed baseline without inventing an advance',async()=>{
 const monitor=freshPartitionFinalityObserver(initial()),row=await monitor.sample('mac-native',reader({epoch:'3',root:root(3)},header(root(3))),'0x2');assert.deepEqual(row.checkpoint,{epoch:'3',root:root(3)});
 const known=await monitor.sample('mac-docker',reader({epoch:'0',root:root(0)},header(root(3))),'0x2');assert.equal(known.checkpoint.epoch,'3');assert.equal(known.source,'previously-observed-checkpoint-and-current-finalized-header');
 assert.equal(await monitor.sample('vps-docker',reader({epoch:'4',root:root(4)},header(root(4)),header(root(3))),'0x0'),null);
 await assert.rejects(monitor.sample('mac-native',reader({epoch:'2',root:root(2)},header(root(2))),'0x2'),/regressed/);
 await assert.rejects(monitor.sample('mac-native',reader({epoch:'3',root:root(3)},{...header(root(3)),execution_optimistic:true}),'0x2'),/Unauthenticated/);
});
const wire=(slot,parent)=>{const b=Buffer.alloc(300);b.writeUInt32LE(100);b.writeUInt32LE(84,180);b.writeBigUInt64LE(BigInt(slot),100);Buffer.from(parent.slice(2),'hex').copy(b,116);return b;};
test('VPS credit requires a matching in-situ digest and complete three-owner ancestry',()=>{
 const c=createRotationCapture();for(let slot=1;slot<=5;slot++){for(const name of rotationNodes.slice(0,2))c.add(name,root(slot),wire(slot,root(slot-1)));const d=c.digest(root(slot));assert.throws(()=>c.addDigest('vps-docker',root(slot),'f'.repeat(64),d.bytes),/differs/);c.addDigest('vps-docker',root(slot),d.sha256,d.bytes);}
 const proof=c.freshProof(root(0),root(5),2);assert.equal(Object.keys(proof).length,5);assert.deepEqual(proof[root(5)].observedBy,rotationNodes);assert.throws(()=>c.freshProof(root(0),root(5),3),/Insufficient/);
 const missing=createRotationCapture();for(const name of rotationNodes.slice(0,2))missing.add(name,root(1),wire(1,root(0)));assert.throws(()=>missing.freshProof(root(0),root(1),0),/three-owner/);
});
function partitionFixture(wrong=false){
 const freshCampaign='12345678-1234-1234-1234-123456789abc',container='nano-qualification-vps-'+freshCampaign,network=container+'-net',revision='a'.repeat(40);let attached=true,cut=false,now=0;const calls=[],events=[];
 const docker=async args=>{calls.push(args);if(args[0]==='inspect')return JSON.stringify({State:{Running:true},Config:{Labels:{'nano.qualification':'multihost','org.opencontainers.image.revision':revision,'nano.fresh.campaign':wrong?'another':freshCampaign}},NetworkSettings:{Networks:attached?{[network]:{IPAddress:'172.30.0.2'}}:{}}});if(args[1]==='disconnect'){attached=false;cut=true;}if(args[1]==='connect')attached=true;};
 const sample=async()=>rotationNodes.map((name,i)=>({name,peers:!attached&&i===2?'0x0':'0x1',checkpoint:{epoch:cut&&attached?'3':'2',root:root(cut&&attached?3:2)}}));
 return {args:{docker,container,network,revision,freshCampaign,sample,record:async e=>events.push(e),cutMs:10000,recoveryMs:10000,clock:()=>now,wait:async ms=>{now+=ms;}},attached:()=>attached,calls,events};
}
test('fresh UUID target needs its own campaign label and cutoff runs before reconnect',async()=>{
 const f=partitionFixture();let cutoff=false;f.args.afterCut=async()=>{assert.equal(f.attached(),false);cutoff=true;};await partitionRemote(f.args);assert.equal(cutoff,true);assert.equal(f.attached(),true);assert.equal(f.events.at(-1).event,'partition-recovery-bound');
 const wrong=partitionFixture(true);await assert.rejects(partitionRemote(wrong.args),/provenance/);assert.ok(wrong.calls.every(c=>c[0]==='inspect'));
 const unscoped=partitionFixture();delete unscoped.args.freshCampaign;await assert.rejects(partitionRemote(unscoped.args),/target/);assert.equal(unscoped.calls.length,0);
});
test('failed isolated cutoff restores the exact network but preserves failure',async()=>{
 const f=partitionFixture();f.args.afterCut=async()=>{assert.equal(f.attached(),false);throw Error('boundary not crossed');};await assert.rejects(partitionRemote(f.args),/boundary not crossed/);assert.equal(f.attached(),true);assert.ok(!f.events.some(e=>e.event==='partition-recovery-bound'));
});
