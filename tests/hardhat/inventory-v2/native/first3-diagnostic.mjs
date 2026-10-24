// SOURCE diagnostic only: reproduce the first THREE retained transactions with
// existing helper + private Anvil; expected root divergence is NEVER equivalence.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,why)=>{if(!ok)throw Error('Inventory first3 diagnostic: '+why);};
const sha=b=>createHash('sha256').update(b).digest('hex'),lower=s=>String(s).toLowerCase();
const root=s=>typeof s==='string'&&/^0x[0-9a-f]{64}$/.test(s);
const uint=s=>{need(typeof s==='string'&&/^(?:0x(?:0|[1-9a-f][0-9a-f]*)|0|[1-9][0-9]*)$/.test(s),'canonical quantity');return BigInt(s);};
const q=n=>'0x'+n.toString(16),COINBASE='0x0000000000000000000000000000000000009999',BALANCE=10n**24n;
const started=performance.now(),held=[],trace=[],accounts=[];let opened=0,closed=0,budget,commands,provider,child,logFd,output,inputPin,retainedTracePin;
let interrupted=false,hasFailure=false,failure,childClosed=false,childExit=null,childClose=null,stopRequested=false,groupJoined=false,completed=false,ready=false,nativeStorageAvailable=false;
let nativeChild,nativeClosed=false,nativeExit=null,nativeClose=null,nativeJoined=false,nativeEOF=false,nativePending=null,nativeCarry=Buffer.alloc(0),nativeStderr=Buffer.alloc(0),nativeBytes=0,nativeRequests=0,nativeCompleted=0;
const retain=e=>{if(!hasFailure){hasFailure=true;failure=e;}};
const check=()=>{need(!interrupted&&performance.now()-started<120000,'deadline/interruption');budget?.check();};
const checked=async f=>{check();const result=await f();check();return result;};
const signal=()=>{interrupted=true;rejectNative(Error('First3 interrupted'));commands?.cancelAll(Error('First3 interrupted'));};
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
function rejectNative(e){retain(e);if(nativePending){const p=nativePending;nativePending=null;clearTimeout(p.timer);p.reject(e);}}
function nativeData(chunk){try{check();nativeBytes+=chunk.length;need(nativeBytes<=8388608,'native stdout bound');nativeCarry=Buffer.concat([nativeCarry,chunk]);
 for(;;){const end=nativeCarry.indexOf(10);if(end<0){need(nativeCarry.length<=2097152,'native line bound');break;}need(end<=2097152,'native response bound');const line=nativeCarry.subarray(0,end);nativeCarry=Buffer.from(nativeCarry.subarray(end+1));const r=JSON.parse(line.toString('utf8')),p=nativePending;
  need(p&&r.id===p.id&&typeof r.ok==='boolean','one matched native response');if(!r.ok){nativePending=null;clearTimeout(p.timer);const e=Error('native protocol refusal');retain(e);p.reject(e);continue;}
  need(r.result&&typeof r.result==='object','native result');nativePending=null;clearTimeout(p.timer);p.resolve(r.result);}
 }catch(e){rejectNative(e);}}
function nativeRequest(action,fields={}){check();need(nativeChild&&!nativeClosed&&!nativePending&&!nativeEOF&&!hasFailure&&nativeRequests<100,'one owned native request');const id=++nativeRequests,b=Buffer.from(JSON.stringify({id,chain:0,action,...fields})+'\n');need(b.length<=2097152,'native request bound');
 return new Promise((resolve,reject)=>{const timer=setTimeout(()=>rejectNative(Error('native request timeout')),budget.remaining(30000));nativePending={id,resolve,reject,timer};try{nativeChild.stdin.write(b,e=>{if(e)rejectNative(Error('native stdin refused'));});}catch(e){rejectNative(e);}});}
async function nativeInvoke(action,fields={}){const r=await checked(()=>nativeRequest(action,fields));need(r.executionKind==='nano-in-process-shanghai-v1'&&r.mined===false&&r.syntheticContext===true&&r.consensusValidated===false&&root(r.executionStateRoot)&&!Object.hasOwn(r,'blockHash'),'actual non-mined native context');return r;}
const argv=['--host','127.0.0.1','--port','18546','--chain-id','31337','--hardfork','shanghai','--gas-limit','60000000',
 '--accounts','0','--disable-default-create2-deployer','--base-fee','0','--gas-price','1','--timestamp','1700000000','--no-mining','--silent'];
