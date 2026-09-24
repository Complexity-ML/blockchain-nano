import { network, artifacts } from 'hardhat';
import { BrowserProvider, JsonRpcProvider, Wallet, Interface, getCreateAddress, keccak256 } from 'ethers';
import { spawn, execFileSync } from 'node:child_process';
import assert from 'node:assert/strict';
const binary=process.env.ANVIL_BINARY;
if(!binary)throw Error('ANVIL_BINARY is required');
const referenceVersion=execFileSync(binary,['--version'],{encoding:'utf8'}).trim();
const fork=process.env.NANO_TEST_EVM_FORK??'Paris';
if(!['Paris','Shanghai'].includes(fork))throw Error('Comparison supports Paris or Shanghai only');
const child=spawn(binary,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--gas-limit','30000000','--base-fee','1000000000'],{stdio:['ignore','pipe','pipe']});
let text='';let reference:JsonRpcProvider|undefined;let connection;
try {
 const port=await new Promise<string>((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Anvil startup timeout')),10000);child.once('error',reject);child.once('exit',()=>{clearTimeout(timer);reject(Error('Anvil exited'));});child.stdout.on('data',chunk=>{text+=chunk;const match=text.match(/Listening on 127\.0\.0\.1:(\d+)/);if(match){clearTimeout(timer);resolve(match[1]);}});});
 reference=new JsonRpcProvider(`http://127.0.0.1:${port}`,1337,{cacheTimeout:-1});
 connection=await network.create('nano');const nano=new BrowserProvider(connection.provider);nano.pollingInterval=100;
 const keys=[1,2].map(n=>new Wallet('0x'+n.toString(16).padStart(64,'0')));
 for(const key of keys)await reference.send('anvil_setBalance',[key.address,'0xd3c21bcecceda1000000']);
 const artifact=await artifacts.readArtifact('DeveloperToken');const abi=new Interface(artifact.abi);
 const token=getCreateAddress({from:keys[0].address,nonce:0});
 const cases:Array<{name:string,signer:number,nonce:number,data:string,to:string|undefined,type?:number}>=[
  {name:'deploy',signer:0,nonce:0,data:artifact.bytecode,to:undefined},
  {name:'transfer',signer:0,nonce:1,data:abi.encodeFunctionData('transfer',[keys[1].address,125]),to:token},
  {name:'unauthorized transferFrom',signer:1,nonce:0,data:abi.encodeFunctionData('transferFrom',[keys[0].address,keys[1].address,1]),to:token},
 ];
 const storageArtifact=await artifacts.readArtifact('StorageMeter');const storageAbi=new Interface(storageArtifact.abi);
 const storage=getCreateAddress({from:keys[0].address,nonce:2});
 cases.push({name:'storage deploy',signer:0,nonce:2,data:storageArtifact.bytecode,to:undefined});
 let nonce=3;
 for(const values of [[0,1],[0,2],[0],[1,0],[1],[0,1,0],[2,3,2],[1]]){
  cases.push({name:`storage ${values.join('->')}`,signer:0,nonce:nonce++,data:storageAbi.encodeFunctionData('write',[values,false]),to:storage});
 }
 for(const rollback of [false,true])cases.push({name:`nested storage rollback=${rollback}`,signer:0,nonce:nonce++,data:storageAbi.encodeFunctionData('nested',[rollback]),to:storage});
 if(fork==='Shanghai'){
  // Explicit Shanghai bytecode, independent of the Solidity compiler target.
  const warmCoinbase=getCreateAddress({from:keys[0].address,nonce});
  cases.push({name:'Shanghai warm coinbase deploy',signer:0,nonce:nonce++,data:'0x6004600c60003960046000f341315000',to:undefined});
  cases.push({name:'Shanghai COINBASE BALANCE warm access',signer:0,nonce:nonce++,data:'0x',to:warmCoinbase});
  const push0=getCreateAddress({from:keys[0].address,nonce});
  cases.push({name:'Shanghai PUSH0 runtime deploy',signer:0,nonce:nonce++,data:'0x6007600c60003960076000f35f5f5260205ff3',to:undefined});
  cases.push({name:'Shanghai PUSH0 runtime call',signer:0,nonce:nonce++,data:'0x',to:push0});
  for(const length of [31,32,33])cases.push({name:`Shanghai initcode ${length} bytes`,signer:0,nonce:nonce++,data:'0x5f5ff3'+'00'.repeat(length-3),to:undefined});
  for(const type of [0,1,2])for(const length of [49151,49152])cases.push({name:`Shanghai type ${type} initcode ${length} bytes`,signer:0,nonce:nonce++,type,data:'0x5f5ff3'+'00'.repeat(length-3),to:undefined});
 }
 const report=[];
 for(const test of cases){
  const type=test.type??0;
  const fees=type===2?{maxFeePerGas:4000000000,maxPriorityFeePerGas:1000000000}:{gasPrice:2000000000};
  const raw=await keys[test.signer].signTransaction({chainId:1337,type,nonce:test.nonce,gasLimit:3000000,...fees,to:test.to,data:test.data});
  const hashes=await Promise.all([nano.send('eth_sendRawTransaction',[raw]),reference.send('eth_sendRawTransaction',[raw])]);assert.equal(hashes[0],hashes[1]);
  async function receipt(provider:any){for(let n=0;n<300;++n){const r=await provider.send('eth_getTransactionReceipt',[hashes[0]]);if(r)return r;await new Promise(resolve=>setTimeout(resolve,100));}throw Error('receipt timeout');}
  const [actual,expected]=await Promise.all([receipt(nano),receipt(reference)]);
  for(const receipt of [actual,expected]){assert.equal(receipt.transactionHash,hashes[0]);assert.equal(BigInt(receipt.type),BigInt(type),`${test.name}: receipt type differs`);}
  if(test.name.startsWith('Shanghai type '))assert.equal(actual.status,'0x1',`${test.name}: valid boundary creation failed`);
  const logs=(r:any)=>r.logs.map((l:any)=>({address:l.address.toLowerCase(),topics:l.topics,data:l.data}));
  assert.equal(actual.status,expected.status,`${test.name}: status differs`);assert.deepEqual(logs(actual),logs(expected),`${test.name}: logs differ`);
  if(!test.to){assert.equal(actual.contractAddress?.toLowerCase(),expected.contractAddress?.toLowerCase(),`${test.name}: deployed address differs`);if(actual.contractAddress){assert.equal(await nano.send('eth_getCode',[actual.contractAddress,'latest']),await reference.send('eth_getCode',[expected.contractAddress,'latest']),`${test.name}: deployed code differs`);}}
  if(test.name==='Shanghai PUSH0 runtime call'){const call={to:test.to,data:'0x'};const result=await nano.send('eth_call',[call,'latest']);assert.equal(result,'0x'+'00'.repeat(32));assert.equal(result,await reference.send('eth_call',[call,'latest']));}
  if(test.to===storage){const call={to:storage,data:storageAbi.encodeFunctionData('value',[])};assert.equal(await nano.send('eth_call',[call,'latest']),await reference.send('eth_call',[call,'latest']),`${test.name}: storage differs`);}
  report.push({case:test.name,nanoGas:BigInt(actual.gasUsed).toString(),anvilGas:BigInt(expected.gasUsed).toString(),gasMatches:BigInt(actual.gasUsed)===BigInt(expected.gasUsed)});
 }
 for(const method of [['totalSupply',[]],['balanceOf',[keys[0].address]],['balanceOf',[keys[1].address]]] as const){const call={to:token,data:abi.encodeFunctionData(method[0],method[1])};assert.equal(await nano.send('eth_call',[call,'latest']),await reference.send('eth_call',[call,'latest']));}
 const admissionReport=[];
 if(fork==='Shanghai')for(const type of [0,1,2]){
  const fees=type===2?{maxFeePerGas:4000000000,maxPriorityFeePerGas:1000000000}:{gasPrice:4000000000};
  const raw=await keys[0].signTransaction({chainId:1337,type,nonce,gasLimit:3000000,...fees,data:'0x5f5ff3'+'00'.repeat(49153-3)});
  const account=async(provider:any)=>({nonce:await provider.send('eth_getTransactionCount',[keys[0].address,'latest']),balance:await provider.send('eth_getBalance',[keys[0].address,'latest'])});
  const nanoBefore=await account(nano);
  const pendingBefore=await nano.send('eth_getTransactionCount',[keys[0].address,'pending']);
  await assert.rejects(nano.send('eth_sendRawTransaction',[raw]),/initcode|init code|EIP.?3860/i,'Nano: accepted oversized initcode');
  assert.equal(await nano.send('eth_getTransactionCount',[keys[0].address,'pending']),pendingBefore,'Oversized initcode changed pending nonce');
  assert.deepEqual(await account(nano),nanoBefore,'Rejected initcode changed Nano canonical state');
  const referenceBefore=await account(reference);
  let admittedHash:string|undefined;
  try{admittedHash=await reference.send('eth_sendRawTransaction',[raw]);}
  catch(error){assert.match(String(error),/initcode|init code|EIP.?3860/i);}
  const expectedHash=keccak256(raw);
  if(admittedHash){
   assert.equal(admittedHash,expectedHash,'Anvil returned a hash unrelated to the signed oversized transaction');
   await reference.send('evm_mine',[]);
   assert.equal(await reference.send('eth_getTransactionReceipt',[admittedHash]),null,'Anvil included invalid initcode in a block');
  }
  assert.deepEqual(await account(reference),referenceBefore,'Invalid initcode changed Anvil canonical account state');
  // Both chains must accept the same valid replacement at the rejected nonce.
  const recovery=await keys[0].signTransaction({chainId:1337,type,nonce,gasLimit:21000,...fees,to:keys[1].address,value:1});
  const hashes=await Promise.all([nano.send('eth_sendRawTransaction',[recovery]),reference.send('eth_sendRawTransaction',[recovery])]);
  assert.equal(hashes[0],hashes[1]);
  await reference.send('evm_mine',[]);
  for(const provider of [nano,reference]){
   let receipt;
   for(let n=0;n<300;++n){receipt=await provider.send('eth_getTransactionReceipt',[hashes[0]]);if(receipt)break;await new Promise(resolve=>setTimeout(resolve,100));}
   assert.equal(receipt?.status,'0x1','Valid transaction at rejected nonce did not execute');
   assert.equal(BigInt(receipt.gasUsed),21000n);
   assert.equal(BigInt(await provider.send('eth_getTransactionCount',[keys[0].address,'latest'])),BigInt(nonce)+1n);
  }
  admissionReport.push({type,initcodeBytes:49153,expectedHash,anvilReturnedHash:admittedHash??null,nanoRejected:true,anvilRejected:!admittedHash,referenceValidationMatches:!admittedHash,canonicalStateUnchanged:true,sameSignedRecovery:true,sameNonceRecovery:true});
  ++nonce;
 }
 const referenceValidationMatches=admissionReport.every(row=>row.referenceValidationMatches);
 console.log(JSON.stringify({reference:`Anvil ${fork}`,referenceVersion,nanoOversizedInitcodeRejected:fork==='Shanghai',referenceValidationMatches,admissionReport,sameSignedTransactions:true,results:report}));
 assert(report.every(row=>row.gasMatches),`Nano gas differs from Anvil ${fork}`);
 assert.notEqual(referenceValidationMatches,false,'Reference validation differs: oversized Shanghai initcode admitted by Anvil');
} finally {
 reference?.destroy();if(connection)await connection.close();
 if(child.exitCode===null){const exit=new Promise(resolve=>child.once('exit',resolve));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await exit;clearTimeout(timer);}
}
