import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {decodeFunctionData,encodeEventTopics,encodeFunctionResult,encodeAbiParameters} from 'viem';
import {airlockAbi} from '@whetstone-research/doppler-sdk/evm';
import {preview} from './preview.mjs';
import {verifyMigration} from './verify-migration.mjs';
import {createSourceRpc,createLaunchRpc} from '../bridge-source/rpc.mjs';
const configuration=JSON.parse(await readFile(new URL('./example.synthetic.json',import.meta.url)));
const historical=JSON.parse(await readFile(new URL('../../docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json',import.meta.url)));
const proof=historical.steps.find(x=>x.name==='migration-state');

test('offline preview snapshots reviewed terms before SDK awaits without freezing caller data',async()=>{
 const c=structuredClone(configuration),original=structuredClone(c);
 const pending=preview(c);
 c.initialSupply='123';c.auction.gamma=120;c.nativeAssetRelationship='caller changed terms after review began';
 const result=await pending;
 const decoded=decodeFunctionData({abi:airlockAbi,data:result.transaction.data});
 assert.equal(decoded.args[0].initialSupply,BigInt(original.initialSupply));
 assert.deepEqual(result.config,original);
 assert.equal(result.nativeAssetRelationship,original.nativeAssetRelationship);
 assert.notEqual(result.config,c);assert.notEqual(result.config.auction,c.auction);
 assert.equal(c.initialSupply,'123');assert.equal(c.auction.gamma,120);
 assert.equal(result.simulation.performed,false);assert.equal(result.productionReady,false);
});

test('mutable provider receipt cannot hide a canonical A to B reorg during migration reads',async()=>{
 const params={tokenAddress:historical.predicted.tokenAddress,numeraire:historical.configuration.numeraire,migrationHash:proof.migrationHash,governanceType:'noOp'};
 const hashA=proof.blockHash,hashB='0x'+'99'.repeat(32),number=BigInt(proof.blockNumber);assert.notEqual(hashA,hashB);
 const shared={status:'success',transactionHash:params.migrationHash,blockNumber:number,blockHash:hashA,logs:[{address:proof.airlock,blockNumber:number,blockHash:hashA,transactionHash:params.migrationHash,removed:false,data:'0x',topics:encodeEventTopics({abi:airlockAbi,eventName:'Migrate',args:{asset:params.tokenAddress,pool:proof.pair}})}]};
 const pairAbi=[{type:'function',name:'token0',inputs:[],outputs:[{type:'address'}]},{type:'function',name:'token1',inputs:[],outputs:[{type:'address'}]},{type:'function',name:'getReserves',inputs:[],outputs:[{type:'uint112'},{type:'uint112'},{type:'uint32'}]},{type:'function',name:'totalSupply',inputs:[],outputs:[{type:'uint256'}]},{type:'function',name:'balanceOf',inputs:[{type:'address'}],outputs:[{type:'uint256'}]}];
 let reorg=false;const selectors=[],sharedHeader={number,hash:hashA};
 const rpc={
  getChainId:async()=>31337,
  getTransactionReceipt:async()=>shared,
  getBlock:async({blockNumber})=>{assert.equal(blockNumber,number);return sharedHeader;},
  request:async(q)=>{
   if(q.method==='web3_clientVersion')return 'anvil/snapshot-test';
   assert.equal(q.method,'eth_call');assert.equal(q.params[1].requireCanonical,true);selectors.push(q.params[1].blockHash);
   // A cache refresh rewrites the SAME object held by the old verifier.
   if(!reorg){reorg=true;shared.blockHash=hashB;shared.logs[0].blockHash=hashB;sharedHeader.hash=hashB;}
   const airlock=q.params[0].to.toLowerCase()===proof.airlock.toLowerCase(),abi=airlock?airlockAbi:pairAbi;
   const {functionName}=decodeFunctionData({abi,data:q.params[0].data});
   const value=airlock?proof.asset:{token0:proof.token0,token1:proof.token1,getReserves:proof.reserves,totalSupply:proof.totalSupply,balanceOf:proof.beneficiaryLP}[functionName];
   return encodeFunctionResult({abi,functionName,result:value});
  }
 };
 await assert.rejects(verifyMigration(rpc,params),/Canonical migration block changed during verification/);
 assert.equal(selectors.length,6);assert.deepEqual(selectors,Array(6).fill(hashA));
 assert.equal(shared.blockHash,hashB);
});


