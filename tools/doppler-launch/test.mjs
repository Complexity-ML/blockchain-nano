import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createServer} from 'node:http';
import {decodeFunctionData, encodeAbiParameters} from 'viem';
import {airlockAbi} from '@whetstone-research/doppler-sdk/evm';
import {parameters,preview,USDC} from './preview.mjs';
const fixture=JSON.parse(await readFile(new URL('./example.synthetic.json',import.meta.url)));
const copy=()=>structuredClone(fixture);
test('official SDK calldata decodes to explicit supply, USDC and migration',async()=>{
 const a=await preview(copy()),b=await preview(copy());
 assert.equal(a.transaction.data,b.transaction.data);
 const decoded=decodeFunctionData({abi:airlockAbi,data:a.transaction.data});
 assert.equal(decoded.functionName,'create');
 assert.equal(decoded.args[0].initialSupply,BigInt(fixture.initialSupply));
 assert.equal(decoded.args[0].numTokensToSell,BigInt(fixture.numTokensToSell));
 assert.equal(decoded.args[0].numeraire.toLowerCase(),USDC.toLowerCase());
 assert.equal(a.simulation.performed,false);
 assert.equal(a.productionReady,false);
 assert.equal(a.transaction.value,'0');
});
test('reject ambiguous monetary units, absent inflation, wrong network and reserve exhaustion',()=>{
 for (const mutate of [c=>c.initialSupply=1e25,c=>delete c.yearlyMintRate,c=>c.modules={airlock:fixture.creator},c=>c.chainId=1,c=>c.numTokensToSell=c.initialSupply,c=>c.numeraire='0x2222222222222222222222222222222222222222',c=>c.nativeAssetRelationship='',c=>c.governance={type:'default'},c=>c.auction.maxProceeds='1',c=>delete c.auction.gamma,c=>c.auction.gamma=120,c=>c.auction.gamma=20041]) {
  const c=copy();mutate(c);assert.throws(()=>parameters(c));
 }
});
test('wrapper and SDK reject incompatible Dynamic tick direction and spacing',async()=>{
 let c=copy();c.pool.tickSpacing=60;await assert.rejects(preview(c),/tickSpacing/);
 c=copy();[c.auction.startTick,c.auction.endTick]=[c.auction.endTick,c.auction.startTick];await assert.rejects(preview(c),/Start tick/);
});
test('simulation uses creator, a single pinned block and read-only RPC',async()=>{
 const predicted=await preview(copy());
 const calls=[];const block='0x1234', hash='0x'+'ab'.repeat(32);
 const server=createServer(async(req,res)=>{
  let data='';for await(const chunk of req)data+=chunk;
  const q=JSON.parse(data);calls.push(q);
  try {
  let result;
  if(q.method==='eth_chainId')result='0x2105';
  else if(q.method==='eth_getBlockByNumber')result={number:block,hash,timestamp:'0x1',transactions:[],gasLimit:'0x1c9c380',gasUsed:'0x0',baseFeePerGas:'0x1'};
  else if(q.method==='eth_getCode')result='0x6000';
  else if(q.method==='eth_estimateGas')result='0x100000';
  else if(q.method==='eth_call')result=q.params[0].data==='0x313ce567'?'0x'+('6'.padStart(64,'0')):encodeAbiParameters(airlockAbi.find(x=>x.name==='create').outputs,[predicted.predicted.tokenAddress,predicted.predicted.hookAddress,...Array(3).fill('0x1111111111111111111111111111111111111111')]);
  else throw new Error(`Unexpected RPC ${q.method}`);
  res.setHeader('content-type','application/json');res.end(JSON.stringify({jsonrpc:'2.0',id:q.id,result}));
  } catch(e) {res.end(JSON.stringify({jsonrpc:'2.0',id:q.id,error:{code:-32000,message:e.message}}));}
 });
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 try {
  const result=await preview(copy(),`http://127.0.0.1:${server.address().port}`);
  assert.equal(result.simulation.performed,true);
  assert.equal(result.simulation.blockHash,hash);
  for(const q of calls.filter(q=>['eth_getCode','eth_call','eth_estimateGas'].includes(q.method))) assert.equal(q.params[1],block);
  for(const q of calls.filter(q=>['eth_call','eth_estimateGas'].includes(q.method)&&q.params[0].data!=='0x313ce567')) assert.equal(q.params[0].from.toLowerCase(),fixture.creator);
  assert.ok(calls.every(q=>!q.method.startsWith('eth_send')));
 } finally {await new Promise(resolve=>server.close(resolve));}
});
test('failed chain preflight is retained as rejection, never a successful simulation',async()=>{
 const server=createServer(async(req,res)=>{let data='';for await(const chunk of req)data+=chunk;const q=JSON.parse(data);res.end(JSON.stringify({jsonrpc:'2.0',id:q.id,result:'0x1'}));});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 try {const r=await preview(copy(),`http://127.0.0.1:${server.address().port}`);assert.equal(r.status,'simulation-rejected');assert.equal(r.simulation.success,false);assert.match(r.simulation.error,/chain mismatch/);assert.ok(r.transaction.data);}finally{await new Promise(resolve=>server.close(resolve));}
});
