import {spawn,spawnSync,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {dirname,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {mkdir,writeFile,readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const root=resolve(dirname(fileURLToPath(import.meta.url)),'../../..');
const require=createRequire(resolve(root,'tests/hardhat/package.json'));
const {JsonRpcProvider,Wallet,getCreateAddress,keccak256,Transaction,decodeRlp,encodeRlp}=require('ethers');
const anvil=process.env.ANVIL_BINARY;if(!anvil)throw Error('ANVIL_BINARY required');
const runner=resolve(root,'build-release/nano-signed-execution-runner');
const output=process.env.NANO_DIAGNOSTIC_REPORT_DIR;if(!output)throw Error('NANO_DIAGNOSTIC_REPORT_DIR required');await mkdir(output,{recursive:true});
const referenceVersion=execFileSync(anvil,['--version'],{encoding:'utf8'}).trim();
const results=[];
function init(runtime){const size=(runtime.length/2).toString(16).padStart(2,'0');return '0x60'+size+'600c60003960'+size+'6000f3'+runtime;}
const availableScenarios=['touched-empty','fresh','funded','fresh-no-proof','unqueried'];
const scenarios=process.env.NANO_DIAGNOSTIC_SCENARIOS?.split(',')??availableScenarios;
assert(scenarios.length&&scenarios.every(s=>availableScenarios.includes(s)),'unknown scenario');
const forks=process.env.NANO_DIAGNOSTIC_FORKS?.split(',')??['Cancun','Prague','Osaka'];assert(forks.length&&forks.every(f=>['Cancun','Prague','Osaka'].includes(f)));
const blobPayload=process.env.NANO_BLOB_PAYLOAD?JSON.parse(await readFile(process.env.NANO_BLOB_PAYLOAD,'utf8')):undefined;
for(const scenario of scenarios)for(const fork of forks){
 const child=spawn(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--gas-limit','30000000'],{stdio:['ignore','pipe','pipe']});let rpc;
 try{
  const port=await new Promise((resolve,reject)=>{let text='',errors='';const timer=setTimeout(()=>reject(Error('Anvil startup timeout '+errors)),10000);child.stderr.on('data',d=>errors+=d);child.once('error',reject);child.once('exit',()=>{clearTimeout(timer);reject(Error('Anvil exited '+errors));});child.stdout.on('data',d=>{text+=d;const match=text.match(/Listening on 127\.0\.0\.1:(\d+)/);if(match){clearTimeout(timer);resolve(match[1]);}});});
  rpc=new JsonRpcProvider('http://127.0.0.1:'+port,1337,{cacheTimeout:-1});
  const wallet=new Wallet('0x'+'1'.padStart(64,'0')),authority=new Wallet('0x'+'2'.padStart(64,'0'));
  await rpc.send('anvil_setBalance',[wallet.address,'0xd3c21bcecceda1000000']);
  const tracked=new Set([wallet.address.toLowerCase(),authority.address.toLowerCase(),'0x0000000000000000000000000000000000000000']);
  let nonce=0;const cases=[];let transient,rollback;let authorityExecuted=false;
  for(const [name,code] of (blobPayload?[['transient-mcopy','5f496001554a60025560014960035500'],['rollback','602b6001555f5ffd']]:[['transient-mcopy','602a5f5d5f5c5f5260205f60205e60205160015500'],['blobbasefee','4a60025500'],['rollback','602b6001555f5ffd'],...(fork==='Osaka'?[['clz','60011e60015500']]:[])])){
   const target=getCreateAddress({from:wallet.address,nonce});tracked.add(target.toLowerCase());if(name==='transient-mcopy')transient=target;if(name==='rollback')rollback=target;
   cases.push({name:name+' deploy',nonce:nonce++,data:init(code)});cases.push({name:name+' call',nonce:nonce++,to:target,data:'0x'});
  }
  if(blobPayload){for(const [name,to,count] of [['blob hash and base fee',transient,1],['two blobs and index',transient,2],['blob fee charged after revert',rollback,1]])cases.push({name,nonce:nonce++,type:3,to,maxFeePerBlobGas:3,blobs:Array(count).fill(blobPayload)});}
  if(!blobPayload)cases.push({name:'calldata floor type 2',nonce:nonce++,type:2,to:['fresh','fresh-no-proof','unqueried'].includes(scenario)?wallet.address:authority.address,value:scenario==='funded'?1:0,data:'0x'+'ff'.repeat(32)});
  if(!blobPayload&&fork!=='Cancun'){
   const zero='0x'+'00'.repeat(20),delegated=a=>'0xef0100'+a.slice(2).toLowerCase();
   const auth=(target,n,chainId=1337)=>authority.authorize({address:target,chainId,nonce:n});
   const add=(name,fields,ownerNonce,code,status='0x1')=>cases.push({name,nonce:nonce++,type:4,to:authority.address,...fields,expected:{ownerNonce,code,status,slot1:42}});
   add('type 4 authorization and delegated execution',{authorizationList:[await auth(transient,0)]},1,delegated(transient));
   add('delegated call without authorization',{type:2},1,delegated(transient));
   add('revoke delegation retains storage',{authorizationList:[await auth(zero,1)]},2,'0x');
   add('wrong-chain authorization ignored',{authorizationList:[await auth(transient,2,1338)]},2,'0x');
   const duplicate=await auth(transient,2);
   add('duplicate authorization applied once',{authorizationList:[duplicate,duplicate]},3,delegated(transient));
   add('authorization persists after delegated revert',{authorizationList:[await auth(rollback,3)]},4,delegated(rollback),'0x0');
   add('reverted delegation persists into next call',{type:2},4,delegated(rollback),'0x0');
   add('clear reverted delegation',{authorizationList:[await auth(zero,4)]},5,'0x');
   add('stale-nonce authorization ignored',{authorizationList:[await auth(transient,0)]},5,'0x');
   add('chain-zero authorization accepted',{authorizationList:[await auth(transient,5,0)]},6,delegated(transient));
  }
  async function accounts(){const state={};for(const a of tracked){if(scenario==='unqueried'&&!authorityExecuted&&a===authority.address.toLowerCase())continue;const [balance,nonce,code]=await Promise.all(['eth_getBalance','eth_getTransactionCount','eth_getCode'].map(method=>rpc.send(method,[a,'latest'])));const storage={};for(const slot of [0,1,2,3]){const value=await rpc.send('eth_getStorageAt',[a,'0x'+slot.toString(16),'latest']);if(BigInt(value)!==0n)storage['0x'+slot.toString(16)]=value;}state[a]={balance,nonce,code,storage};}return state;}
  for(const test of cases){
   const pre=await accounts();if(test.type===4&&!['fresh-no-proof','unqueried'].includes(scenario)){const proof=await rpc.send('eth_getProof',[authority.address,[],'latest']);await writeFile(resolve(output,scenario+'-'+fork+'-'+test.nonce+'-authority-proof.json'),JSON.stringify(proof,null,2)+'\n');}const type=test.type??0;const {name,expected,...fields}=test;
   let wire=await wallet.signTransaction({chainId:1337,gasLimit:1000000,...(type===0?{gasPrice:3000000000}:{maxFeePerGas:3000000000,maxPriorityFeePerGas:1000000000}),...fields,type});
   const envelope=Transaction.from(wire);envelope.blobs=null;const raw=envelope.serialized;
   if(type===3&&fork==='Osaka'){
    assert.equal(blobPayload.cellProofs?.length,128,'Osaka requires cell proofs');
    const body=decodeRlp(raw.slice(0,2)+raw.slice(4));
    wire='0x03'+encodeRlp([body,'0x01',test.blobs.map(b=>b.data),test.blobs.map(b=>b.commitment),test.blobs.flatMap(()=>blobPayload.cellProofs)]).slice(2);
   }
   if(type===3)await writeFile(resolve(output,scenario+'-'+fork+'-'+test.nonce+'-network-attempt.json'),JSON.stringify({wire,raw,expectedHash:keccak256(raw)})+'\n');
   let hash;try{hash=await rpc.send('eth_sendRawTransaction',[wire]);}catch(error){await writeFile(resolve(output,scenario+'-'+fork+'-'+test.nonce+'-admission-error.json'),JSON.stringify({code:error.code,message:error.shortMessage,error:error.info?.error??error.error,detail:String(error.message).slice(0,2000)},null,2)+'\n');throw Error('Anvil admission failed: '+(error.shortMessage??String(error)).slice(0,500));}assert.equal(hash,keccak256(raw),'Anvil returned wrong signed transaction hash');let receipt;
   for(let i=0;i<100;++i){receipt=await rpc.send('eth_getTransactionReceipt',[hash]);if(receipt)break;await new Promise(r=>setTimeout(r,20));}assert(receipt,'missing Anvil receipt');
   const block=await rpc.send('eth_getBlockByHash',[receipt.blockHash,false]);assert(tracked.has(block.miner.toLowerCase()),'untracked fee recipient');
   const input={fork,chainId:'0x539',pre,block,rawTransaction:raw};const stem=scenario+'-'+fork+'-'+test.nonce;
   if(type===3)await writeFile(resolve(output,stem+'-network-transaction.json'),JSON.stringify({wire,raw,hash})+'\n');
   await writeFile(resolve(output,stem+'-input.json'),JSON.stringify(input,null,2)+'\n');
   const replay=spawnSync(runner,[],{input:JSON.stringify(input),encoding:'utf8',timeout:10000,maxBuffer:8*1024*1024});assert.ifError(replay.error);assert.equal(replay.status,0,replay.stderr);
   const actual=JSON.parse(replay.stdout);await writeFile(resolve(output,stem+'-nano.json'),JSON.stringify(actual,null,2)+'\n');
   if(test.type===4)authorityExecuted=true;
   const post=await accounts();await writeFile(resolve(output,stem+'-anvil.json'),JSON.stringify({receipt,post},null,2)+'\n');
   let mismatch;try{
   if(type===3){
    assert.equal(BigInt(block.excessBlobGas),0n,'blob probe assumes minimum blob fee');
    const reverted=test.to===rollback;
    for(const [label,state,r] of [['Nano',actual.post,actual.receipt],['Anvil',post,receipt]]){
     assert.equal(r.status,reverted?'0x0':'0x1',label+' blob status');if(reverted){assert(BigInt(r.gasUsed)<1000000n,label+' expected REVERT, not all-gas exceptional halt');assert.equal(BigInt(state[test.to.toLowerCase()].storage?.['0x1']??0),0n,label+' reverted write persisted');}
     if(!reverted){const storage=state[test.to.toLowerCase()].storage;
      assert.equal(BigInt(storage['0x1']),BigInt(blobPayload.versionedHash),label+' first blob hash');assert.equal(BigInt(storage['0x2']),1n,label+' blob base fee');
      assert.equal(BigInt(storage['0x3']??0),test.blobs.length===2?BigInt(blobPayload.versionedHash):0n,label+' second blob hash');
     }
     const paid=BigInt(pre[wallet.address.toLowerCase()].balance)-BigInt(state[wallet.address.toLowerCase()].balance);
     assert.equal(paid,BigInt(r.gasUsed)*BigInt(receipt.effectiveGasPrice)+131072n*BigInt(test.blobs.length),label+' blob fee burn including revert');
    }
   }
   if(expected){for(const [label,state,r] of [['Nano',actual.post,actual.receipt],['Anvil',post,receipt]]){
    const a=state[authority.address.toLowerCase()];assert(a,label+' missing authority');
    assert.equal(BigInt(a.nonce),BigInt(expected.ownerNonce),label+' authority nonce');assert.equal(a.code,expected.code,label+' authority code');
    assert.equal(BigInt(a.storage?.['0x1']??0),BigInt(expected.slot1),label+' storage rollback/persistence');assert.equal(r.status,expected.status,label+' receipt status');if(expected.status==='0x0')assert(BigInt(r.gasUsed)<900000n,label+' expected bounded REVERT');
   }}
   for(const field of ['transactionHash','status','type','gasUsed','cumulativeGasUsed','logsBloom'])assert.equal(actual.receipt[field],receipt[field],fork+' '+name+' '+field);
   const logs=r=>r.logs.map(l=>({address:l.address.toLowerCase(),topics:l.topics,data:l.data}));assert.deepEqual(logs(actual.receipt),logs(receipt));
   for(const [a,expected] of Object.entries(post)){const got=actual.post[a]??{balance:'0x0',nonce:'0x0',code:'0x',storage:{}};assert.equal(BigInt(got.balance),BigInt(expected.balance),a+' balance');assert.equal(BigInt(got.nonce),BigInt(expected.nonce),a+' nonce');assert.equal(got.code,expected.code,a+' code');for(const slot of [0,1,2,3]){const key='0x'+slot.toString(16);assert.equal(BigInt(got.storage?.[key]??0),BigInt(expected.storage[key]??0),a+' storage '+key);}}
   }catch(error){mismatch=error.message;}
   const row={scenario,fork,case:name,transactionHash:hash,status:mismatch?'mismatch':'matched',gasUsed:actual.receipt.gasUsed,referenceGasUsed:receipt.gasUsed,...(mismatch?{error:mismatch}:{})};results.push(row);console.log(JSON.stringify(row));
  }
 }finally{rpc?.destroy();if(child.exitCode===null){const stopped=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await stopped;clearTimeout(timer);}}
}
const mismatched=results.filter(r=>r.status==='mismatch').length;
await writeFile(resolve(output,'results.json'),JSON.stringify({results,referenceVersion},null,2)+'\n');
process.exitCode=mismatched?1:0;
console.log(JSON.stringify({summary:{matched:results.length-mismatched,mismatched,referenceVersion,scope:'Identical signed transactions, receipts and explicitly tracked accounts/slots; no global state-root or consensus comparison'}}));
