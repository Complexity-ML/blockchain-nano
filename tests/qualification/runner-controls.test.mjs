import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,readFile,writeFile,rm} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import {tmpdir} from 'node:os';
import {resolve,join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
const root=resolve(dirname(fileURLToPath(import.meta.url)),'../..');
const build=resolve(root,process.env.NANO_QUALIFICATION_BUILD??'build-release');
async function run(fixture,kind){
 const dir=await mkdtemp(join(tmpdir(),'nano-runner-control-'));
 try{
  const path=join(dir,'fixture.json');await writeFile(path,JSON.stringify(fixture));
  const result=spawnSync(join(build,`nano-ethereum-${kind}-runner`),[path],{encoding:'utf8',timeout:10000,maxBuffer:2**20});
  assert.ifError(result.error);assert.equal(result.signal,null);
  return {exit:result.status,rows:result.stdout.trim().split('\n').map(s=>JSON.parse(s))};
 }finally{await rm(dir,{recursive:true,force:true});}
}
const original=JSON.parse(await readFile(join(root,'tests/fixtures/ethereum-official/call-insufficient-balance.json'),'utf8'));
const [name,value]=Object.entries(original)[0];
const stateFixture=()=>({[name]:structuredClone(value)});
test('state runner accepts the unmodified control',async()=>{
 const r=await run(stateFixture(),'state');assert.equal(r.exit,0);assert.equal(r.rows.at(-1).summary.matched,1);
});
for(const field of ['hash','logs','receiptRlp','receiptGas','receiptGasWithoutRlp','transactionHash'])test(`state runner rejects corrupted expected ${field}`,async()=>{
 const fixture=stateFixture(),expected=Object.values(fixture[name].post)[0][0];
 if(field==='transactionHash')expected.receipt.transactionHash='0x'+'00'.repeat(32);
 else if(field==='receiptRlp')expected.receipt.rlp='0xc0';
 else if(field.startsWith('receiptGas')){expected.receipt.cumulativeGasUsed='0x1';if(field==='receiptGasWithoutRlp')delete expected.receipt.rlp;}
 else expected[field]='0x'+'00'.repeat(32);
 const r=await run(fixture,'state');assert.equal(r.exit,1);assert.equal(r.rows[0].status,'mismatch');
});
test('transaction runner cannot accept a different expected rejection',async()=>{
 const path='tests/fixtures/ethereum-transactions/for_osaka--prague--eip7702_set_code_tx--invalid_tx--empty_authorization_list.json';
 const all=JSON.parse(await readFile(join(root,path),'utf8')),[name,fixture]=Object.entries(all)[0];
 const good=await run({[name]:fixture},'transaction');assert.equal(good.exit,0);
 Object.values(fixture.result)[0].exception='TransactionException.TYPE_4_INVALID_AUTHORITY_SIGNATURE';
 const bad=await run({[name]:fixture},'transaction');assert.equal(bad.exit,1);assert.equal(bad.rows[0].status,'mismatch');
});

const blockPath='tests/fixtures/ethereum-blocks/for_berlin--berlin--eip2929_gas_cost_increases--call--call_insufficient_balance.json';
const blockOriginal=JSON.parse(await readFile(join(root,blockPath),'utf8'));
const [blockName,blockValue]=Object.entries(blockOriginal)[0];
const blockFixture=()=>({[blockName]:structuredClone(blockValue)});
test('block runner accepts the unmodified control',async()=>{
 const r=await run(blockFixture(),'block');assert.equal(r.exit,0);assert.equal(r.rows[0].status,'matched');
});
for(const field of ['postState','lastblockhash'])test(`block runner rejects corrupted expected ${field}`,async()=>{
 const fixture=blockFixture(),value=fixture[blockName];
 if(field==='lastblockhash')value.lastblockhash='0x'+'00'.repeat(32);
 else {const account=Object.values(value.postState)[0];account.balance='0x'+(BigInt(account.balance)+1n).toString(16);}
 const r=await run(fixture,'block');assert.equal(r.exit,1);assert.equal(r.rows[0].status,'mismatch');
 assert.equal(r.rows[0][field==='postState'?'postStateMatches':'lastBlockHashMatches'],false);
});
test('block runner cannot accept a different expected rejection',async()=>{
 const all=JSON.parse(await readFile(join(root,'tests/fixtures/ethereum-blocks/for_paris--london--validation--header--invalid_header.json'),'utf8'));
 const [name,fixture]=Object.entries(all)[0];
 const good=await run({[name]:fixture},'block');assert.equal(good.exit,0);
 fixture.blocks[0].expectException='BlockException.INCORRECT_BLOCK_FORMAT';
 const bad=await run({[name]:fixture},'block');assert.equal(bad.exit,1);assert.equal(bad.rows[0].status,'mismatch');assert.equal(bad.rows[0].rejections[0].matched,false);
});
test('unsupported block fork stays an explicit failure',async()=>{
 const fixture=blockFixture();fixture[blockName].network='UnknownFork';
 const r=await run(fixture,'block');assert.equal(r.exit,1);assert.equal(r.rows[0].status,'unsupported');assert.equal(r.rows.at(-1).summary.unsupported,1);
});
