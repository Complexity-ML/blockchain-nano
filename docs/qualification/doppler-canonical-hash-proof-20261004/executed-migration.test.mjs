import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {encodeEventTopics,decodeFunctionData,encodeFunctionResult} from 'viem';
import {airlockAbi} from '@whetstone-research/doppler-sdk/evm';
import {verifyMigration} from './verify-migration.mjs';
const report=JSON.parse(await readFile(new URL('../../docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json',import.meta.url)));
const proof=report.steps.find(x=>x.name==='migration-state');
const hash=proof.blockHash,other='0x'+'99'.repeat(32),number=BigInt(proof.blockNumber);
const params={tokenAddress:report.predicted.tokenAddress,numeraire:report.configuration.numeraire,migrationHash:proof.migrationHash,governanceType:'noOp'};
function fixture(change={}) {
 let headers=0;const calls=[];
 const receipt={status:'success',transactionHash:params.migrationHash,blockNumber:number,blockHash:hash,logs:[{address:proof.airlock,blockNumber:number,blockHash:hash,transactionHash:params.migrationHash,removed:false,data:'0x',topics:encodeEventTopics({abi:airlockAbi,eventName:'Migrate',args:{asset:params.tokenAddress,pool:proof.pair}})}]};
 change.receipt?.(receipt);
 const rpc={getChainId:async()=>31337,getTransactionReceipt:async()=>receipt,getBlock:async()=>({number,hash:change.header?.(++headers)??hash}),request:async(q)=>{
  if(q.method==='web3_clientVersion')return 'anvil/test';
  assert.equal(q.method,'eth_call');calls.push(q);
  assert.deepEqual(q.params[1],{blockHash:hash,requireCanonical:true});
  if(change.rejectCall)throw new Error('Canonical block unavailable');
  let abi,value;
  if(q.params[0].to.toLowerCase()===proof.airlock.toLowerCase()){abi=airlockAbi;value=proof.asset;}
  else {
   abi=[{type:'function',name:'token0',inputs:[],outputs:[{type:'address'}]},{type:'function',name:'token1',inputs:[],outputs:[{type:'address'}]},{type:'function',name:'getReserves',inputs:[],outputs:[{type:'uint112'},{type:'uint112'},{type:'uint32'}]},{type:'function',name:'totalSupply',inputs:[],outputs:[{type:'uint256'}]},{type:'function',name:'balanceOf',inputs:[{type:'address'}],outputs:[{type:'uint256'}]}];
   const {functionName}=decodeFunctionData({abi,data:q.params[0].data});value={token0:proof.token0,token1:proof.token1,getReserves:proof.reserves,totalSupply:proof.totalSupply,balanceOf:proof.beneficiaryLP}[functionName];
  }
  const {functionName}=decodeFunctionData({abi,data:q.params[0].data});return encodeFunctionResult({abi,functionName,result:value});
 }};
 return {rpc,calls};
}
test('all financial reads use the canonical receipt hash, never a height-only selector',async()=>{const f=fixture();const r=await verifyMigration(f.rpc,params);assert.equal(f.calls.length,6);assert.equal(r.canonicalBlockHashPinned,true);assert.equal(r.blockHash,hash);});
test('receipt from a replaced block is rejected before state reads',async()=>{const f=fixture({header:()=>other});await assert.rejects(verifyMigration(f.rpc,params),/not canonical/);assert.equal(f.calls.length,0);});
test('reorg at the end of verification invalidates the complete financial proof',async()=>{const f=fixture({header:n=>n===2?other:hash});await assert.rejects(verifyMigration(f.rpc,params),/changed during/);assert.equal(f.calls.length,6);});
test('canonical hash unavailable during state reads is never retried by height',async()=>{const f=fixture({rejectCall:true});await assert.rejects(verifyMigration(f.rpc,params),/Canonical block unavailable/);assert.equal(f.calls.length,1);});
test('removed or cross-transaction/block logs and swapped event arguments are refused',async()=>{
 for(const mutate of [r=>r.transactionHash=other,r=>r.logs[0].removed=true,r=>r.logs[0].transactionHash=other,r=>r.logs[0].blockHash=other,r=>r.logs[0].blockNumber=number+1n,r=>r.logs[0].topics=encodeEventTopics({abi:airlockAbi,eventName:'Migrate',args:{asset:proof.pair,pool:params.tokenAddress}})]){
  const f=fixture({receipt:mutate});await assert.rejects(verifyMigration(f.rpc,params));
 }
});