async function withFetch(handler,callback){
 const original=globalThis.fetch;
 globalThis.fetch=async(_endpoint,options)=>{
  assert.equal(options.redirect,'error');
  const q=JSON.parse(options.body),body=await handler(q);
  return new Response(typeof body==='string'?body:JSON.stringify(body),{status:200,headers:{'content-type':'application/json'}});
 };
 try{return await callback();}finally{globalThis.fetch=original;}
}
let predictedFixture;
async function launchFixture({aba=false,refuseMethod=null,changedFinalHeader=false}={}){
 const predicted=await (predictedFixture??=preview(structuredClone(configuration))),calls=[];
 const hashA='0x'+'ab'.repeat(32),hashB='0x'+'cd'.repeat(32),number='0x1234';let headers=0,servedHeightB=0;
 const handler=q=>{
  calls.push(q);let result;
  if(q.method==='eth_chainId')result='0x2105';
  else if(q.method==='eth_getBlockByNumber'){
   ++headers;result={number,hash:changedFinalHeader&&headers>1?hashB:hashA,timestamp:'0x1',transactions:[],gasLimit:'0x1c9c380',gasUsed:'0x0',baseFeePerGas:'0x1'};
  }else{
   assert.ok(['eth_getCode','eth_call','eth_estimateGas'].includes(q.method));
   const selector=q.params[1],byHash=selector&&typeof selector==='object'&&selector.blockHash===hashA&&selector.requireCanonical===true;
   if(q.method===refuseMethod&&byHash)return {jsonrpc:'2.0',id:q.id,error:{code:-32602,message:'sensitive provider message https://secret.invalid/token',data:'0x248d3d6a'}};
   // A -> B for this state read -> A before the final number/header lookup.
   if(aba){if(byHash)return {jsonrpc:'2.0',id:q.id,error:{code:-32000,message:'not canonical'}};assert.equal(selector,number);++servedHeightB;}
   if(q.method==='eth_getCode')result='0x6000';
   else if(q.method==='eth_estimateGas')result='0x100000';
   else result=q.params[0].data==='0x313ce567'?'0x'+'6'.padStart(64,'0'):encodeAbiParameters(airlockAbi.find(x=>x.name==='create').outputs,[predicted.predicted.tokenAddress,predicted.predicted.hookAddress,...Array(3).fill('0x1111111111111111111111111111111111111111')]);
  }
  return {jsonrpc:'2.0',id:q.id,result};
 };
 const result=await withFetch(handler,()=>preview(structuredClone(configuration),'https://snapshot.invalid'));
 return {result,calls,hashA,headers,servedHeightB};
}

test('successful preview pins all code, call and gas-extension reads to one canonical hash',async()=>{
 const f=await launchFixture();assert.equal(f.result.simulation.success,true);assert.equal(f.result.simulation.referenceHashPinned,true);assert.equal(f.result.simulation.gasEstimateHashExtensionAccepted,true);
 const states=f.calls.filter(q=>['eth_getCode','eth_call','eth_estimateGas'].includes(q.method));assert.equal(states.length,9);
 for(const q of states)assert.deepEqual(q.params[1],{blockHash:f.hashA,requireCanonical:true});
 assert.equal(f.headers,2);assert.equal(f.result.productionReady,false);assert.ok(f.calls.every(q=>!/^eth_(send|sign)/.test(q.method)));
});

test('preview refuses an ABA canonical oscillation instead of accepting height-only state B',async()=>{
 const f=await launchFixture({aba:true});assert.equal(f.result.status,'simulation-rejected');assert.equal(f.result.simulation.success,false);assert.equal(f.result.simulation.referenceHashPinned,false);assert.equal(f.servedHeightB,0);
 const states=f.calls.filter(q=>['eth_getCode','eth_call','eth_estimateGas'].includes(q.method));assert.equal(states.length,1);assert.deepEqual(states[0].params[1],{blockHash:f.hashA,requireCanonical:true});
});

test('unsupported hash selectors reject without height/latest fallback or diagnostic secrets',async()=>{
 for(const method of ['eth_getCode','eth_call','eth_estimateGas']){
  const f=await launchFixture({refuseMethod:method});assert.equal(f.result.status,'simulation-rejected');assert.equal(f.result.simulation.referenceHashPinned,false);
  const states=f.calls.filter(q=>['eth_getCode','eth_call','eth_estimateGas'].includes(q.method));for(const q of states)assert.deepEqual(q.params[1],{blockHash:f.hashA,requireCanonical:true});
  assert.equal(f.calls.filter(q=>q.method===method).length,1);
  assert.deepEqual(f.result.simulation.rpcErrors,[{method,code:-32602,data:'0x248d3d6a'}]);assert.ok(!JSON.stringify(f.result,(_,v)=>typeof v==='bigint'?v.toString():v).includes('secret.invalid'));
 }
});

test('final header change rejects even after all hash-bound requests succeed',async()=>{
 const f=await launchFixture({changedFinalHeader:true});assert.equal(f.result.status,'simulation-rejected');assert.equal(f.result.simulation.referenceHashPinned,false);assert.match(f.result.simulation.error,/Reference block changed/);
});

test('launch transport refuses mismatched IDs, ambiguous envelopes, oversized and truncated bodies',async()=>{
 const cases=[q=>({jsonrpc:'2.0',id:q.id+1,result:'0x2105'}),q=>({jsonrpc:'2.0',id:q.id,result:'0x2105',error:{code:-32000}}),q=>' '.repeat(1048577),q=>'{' ];
 for(const response of cases)await withFetch(response,async()=>{await assert.rejects(createLaunchRpc('https://snapshot.invalid').request({method:'eth_chainId',params:[]}));});
});

test('both factories keep fixed read-only method sets and bounded revert diagnostics',async()=>{
 let calls=0;
 await withFetch(q=>{++calls;return {jsonrpc:'2.0',id:q.id,error:{code:-32000,message:'private provider URL',data:'0x'+'aa'.repeat(4097)}};},async()=>{
  const diagnostics=[],source=createSourceRpc('https://snapshot.invalid'),launch=createLaunchRpc('https://snapshot.invalid',{onRpcError:d=>diagnostics.push(d)});
  await assert.rejects(source.request({method:'eth_estimateGas',params:[]}));
  await assert.rejects(launch.request({method:'eth_getTransactionReceipt',params:[]}));
  for(const rpc of [source,launch])for(const method of ['eth_sendTransaction','eth_sendRawTransaction','eth_sign'])await assert.rejects(rpc.request({method,params:[]}));
  assert.equal(calls,0);await assert.rejects(launch.request({method:'eth_estimateGas',params:[]}));assert.equal(calls,1);assert.deepEqual(diagnostics,[{method:'eth_estimateGas',code:-32000}]);
 });
});
