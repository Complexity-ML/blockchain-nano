import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {encodeEventTopics,decodeFunctionData,encodeFunctionResult,encodeAbiParameters,parseAbi,zeroAddress} from 'viem';
import {airlockAbi} from '@whetstone-research/doppler-sdk/evm';
import {verifyMigration} from './verify-migration.mjs';
const report=JSON.parse(await readFile(new URL('../../docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json',import.meta.url)));
const proof=report.steps.find(x=>x.name==='migration-state');
const hash=proof.blockHash,other='0x'+'99'.repeat(32),number=BigInt(proof.blockNumber);
const params={tokenAddress:report.predicted.tokenAddress,numeraire:report.configuration.numeraire,migrationHash:proof.migrationHash,governanceType:'noOp'};
const migrator=proof.asset[3],beneficiary=proof.asset[1],otherAddress='0x'+'11'.repeat(20),locker='0x'+'22'.repeat(20);
const lpAbi=parseAbi(['event Transfer(address indexed from,address indexed to,uint256 value)']);
function transfer(from,to,value,logIndex,extra={}) {
 return {address:proof.pair,blockNumber:number,blockHash:hash,transactionHash:params.migrationHash,removed:false,logIndex,data:encodeAbiParameters([{type:'uint256'}],[value]),topics:encodeEventTopics({abi:lpAbi,eventName:'Transfer',args:{from,to}}),...extra};
}
function replaceTransfer(r,index,from,to,value,extra={}){r.logs[r.logs.findIndex(l=>l.logIndex===index)]=transfer(from,to,value,index,extra);}
function fixture(change={}) {
 let headers=0;const calls=[];
 const receipt={status:'success',transactionHash:params.migrationHash,blockNumber:number,blockHash:hash,logs:[{address:proof.airlock,blockNumber:number,blockHash:hash,transactionHash:params.migrationHash,removed:false,logIndex:4,data:'0x',topics:encodeEventTopics({abi:airlockAbi,eventName:'Migrate',args:{asset:params.tokenAddress,pool:proof.pair}})},transfer(zeroAddress,zeroAddress,1000n,0),transfer(zeroAddress,migrator,1000n,1),transfer(migrator,beneficiary,950n,2),transfer(migrator,locker,50n,3)]};
 change.receipt?.(receipt);
 const rpc={getChainId:async()=>31337,getTransactionReceipt:async()=>receipt,getBlock:async()=>({number,hash:change.header?.(++headers)??hash}),request:async(q)=>{
  if(q.method==='web3_clientVersion')return 'anvil/test';
  assert.equal(q.method,'eth_call');calls.push(q);
  assert.deepEqual(q.params[1],{blockHash:hash,requireCanonical:true});
  if(change.rejectCall)throw new Error('Canonical block unavailable');
  let abi,value;
  if(q.params[0].to.toLowerCase()===proof.airlock.toLowerCase()){abi=airlockAbi;value=change.asset??proof.asset;}
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
test('a pre-existing positive LP balance cannot replace a credit in this migration transaction',async()=>{
 const f=fixture({receipt:r=>{r.logs=r.logs.slice(0,1);}});
 await assert.rejects(verifyMigration(f.rpc,params),/transaction-bound V2 LP attribution/);
});
test('V2 attribution accepts mint to migrator then beneficiary credit, without imposing a split',async()=>{
 const f=fixture({receipt:r=>{replaceTransfer(r,2,migrator,beneficiary,7n);replaceTransfer(r,3,migrator,locker,993n);r.logs.reverse();}});
 const r=await verifyMigration(f.rpc,params);
 assert.equal(f.calls.length,6);assert.equal(r.beneficiaryLP,BigInt(proof.beneficiaryLP));
 assert.deepEqual(r.lpAttribution,{migrator,mintedLP:1000n,beneficiaryLpCredit:7n,beneficiaryLpNetCredit:7n,beneficiaryLpAttributedNetCredit:7n,mintLogIndex:1,creditLogIndex:2,migrateLogIndex:4});
});
test('LP attribution rejects wrong identities, amounts, order, cancellation and malformed evidence',async(t)=>{
 const cases=[
  ['another pair mint',r=>replaceTransfer(r,1,zeroAddress,migrator,1000n,{address:otherAddress})],
  ['another pair credit',r=>replaceTransfer(r,2,migrator,beneficiary,950n,{address:otherAddress})],
  ['another beneficiary',r=>replaceTransfer(r,2,migrator,otherAddress,950n)],
  ['pool is not migrator',r=>replaceTransfer(r,1,zeroAddress,proof.asset[5],1000n)],
  ['credit from a different account',r=>replaceTransfer(r,2,otherAddress,beneficiary,950n)],
  ['direct mint is not the V2 route',r=>{replaceTransfer(r,1,zeroAddress,beneficiary,1000n);r.logs=r.logs.filter(l=>l.logIndex!==2);}],
  ['zero credit',r=>replaceTransfer(r,2,migrator,beneficiary,0n)],
  ['credit exceeds this mint',r=>replaceTransfer(r,2,migrator,beneficiary,1001n)],
  ['credit precedes mint',r=>{r.logs[1].logIndex=6;replaceTransfer(r,2,migrator,beneficiary,950n,{logIndex:0});}],
  ['credit follows Migrate',r=>replaceTransfer(r,2,migrator,beneficiary,950n,{logIndex:5})],
  ['credit cancelled after Migrate',r=>r.logs.push(transfer(beneficiary,otherAddress,950n,5))],
  ['foreign donation cannot mask cancellation',r=>r.logs.push(transfer(beneficiary,otherAddress,950n,5),transfer(otherAddress,beneficiary,1n,6))],
  ['ambiguous mints',r=>{r.logs[1].logIndex=6;r.logs.push(transfer(zeroAddress,migrator,2000n,0));}],
  ['duplicate log index',r=>{r.logs[1].logIndex=2;}],
  ['missing log index',r=>{delete r.logs[2].logIndex;}],
  ['other transaction credit',r=>replaceTransfer(r,2,migrator,beneficiary,950n,{transactionHash:other})],
  ['malformed debit cannot disappear',r=>r.logs.push(transfer(beneficiary,otherAddress,950n,5,{data:'0x01'}))],
  ['noncanonical address topic',r=>{r.logs[3].topics[1]='0x'+'01'.repeat(32);}],
  ['extra Transfer topic',r=>{r.logs[3].topics.push('0x'+'00'.repeat(32));}],
 ];
 for(const [name,mutate] of cases)await t.test(name,async()=>{const f=fixture({receipt:mutate});await assert.rejects(verifyMigration(f.rpc,params));});
 await t.test('unsupported migration module is not interpreted as V2',async()=>{const asset=[...proof.asset];asset[3]=otherAddress;const f=fixture({asset});await assert.rejects(verifyMigration(f.rpc,params),/Historical V2 migration module/);});
 await t.test('a migrator self-transfer is not a beneficiary credit',async()=>{const asset=[...proof.asset];asset[1]=migrator;const f=fixture({asset,receipt:r=>replaceTransfer(r,2,migrator,migrator,950n)});await assert.rejects(verifyMigration(f.rpc,{...params,governanceType:'launchpad'}),/transaction-bound V2 LP attribution/);});
});