try{
 need(process.argv.length===13,'CLI output modules caseRoot anvil anvilSHA shim shimSHA nativeAccounting nativeSHA retainedRawTrace traceSHA');
 const [out,modules,caseRoot,anvilFile,anvilSha,shim,shimSha,accountingFile,accountingSha,retainedTraceFile,retainedTraceSha]=process.argv.slice(2);
 need([out,modules,caseRoot,anvilFile,shim,accountingFile,retainedTraceFile].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&[anvilSha,shimSha,accountingSha,retainedTraceSha].every(s=>/^[0-9a-f]{64}$/.test(s)),'absolute paths and external SHA pins');
 const d=await hold(path.join(caseRoot,'tests/multihost/campaign-deadline.mjs')),c=await hold(path.join(caseRoot,'tests/multihost/campaign-commands.mjs'));
 const {createCampaignDeadline}=await checked(()=>importBytes(d.data)),{createCampaignCommands}=await checked(()=>importBytes(c.data));
 budget=createCampaignDeadline({startedAt:started,maximumRunMs:120000,closeoutReserveMs:20000,onExpire:()=>commands?.cancelAll(Error('First3 diagnostic budget'))});commands=createCampaignCommands({deadline:budget,spawn});
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
 retainedTracePin=await hold(retainedTraceFile,retainedTraceSha);const old=JSON.parse(retainedTracePin.data);need(Array.isArray(old)&&old.length===3,'exact retained failed three-row trace');
 for(let i=0;i<3;i++){const r=old[i],n=parsed[i].row;need(r.step===i+1&&r.transactionHash===n.transactionHash&&r.native.stateRoot===n.executionStateRoot&&uint(r.anvil.gasUsed)===uint(n.gasUsed)&&root(r.anvil.stateRoot),'retained trace identity/gas/root');need(Object.entries(r.checks).every(([k,v])=>v===(i===2&&k==='stateRoot'?false:true))&&r.checks.stateRoot===(i!==2),'retained failure is exactly third stateRoot');}
 need(old[0].anvil.stateRoot===parsed[0].row.executionStateRoot&&old[1].anvil.stateRoot===parsed[1].row.executionStateRoot&&old[2].anvil.stateRoot!==parsed[2].row.executionStateRoot,'retain two matches and original mismatch');
 await hold(anvilFile,anvilSha,false);await hold(shim,shimSha,false);check();try{await fs.lstat(out);throw Error('output exists');}catch(e){if(e.code!=='ENOENT')throw e;}
 await fs.mkdir(out,{mode:0o700});output=out;await save('input-binding.json',{nativeAccounting:{sha256:inputPin.sha256,bytes:inputPin.bytes},retainedTrace:{sha256:retainedTracePin.sha256,bytes:retainedTracePin.bytes},anvilSha256:anvilSha,shimSha256:shimSha,argv,requestedNativeDiagnosticPrefix:3,compiledNow:false,realFunds:false});
 nativeChild=spawn(shim,[],{detached:true,stdio:['pipe','pipe','pipe'],env:{PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'}});nativeChild.on('error',()=>rejectNative(Error('owned native spawn failed')));
 nativeChild.once('exit',(code,signal)=>{nativeExit={code,signal};if(!nativeEOF)rejectNative(Error('native exited before requested EOF'));});nativeChild.once('close',(code,signal)=>{nativeClosed=true;nativeClose={code,signal};if(nativePending)rejectNative(Error('native closed with pending request'));});
 nativeChild.stdout.on('data',nativeData);nativeChild.stdout.on('error',()=>rejectNative(Error('native stdout failed')));nativeChild.stdin.on('error',()=>rejectNative(Error('native stdin failed')));nativeChild.stderr.on('error',()=>rejectNative(Error('native stderr failed')));nativeChild.stderr.on('data',b=>{if(nativeStderr.length+b.length>16384)rejectNative(Error('native stderr bound'));else nativeStderr=Buffer.concat([nativeStderr,b]);});commands.ownPersistent(nativeChild);
 const metadata=await nativeInvoke('metadata');need(metadata.evmRevision==='Shanghai'&&uint(metadata.chainId)===31337n&&uint(metadata.executionStep)===0n&&same(metadata.signers,fixtures)&&metadata.executionStateRoot===parsed[0].row.stateRootBefore,'original fresh native state');
 for(const {row,tx}of parsed.slice(0,3)){const r=await nativeInvoke('send',{from:fixtures[0],to:tx.to===null?null:lower(tx.to),data:lower(tx.data),value:tx.value.toString(),gas:'30000000'});need(r.rawSignedTransaction===row.rawSignedTransaction&&r.transactionHash===row.transactionHash&&r.success===row.success&&uint(r.gasUsed)===uint(row.gasUsed)&&r.stateRootBefore===row.stateRootBefore&&r.executionStateRoot===row.executionStateRoot&&uint(r.executionStep)===uint(row.executionStep),'native first3 exact raw/gas/root reproduction');++nativeCompleted;}
 const top=parsed.slice(0,3).map(({tx})=>{need(tx.to===null,'three original CREATE transactions');return lower(ethers.getCreateAddress({from:fixtures[0],nonce:tx.nonce}));}),children=Array.from({length:5},(_,i)=>lower(ethers.getCreateAddress({from:top[2],nonce:i+1}))),addresses=[...fixtures,COINBASE,...top,...children,'0x'+'0'.repeat(40),'0x4e59b44847b379578588920ca78fbf26c0b4956c'];
 need(addresses.length===14&&new Set(addresses).size===14,'twelve source-derived accounts plus two absence controls');
 const nativeAccounts=new Map();for(const address of addresses){const r=await nativeInvoke('account',{address});need(r.address===address&&r.executionStateRoot===parsed[2].row.executionStateRoot&&uint(r.executionStep)===3n&&typeof r.exists==='boolean'&&/^0x(?:[0-9a-f]{2})*$/.test(r.code)&&r.codeHash===ethers.keccak256(r.code),'readonly native account at exact step3');nativeAccounts.set(address,r);}
 const optional=addresses.map(a=>nativeAccounts.get(a));nativeStorageAvailable=optional.every(n=>Object.hasOwn(n,'storageRoot')&&Object.hasOwn(n,'storage'));need(nativeStorageAvailable||optional.every(n=>!Object.hasOwn(n,'storageRoot')&&!Object.hasOwn(n,'storage')),'uniform optional native storage fields');
 if(nativeStorageAvailable)for(const n of optional){need(root(n.storageRoot)&&n.storage!==null&&typeof n.storage==='object'&&!Array.isArray(n.storage)&&Object.keys(n.storage).length<=1024,'bounded native storage diagnostic');for(const [key,value]of Object.entries(n.storage)){need(/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(key)&&key.length<=66&&typeof value==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(value)&&value.length<=66,'canonical uint256 storage entry');}}
 need((await nativeInvoke('root')).executionStateRoot===parsed[2].row.executionStateRoot,'account reads leave native root unchanged');nativeEOF=true;nativeChild.stdin.end();
 await checked(()=>new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('native EOF timeout')),budget.remaining(5000));const done=()=>{clearTimeout(timer);resolve();};if(nativeClosed)done();else nativeChild.once('close',done);}));need(!nativePending&&nativeCarry.length===0&&nativeStderr.length===0&&nativeExit?.code===0&&nativeExit?.signal===null&&nativeClose?.code===0&&nativeClose?.signal===null,'clean native EOF/exit/close');await commands.terminatePersistent(nativeChild,()=>nativeClosed,1000);nativeJoined=nativeClosed;
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
 for(const [i,{row,tx}]of parsed.slice(0,3).entries()){
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
  need(Object.entries(checks).every(([key,ok])=>ok===(i===2&&key==='stateRoot'?false:true))&&block.stateRoot===old[i].anvil.stateRoot,'exact retained three-step diagnostic reproduction');gas+=uint(receipt.gasUsed);
 }
 need(trace.length===3&&gas===parsed.slice(0,3).reduce((s,{row})=>s+uint(row.gasUsed),0n),'exact first3 gas and expected root mismatch');
 for(const address of addresses){const n=nativeAccounts.get(address),keys=nativeStorageAvailable?Object.keys(n.storage):[],keyWords=keys.map(k=>'0x'+uint(k).toString(16).padStart(64,'0')),proof=await rpc('eth_getProof',[address,keyWords,'0x3']),code=await rpc('eth_getCode',[address,'0x3']);need(lower(proof.address)===address&&root(proof.storageHash)&&root(proof.codeHash)&&Array.isArray(proof.accountProof)&&Array.isArray(proof.storageProof)&&proof.storageProof.length===keys.length&&proof.codeHash===ethers.keccak256(code),'actual Anvil proof/code envelope');
  const matches={nonce:uint(n.nonce)===uint(proof.nonce),balance:uint(n.balance)===uint(proof.balance),code:n.code===code,codeHash:n.codeHash===proof.codeHash},slots=[];
  if(nativeStorageAvailable){matches.storageRoot=n.storageRoot===proof.storageHash;for(const [i,key]of keys.entries()){const value=await rpc('eth_getStorageAt',[address,keyWords[i],'0x3']);need(/^0x[0-9a-f]{64}$/.test(value),'actual storage word');slots.push({key,native:n.storage[key],anvil:value,matches:uint(n.storage[key])===BigInt(value)});}matches.knownStorageValues=slots.every(s=>s.matches);}
  accounts.push({address,native:{nonce:n.nonce,balance:n.balance,code:n.code,codeHash:n.codeHash,exists:n.exists,...(nativeStorageAvailable?{storageRoot:n.storageRoot,storage:n.storage}:{})},anvil:{nonce:proof.nonce,balance:proof.balance,code,codeHash:proof.codeHash,storageHash:proof.storageHash,accountProof:proof.accountProof,storageProof:proof.storageProof},matches,slots,storageCompared:nativeStorageAvailable,referenceStorageEnumerationComplete:false,proofCryptographicallyVerified:false});}
 const finalBlock=await rpc('eth_getBlockByNumber',['0x3',false]);need(finalBlock.stateRoot===old[2].anvil.stateRoot&&uint(await rpc('eth_blockNumber'))===3n,'readonly Anvil account queries preserve original mismatching root');completed=true;
}catch(e){retain(e);}
finally{
 if(budget)try{budget.enterCloseout();}catch(e){retain(e);}
 if(commands)try{await commands.closeAll();}catch(e){retain(e);}
 if(nativeChild&&commands)try{await commands.terminatePersistent(nativeChild,()=>nativeClosed,1000);nativeJoined=nativeClosed;}catch(e){retain(e);}
 if(provider)try{provider.destroy();}catch(e){retain(e);}
 if(child&&commands)try{if(childClosed||child.exitCode!==null||child.signalCode!==null)retain(Error('owned Anvil stopped before requested closeout'));stopRequested=true;await commands.terminatePersistent(child,()=>childClosed,1000);groupJoined=childClosed;}catch(e){retain(e);}
 if(logFd){try{await logFd.sync();need((await logFd.stat()).size<=1048576,'bounded Anvil log');}catch(e){retain(e);}finally{try{await logFd.close();++closed;}catch(e){retain(e);}}}
 for(const item of held)try{need(item.identity&&same(nine(await item.h.stat({bigint:true})),item.identity)&&same(nine(await fs.lstat(item.file,{bigint:true})),item.identity),'final input FD/path full-nine fence');}catch(e){retain(e);}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){retain(e);}
 try{check();need(completed&&!hasFailure&&groupJoined&&nativeJoined&&accounts.length===14&&opened===closed,'complete diagnostic and closeout');ready=true;}catch(e){retain(e);ready=false;}
 if(output)try{await save('first3-reproduction.json',trace);await save('paired-account-diagnostic.json',accounts);await save('first3-diagnostic-controller.json',{format:'nano-inventory-v2-first3-diagnostic-controller-v1',passed:false,terminalEvidenceReady:ready&&!hasFailure,externalTerminalExitRequired:true,diagnosticOnly:true,expectedRootMismatchReproduced:completed,differentialPassed:false,
  comparedTransactions:trace.length,accountPairs:accounts.length,differingAccounts:accounts.filter(a=>Object.values(a.matches).some(ok=>!ok)).map(a=>a.address),nativeStorageFieldsAvailable:nativeStorageAvailable,storageRootsAndKnownSlotsCompared:completed&&nativeStorageAvailable,referenceStorageEnumerationComplete:false,proofCryptographicallyVerified:false,nativeAccounting:inputPin?{sha256:inputPin.sha256,bytes:inputPin.bytes}:null,
  nativeExit,nativeClose,ownedNativeGroupJoined:nativeJoined,nativeRequests,nativeStdoutBytes:nativeBytes,
  childExit,childClose,ownedAnvilGroupJoined:groupJoined,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,failure:hasFailure?String(failure?.message??failure):null,
  nativeDiagnosticPrefixReproduced:nativeCompleted,publicFixtureSignaturesRecomputed:nativeCompleted>0,rawSignedBytesRequiredIdentical:true,newIdentities:false,compiledNow:false,realAssetsUsed:false,consensusValidated:false,networkForkQualified:false,productionQualified:false});check();}catch(e){retain(e);ready=false;}
 try{check();need(opened===closed,'all publication FDs closed');}catch(e){retain(e);ready=false;}
 budget?.dispose();process.removeListener('SIGINT',signal);process.removeListener('SIGTERM',signal);if(interrupted||performance.now()-started>=120000){retain(Error('final deadline/interruption'));ready=false;}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-first3-diagnostic-terminal-v1',passed:false,diagnosticOnly:true,differentialPassed:false,terminalEvidenceReady:ready&&!hasFailure,actualTerminalExitRequired:true,comparedTransactions:trace.length,accountPairs:accounts.length,ownedNativeGroupJoined:nativeJoined,ownedAnvilGroupJoined:groupJoined,openedDescriptors:opened,closedDescriptors:closed,failure:hasFailure?String(failure?.message??failure):null,productionQualified:false})+'\n');process.exitCode=ready&&!hasFailure?0:1;
}
