import { network, artifacts } from 'hardhat';
import { BrowserProvider, JsonRpcProvider, Wallet, Interface, getCreateAddress } from 'ethers';
import { spawn } from 'node:child_process';
import assert from 'node:assert/strict';
const binary=process.env.ANVIL_BINARY;
if(!binary)throw Error('ANVIL_BINARY is required');
const child=spawn(binary,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork','paris','--gas-limit','30000000','--base-fee','1000000000'],{stdio:['ignore','pipe','pipe']});
let text='';let reference:JsonRpcProvider|undefined;let connection;
try {
 const port=await new Promise<string>((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Anvil startup timeout')),10000);child.once('error',reject);child.once('exit',()=>{clearTimeout(timer);reject(Error('Anvil exited'));});child.stdout.on('data',chunk=>{text+=chunk;const match=text.match(/Listening on 127\.0\.0\.1:(\d+)/);if(match){clearTimeout(timer);resolve(match[1]);}});});
 reference=new JsonRpcProvider(`http://127.0.0.1:${port}`,1337,{cacheTimeout:-1});
 connection=await network.create('nano');const nano=new BrowserProvider(connection.provider);nano.pollingInterval=100;
 const keys=[1,2].map(n=>new Wallet('0x'+n.toString(16).padStart(64,'0')));
 for(const key of keys)await reference.send('anvil_setBalance',[key.address,'0xd3c21bcecceda1000000']);
 const artifact=await artifacts.readArtifact('DeveloperToken');const abi=new Interface(artifact.abi);
 const token=getCreateAddress({from:keys[0].address,nonce:0});
 const cases=[
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
 const report=[];
 for(const test of cases){
  const raw=await keys[test.signer].signTransaction({chainId:1337,type:0,nonce:test.nonce,gasLimit:3000000,gasPrice:2000000000,to:test.to,data:test.data});
  const hashes=await Promise.all([nano.send('eth_sendRawTransaction',[raw]),reference.send('eth_sendRawTransaction',[raw])]);assert.equal(hashes[0],hashes[1]);
  async function receipt(provider:any){for(let n=0;n<300;++n){const r=await provider.send('eth_getTransactionReceipt',[hashes[0]]);if(r)return r;await new Promise(resolve=>setTimeout(resolve,100));}throw Error('receipt timeout');}
  const [actual,expected]=await Promise.all([receipt(nano),receipt(reference)]);
  const logs=(r:any)=>r.logs.map((l:any)=>({address:l.address.toLowerCase(),topics:l.topics,data:l.data}));
  assert.equal(actual.status,expected.status,`${test.name}: status differs`);assert.deepEqual(logs(actual),logs(expected),`${test.name}: logs differ`);
  if(test.to===storage){const call={to:storage,data:storageAbi.encodeFunctionData('value',[])};assert.equal(await nano.send('eth_call',[call,'latest']),await reference.send('eth_call',[call,'latest']),`${test.name}: storage differs`);}
  report.push({case:test.name,nanoGas:BigInt(actual.gasUsed).toString(),anvilGas:BigInt(expected.gasUsed).toString(),gasMatches:BigInt(actual.gasUsed)===BigInt(expected.gasUsed)});
 }
 for(const method of [['totalSupply',[]],['balanceOf',[keys[0].address]],['balanceOf',[keys[1].address]]] as const){const call={to:token,data:abi.encodeFunctionData(method[0],method[1])};assert.equal(await nano.send('eth_call',[call,'latest']),await reference.send('eth_call',[call,'latest']));}
 console.log(JSON.stringify({reference:'Anvil Paris',sameSignedTransactions:true,results:report}));
 assert(report.every(row=>row.gasMatches),'Nano gas differs from Anvil Paris');
} finally {
 reference?.destroy();if(connection)await connection.close();
 if(child.exitCode===null){const exit=new Promise(resolve=>child.once('exit',resolve));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await exit;clearTimeout(timer);}
}
