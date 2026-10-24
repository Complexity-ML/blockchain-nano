// SOURCE proposal: exact retained public-fixture raw txs, one PRIVATE Anvil.
// No compiler, Native rerun, Wallet, new signature, remote RPC or real funds.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,why)=>{if(!ok)throw Error('Inventory raw differential: '+why);};
const sha=b=>createHash('sha256').update(b).digest('hex'),lower=s=>String(s).toLowerCase();
const root=s=>typeof s==='string'&&/^0x[0-9a-f]{64}$/.test(s);
const uint=s=>{need(typeof s==='string'&&/^(?:0x(?:0|[1-9a-f][0-9a-f]*)|0|[1-9][0-9]*)$/.test(s),'canonical quantity');return BigInt(s);};
const q=n=>'0x'+n.toString(16),COINBASE='0x0000000000000000000000000000000000009999',BALANCE=10n**24n;
const started=performance.now(),held=[],trace=[];let opened=0,closed=0,budget,commands,provider,child,logFd,output,inputPin;
let interrupted=false,hasFailure=false,failure,childClosed=false,childExit=null,childClose=null,stopRequested=false,groupJoined=false,completed=false,ready=false;
const retain=e=>{if(!hasFailure){hasFailure=true;failure=e;}};
const check=()=>{need(!interrupted&&performance.now()-started<540000,'deadline/interruption');budget?.check();};
const checked=async f=>{check();const result=await f();check();return result;};
const signal=()=>{interrupted=true;commands?.cancelAll(Error('Raw differential interrupted'));};
process.on('SIGINT',signal);process.on('SIGTERM',signal);
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
const same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
async function hold(file,pin=null,retainBytes=true){
 check();const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);++opened;const item={file,h};held.push(item);
 const a=await h.stat({bigint:true});check();need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=134217728n&&(!retainBytes||a.size<=16777216n),'bounded regular input');
 const hash=createHash('sha256'),chunks=[];let count=0;
 for(;;){check();const b=Buffer.alloc(65536),r=await h.read(b,0,b.length,null);check();if(!r.bytesRead)break;count+=r.bytesRead;need(BigInt(count)<=a.size,'input length');hash.update(b.subarray(0,r.bytesRead));if(retainBytes)chunks.push(Buffer.from(b.subarray(0,r.bytesRead)));}
 const digest=hash.digest('hex');need(BigInt(count)===a.size&&(!pin||digest===pin)&&same(nine(a),nine(await h.stat({bigint:true})))&&same(nine(a),nine(await fs.lstat(file,{bigint:true}))),'external SHA/full-nine/path fence');
 item.identity=nine(a);return {sha256:digest,bytes:count,data:retainBytes?Buffer.concat(chunks,count):null};
}
async function save(name,value){
 check();need(output&&/^[a-z0-9-]+\.json$/.test(name),'owned output leaf');const b=Buffer.from(JSON.stringify(value,null,2)+'\n');need(b.length<=16777216,'bounded output');
 const h=await fs.open(path.join(output,name),F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;
 try{check();await h.writeFile(b);check();await h.sync();check();}finally{await h.close();++closed;}
}
const importBytes=b=>import('data:text/javascript;base64,'+b.toString('base64'));
const argv=['--host','127.0.0.1','--port','18546','--chain-id','31337','--hardfork','shanghai','--gas-limit','60000000',
 '--accounts','0','--disable-default-create2-deployer','--base-fee','0','--gas-price','1','--timestamp','1700000000','--no-mining','--silent'];
try{
 need(process.argv.length===9,'CLI output nodeModules caseRoot anvilPath anvilSHA nativeAccounting nativeAccountingSHA');
 const [out,modules,caseRoot,anvilFile,anvilSha,accountingFile,accountingSha]=process.argv.slice(2);
 need([out,modules,caseRoot,anvilFile,accountingFile].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&[anvilSha,accountingSha].every(s=>/^[0-9a-f]{64}$/.test(s)),'absolute paths and external SHA pins');
 const d=await hold(path.join(caseRoot,'tests/multihost/campaign-deadline.mjs')),c=await hold(path.join(caseRoot,'tests/multihost/campaign-commands.mjs'));
 const {createCampaignDeadline}=await checked(()=>importBytes(d.data)),{createCampaignCommands}=await checked(()=>importBytes(c.data));
 budget=createCampaignDeadline({startedAt:started,maximumRunMs:540000,closeoutReserveMs:60000,onExpire:()=>commands?.cancelAll(Error('Raw differential budget'))});commands=createCampaignCommands({deadline:budget,spawn});
 const ethers=await checked(()=>import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href));need(ethers.version==='6.16.0','retained ethers6.16.0');
 inputPin=await hold(accountingFile,accountingSha);const native=JSON.parse(inputPin.data);
 need(native.format==='nano-inventory-v2-native-accounting-v1'&&native.executionKind==='nano-in-process-shanghai-v1'&&native.mined===false&&native.consensusValidated===false&&native.productionQualified===false&&native.transactionCount===65&&native.walletDebitChecks===65&&native.coinbaseFeeChecks===65&&Array.isArray(native.transactions)&&native.transactions.length===65,'retained native accounting scope/counts');
 const fixtures=[1,2,3].map(n=>lower(ethers.computeAddress('0x'+BigInt(n).toString(16).padStart(64,'0'))));
 const parsed=[];let nativeGas=0n,priorRoot=null;
 for(const [i,row]of native.transactions.entries()){
  check();need(row&&typeof row.rawSignedTransaction==='string'&&/^0x(?:[0-9a-f]{2})+$/.test(row.rawSignedTransaction)&&root(row.transactionHash)&&root(row.stateRootBefore)&&root(row.executionStateRoot),'exact retained raw/root encoding');
  const tx=ethers.Transaction.from(row.rawSignedTransaction),used=uint(row.gasUsed),amount=uint(row.value);
  need(tx.serialized===row.rawSignedTransaction&&tx.hash===row.transactionHash&&ethers.keccak256(row.rawSignedTransaction)===row.transactionHash&&lower(tx.from)===fixtures[0],'raw hash/recovered public pk1');
  need(tx.type===0&&tx.chainId===31337n&&tx.nonce===i&&tx.gasPrice===1n&&tx.gasLimit===30000000n&&tx.value===amount&&used>0n&&used<=30000000n,'exact signature domain/nonce/gas/value');
  need(row.mined===false&&typeof row.success==='boolean'&&row.walletDebitVerified===true&&row.coinbaseFeeVerified===true&&uint(row.effectiveGasPrice)===1n&&uint(row.nonceBefore)===BigInt(i)&&uint(row.nonceAfter)===BigInt(i+1)&&uint(row.executionStep)===BigInt(i+1),'native row evidence');
  need(uint(row.senderBalanceBefore)-uint(row.senderBalanceAfter)===used+(row.success?amount:0n)&&(!priorRoot||row.stateRootBefore===priorRoot),'native wallet/root continuity');
  priorRoot=row.executionStateRoot;nativeGas+=used;parsed.push({row,tx});
 }
 need(priorRoot===native.actualStateRoot&&nativeGas===uint(native.summedGasUsed)&&nativeGas===uint(native.actualCoinbaseFees),'native final root/gas reconciliation');
 await hold(anvilFile,anvilSha,false);check();try{await fs.lstat(out);throw Error('output exists');}catch(e){if(e.code!=='ENOENT')throw e;}
 await fs.mkdir(out,{mode:0o700});output=out;await save('input-binding.json',{nativeAccounting:{sha256:inputPin.sha256,bytes:inputPin.bytes},anvilSha256:anvilSha,argv,rawTransactionsResigned:false,nativeRerun:false,compiledNow:false,realFunds:false});
 logFd=await fs.open(path.join(output,'anvil.log'),F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;check();
 child=spawn(anvilFile,argv,{detached:true,stdio:['ignore',logFd.fd,logFd.fd],env:{PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'}});
 child.on('error',()=>retain(Error('owned Anvil spawn failed')));child.once('exit',(code,signal)=>{childExit={code,signal};if(!stopRequested)retain(Error('owned Anvil exited early'));});child.once('close',(code,signal)=>{childClosed=true;childClose={code,signal};});commands.ownPersistent(child);
 const fetch=new ethers.FetchRequest('http://127.0.0.1:18546/');fetch.timeout=3000;provider=new ethers.JsonRpcProvider(fetch,undefined,{batchMaxCount:1,cacheTimeout:-1});
 const rpc=async(method,params=[])=>{need(!hasFailure&&!childClosed,'owned reference remains live');return checked(()=>provider.send(method,params));};
 const until=Math.min(budget.workEnd,performance.now()+15000);let healthy=false;
 while(!healthy){check();need(!hasFailure&&!childClosed,'owned reference startup');try{need(/^anvil\//i.test(await rpc('web3_clientVersion')),'Anvil client');healthy=true;}catch(e){check();if(performance.now()>=until)throw e;await checked(()=>new Promise(r=>setTimeout(r,100)));}}
 need(uint(await rpc('eth_chainId'))===31337n&&uint(await rpc('eth_blockNumber'))===0n&&same(await rpc('eth_accounts'),[]),'fresh private chain without default signers');
 const create2='0x4e59b44847b379578588920ca78fbf26c0b4956c';
 need(await rpc('eth_getCode',[create2,'latest'])==='0x','default CREATE2 deployer absent');
 for(const address of fixtures){await rpc('anvil_setBalance',[address,q(BALANCE)]);need(uint(await rpc('eth_getBalance',[address,'latest']))===BALANCE&&uint(await rpc('eth_getTransactionCount',[address,'latest']))===0n&&await rpc('eth_getCode',[address,'latest'])==='0x','only explicit funded public fixture accounts');}
 await rpc('anvil_setCoinbase',[COINBASE]);need(lower(await rpc('eth_coinbase'))===COINBASE&&uint(await rpc('eth_getBalance',[COINBASE,'latest']))===0n,'fixed initially empty coinbase');
 let gas=0n;
 for(const [i,{row,tx}]of parsed.entries()){
  check();const step=BigInt(i+1),timestamp=1700000000+12*(i+1);need(uint(await rpc('eth_blockNumber'))===step-1n&&uint(await rpc('eth_getTransactionCount',[fixtures[0],'latest']))===BigInt(i)&&uint(await rpc('eth_getBalance',[fixtures[0],'latest']))===uint(row.senderBalanceBefore),'exact pre-transaction chain/nonce/balance');
  await rpc('evm_setNextBlockTimestamp',[timestamp]);const sent=await rpc('eth_sendRawTransaction',[row.rawSignedTransaction]);need(sent===row.transactionHash,'broadcast exact retained hash');await rpc('evm_mine');
  const receipt=await rpc('eth_getTransactionReceipt',[row.transactionHash]),actual=await rpc('eth_getTransactionByHash',[row.transactionHash]);need(receipt&&actual&&root(receipt.blockHash),'actual mined receipt');
  const block=await rpc('eth_getBlockByHash',[receipt.blockHash,false]);need(block&&root(block.hash)&&root(block.stateRoot),'actual canonical block/state root');
  const afterBalance=uint(await rpc('eth_getBalance',[fixtures[0],'latest'])),afterNonce=uint(await rpc('eth_getTransactionCount',[fixtures[0],'latest'])),feeBalance=uint(await rpc('eth_getBalance',[COINBASE,'latest']));
  const checks={hash:receipt.transactionHash===row.transactionHash&&actual.hash===row.transactionHash,sender:lower(actual.from)===fixtures[0]&&lower(receipt.from)===fixtures[0],nonce:uint(actual.nonce)===BigInt(i)&&afterNonce===step,
   status:uint(receipt.status)===(row.success?1n:0n),gasUsed:uint(receipt.gasUsed)===uint(row.gasUsed),gasPrice:uint(receipt.effectiveGasPrice)===1n&&uint(actual.gasPrice)===1n,
   signedFields:(actual.to===null?null:lower(actual.to))===(tx.to===null?null:lower(tx.to))&&lower(actual.input)===lower(tx.data)&&uint(actual.value)===tx.value&&uint(actual.gas)===tx.gasLimit,
   block:uint(block.number)===step&&uint(receipt.blockNumber)===step&&uint(actual.blockNumber)===step&&uint(await rpc('eth_blockNumber'))===step&&block.hash===receipt.blockHash&&actual.blockHash===block.hash&&Array.isArray(block.transactions)&&block.transactions.length===1&&block.transactions[0]===row.transactionHash,
   context:uint(block.timestamp)===BigInt(timestamp)&&uint(block.gasLimit)===60000000n&&uint(block.baseFeePerGas)===0n&&lower(block.miner)===COINBASE,
   wallet:afterBalance===uint(row.senderBalanceAfter),coinbase:feeBalance===gas+uint(row.gasUsed),stateRoot:block.stateRoot===row.executionStateRoot};
  trace.push({step:Number(step),transactionHash:row.transactionHash,native:{success:row.success,gasUsed:row.gasUsed,stateRoot:row.executionStateRoot},anvil:{status:receipt.status,gasUsed:receipt.gasUsed,stateRoot:block.stateRoot,blockHash:block.hash,timestamp:block.timestamp,miner:block.miner},checks});
  need(Object.values(checks).every(x=>x===true),'exact differential mismatch at step '+step+': '+Object.entries(checks).filter(([,ok])=>!ok).map(([key])=>key).join(','));gas+=uint(receipt.gasUsed);
 }
 need(trace.length===65&&gas===nativeGas,'all65 exact raw/gas/root comparisons');completed=true;
}catch(e){retain(e);}
finally{
 if(budget)try{budget.enterCloseout();}catch(e){retain(e);}
 if(commands)try{await commands.closeAll();}catch(e){retain(e);}
 if(provider)try{provider.destroy();}catch(e){retain(e);}
 if(child&&commands)try{if(childClosed||child.exitCode!==null||child.signalCode!==null)retain(Error('owned Anvil stopped before requested closeout'));stopRequested=true;await commands.terminatePersistent(child,()=>childClosed,1000);groupJoined=childClosed;}catch(e){retain(e);}
 if(logFd){try{await logFd.sync();need((await logFd.stat()).size<=1048576,'bounded Anvil log');}catch(e){retain(e);}finally{try{await logFd.close();++closed;}catch(e){retain(e);}}}
 for(const item of held)try{need(item.identity&&same(nine(await item.h.stat({bigint:true})),item.identity)&&same(nine(await fs.lstat(item.file,{bigint:true})),item.identity),'final input FD/path full-nine fence');}catch(e){retain(e);}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){retain(e);}
 try{check();need(completed&&!hasFailure&&groupJoined&&opened===closed,'complete trace and closeout');ready=true;}catch(e){retain(e);ready=false;}
 if(output)try{await save('raw-differential-trace.json',trace);await save('raw-differential-controller.json',{format:'nano-inventory-v2-raw-differential-controller-v1',passed:false,terminalEvidenceReady:ready&&!hasFailure,externalTerminalExitRequired:true,
  comparedTransactions:trace.length,matchedTransactions:trace.filter(t=>Object.values(t.checks).every(Boolean)).length,firstMismatch:trace.find(t=>!Object.values(t.checks).every(Boolean))??null,nativeAccounting:inputPin?{sha256:inputPin.sha256,bytes:inputPin.bytes}:null,
  childExit,childClose,ownedAnvilGroupJoined:groupJoined,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,failure:hasFailure?String(failure?.message??failure):null,
  nativeRerun:false,newSignatures:false,rawTransactionsResigned:false,compiledNow:false,realAssetsUsed:false,consensusValidated:false,networkForkQualified:false,productionQualified:false});check();}catch(e){retain(e);ready=false;}
 try{check();need(opened===closed,'all publication FDs closed');}catch(e){retain(e);ready=false;}
 budget?.dispose();process.removeListener('SIGINT',signal);process.removeListener('SIGTERM',signal);if(interrupted||performance.now()-started>=540000){retain(Error('final deadline/interruption'));ready=false;}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-raw-differential-terminal-v1',passed:false,terminalEvidenceReady:ready&&!hasFailure,actualTerminalExitRequired:true,comparedTransactions:trace.length,ownedAnvilGroupJoined:groupJoined,openedDescriptors:opened,closedDescriptors:closed,failure:hasFailure?String(failure?.message??failure):null,productionQualified:false})+'\n');process.exitCode=ready&&!hasFailure?0:1;
}
