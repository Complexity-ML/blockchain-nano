import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {SYNTHETIC_BASE_FORK,requireFreshSyntheticBaseFork,verifySwapBalanceDelta} from './fork-invariants.mjs';

const report=JSON.parse(await readFile(new URL('../../docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json',import.meta.url)));
const anchor=SYNTHETIC_BASE_FORK,other='0x'+'99'.repeat(32);
function fixture({chain=31337,version='anvil/v1.5.1',infoChange,headerChange,latestChange,onLatest}={}){
 const info=structuredClone(report.anvil.info);infoChange?.(info);const calls=[];
 const header={number:anchor.blockNumber,hash:anchor.blockHash};headerChange?.(header);
 return {info,header,calls,rpc:{
  getChainId:async()=>{calls.push('chainId');return chain;},
  request:async({method})=>{calls.push(method);if(method==='web3_clientVersion')return version;
   assert.equal(method,'anvil_nodeInfo');return info;},
  getBlock:async(q)=>{calls.push(q);if(q.blockNumber!==undefined){assert.equal(q.blockNumber,anchor.blockNumber);return header;}
   assert.equal(q.blockTag,'latest');onLatest?.(info,header);const latest={number:anchor.blockNumber,hash:anchor.blockHash};
   latestChange?.(latest);return latest;}
 }};
}

test('private fork anchor equals the retained public initial checkpoint',async()=>{
 assert.equal(BigInt(report.anvil.info.forkConfig.forkBlockNumber),anchor.blockNumber);
 assert.equal(report.anvil.info.currentBlockHash,anchor.blockHash);
 const f=fixture(),result=await requireFreshSyntheticBaseFork(f.rpc);
 assert.equal(result.freshInitialHeadMatched,true);assert.equal(result.anchor.blockHash,anchor.blockHash);
 assert.equal(f.calls.length,5);
 assert.ok(f.calls.every(q=>typeof q!=='string'||['chainId','web3_clientVersion','anvil_nodeInfo'].includes(q)));
});

test('wrong fork identity refuses before a subsequent mutation can run',async(t)=>{
 const cases=[
  ['chain',{chain:8453}],['client',{version:'other/client'}],
  ['no fork',{infoChange:i=>delete i.forkConfig}],
  ['wrong base height',{infoChange:i=>i.forkConfig.forkBlockNumber++}],
  ['wrong node environment',{infoChange:i=>i.environment.chainId=1}],
  ['already advanced',{infoChange:i=>i.currentBlockNumber='0x31c1a8f'}],
  ['wrong initial hash',{infoChange:i=>i.currentBlockHash=other}],
  ['replaced anchor',{headerChange:h=>h.hash=other}],
  ['head changed during check',{latestChange:h=>h.hash=other}],
 ];
 for(const[name,options]of cases)await t.test(name,async()=>{
  const f=fixture(options);let mutationReached=false;
  await assert.rejects((async()=>{await requireFreshSyntheticBaseFork(f.rpc);mutationReached=true;})());
  assert.equal(mutationReached,false);
 });
});

test('fork observations are detached from provider caches across later awaits',async()=>{
 const f=fixture({onLatest:(info,header)=>{info.currentBlockHash=other;header.hash=other;}});
 const result=await requireFreshSyntheticBaseFork(f.rpc);
 assert.equal(result.info.currentBlockHash,anchor.blockHash);assert.equal(result.anchor.blockHash,anchor.blockHash);
 assert.equal(f.info.currentBlockHash,other);assert.equal(f.header.hash,other);
});

test('quote100 minimum99 output1 is refused despite a positive old balance',()=>{
 assert.throws(()=>verifySwapBalanceDelta({amountIn:10n,minimumOutput:99n,
  before:{input:100n,output:1000000n},after:{input:90n,output:1000001n}}),/below encoded minimum/);
 const result=verifySwapBalanceDelta({amountIn:10n,minimumOutput:99n,
  before:{input:100n,output:1000000n},after:{input:90n,output:1000099n}});
 assert.equal(result.amountOutReceived,99n);assert.equal(result.minimumOutput,99n);assert.equal(result.inputDelta,10n);
});

test('swap delta retains exact-input and positive-output refusals',()=>{
 for(const change of [v=>v.after.input=91n,v=>v.after.output=v.before.output,v=>v.amountIn=0n,
                      v=>v.minimumOutput=-1n,v=>v.before.output=-1n,v=>v.after.output=99]){
  const value={amountIn:10n,minimumOutput:0n,before:{input:100n,output:0n},after:{input:90n,output:99n}};
  change(value);assert.throws(()=>verifySwapBalanceDelta(value));
 }
});
