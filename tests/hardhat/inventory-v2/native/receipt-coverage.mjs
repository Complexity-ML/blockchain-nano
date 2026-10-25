// Exact retained raw65; one owned C++ helper, retained Geth outputs only.
// Parent authenticates Core/link/tool provenance and actual terminal closure.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,m)=>{if(!ok)throw Error('Inventory receipt coverage: '+m);};
const sha=b=>createHash('sha256').update(b).digest('hex'),same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const quantity=n=>'0x'+BigInt(n).toString(16),uint=s=>{need(typeof s==='string'&&/^(?:0x[0-9a-f]+|0|[1-9][0-9]*)$/i.test(s),'integer quantity');return BigInt(s);};
const hex=(s,n=null)=>typeof s==='string'&&/^0x(?:[0-9a-f]{2})*$/i.test(s)&&(n===null||s.length===2+n*2);
const ROOT=s=>hex(s,32),KIND='nano-in-process-shanghai-v1',COINBASE='0x0000000000000000000000000000000000009999';
const started=performance.now(),held=[],trace=[],protocol=[];
let budget,commands,child,childClosed=false,childExit=null,childClose=null,carry=Buffer.alloc(0),stderr=Buffer.alloc(0),pending=null;
let opened=0,closed=0,requests=0,stdoutBytes=0,eofRequested=false,groupJoined=false,interrupted=false,failed=false,failure,ready=false,output;
const retain=e=>{if(!failed){failed=true;failure=e;}};
const check=()=>{need(!interrupted&&performance.now()-started<540000,'deadline/interruption');budget?.check();};
const checked=async f=>{check();const r=await f();check();return r;};
function fail(e){retain(e);if(pending){const p=pending;pending=null;clearTimeout(p.timer);p.reject(e);}}
const signal=()=>{interrupted=true;fail(Error('receipt coverage interrupted'));commands?.cancelAll(Error('receipt coverage interrupted'));};
process.on('SIGINT',signal);process.on('SIGTERM',signal);
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
async function hold(file,expected=null,retainBytes=true){
 check();const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);++opened;const item={file,h};held.push(item);
 const a=await h.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=134217728n&&(!retainBytes||a.size<=16777216n),'bounded regular input');
 const hash=createHash('sha256'),parts=[];let bytes=0;
 for(;;){check();const b=Buffer.alloc(65536),r=await h.read(b,0,b.length,null);check();if(!r.bytesRead)break;bytes+=r.bytesRead;need(BigInt(bytes)<=a.size,'input length');hash.update(b.subarray(0,r.bytesRead));if(retainBytes)parts.push(Buffer.from(b.subarray(0,r.bytesRead)));}
 const digest=hash.digest('hex');need(BigInt(bytes)===a.size&&(!expected||digest===expected)&&same(nine(a),nine(await h.stat({bigint:true})))&&same(nine(a),nine(await fs.lstat(file,{bigint:true}))),'SHA/full-nine/path admission');
 item.identity=nine(a);return {sha256:digest,bytes,data:retainBytes?Buffer.concat(parts,bytes):null};
}
async function save(name,value){
 check();need(output&&/^[a-z0-9-]+\.json$/.test(name),'owned output leaf');const b=Buffer.from(JSON.stringify(value,null,2)+'\n');need(b.length<=16777216,'output size');
 const h=await fs.open(path.join(output,name),F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;
 try{check();await h.writeFile(b);check();await h.sync();check();}finally{await h.close();++closed;}
}
const fromBytes=b=>import('data:text/javascript;base64,'+b.toString('base64'));
function request(action,fields={}){
 check();need(child&&!childClosed&&!failed&&!pending&&!eofRequested&&requests<132,'one live bounded owned request');
 const id=++requests,b=Buffer.from(JSON.stringify({id,chain:0,action,...fields})+'\n');need(b.length<=2097152,'request line size');
 return new Promise((resolve,reject)=>{const timer=setTimeout(()=>fail(Error('owned helper request timeout')),budget.remaining(30000));pending={id,action,resolve,reject,timer};
  try{check();child.stdin.write(b,e=>{if(e)fail(Error('owned helper stdin write failed'));});}catch(e){fail(e);}
 });
}
function stdout(chunk){
 try{check();stdoutBytes+=chunk.length;need(stdoutBytes<=33554432,'stdout total size');carry=Buffer.concat([carry,chunk]);
  for(;;){const end=carry.indexOf(10);if(end<0){need(carry.length<=2097152,'response line size');break;}need(end<=2097152,'response line size');
   const line=carry.subarray(0,end);carry=Buffer.from(carry.subarray(end+1));const r=JSON.parse(line.toString('utf8')),p=pending;
   need(p&&r.id===p.id&&typeof r.ok==='boolean','one matched protocol response');
   need(r.ok===true,'helper protocol refusal is fatal, never an EVM REVERT');
   need(r.result&&r.result.executionKind===KIND&&r.result.mined===false&&r.result.consensusValidated===false&&ROOT(r.result.executionStateRoot),'actual unmined execution envelope');
   protocol.push({id:p.id,action:p.action,executionStep:r.result.executionStep,executionStateRoot:r.result.executionStateRoot});
   pending=null;clearTimeout(p.timer);p.resolve(r.result);
  }
 }catch(e){fail(e);}
}
function logs(value){
 need(Array.isArray(value)&&value.length<=100000,'bounded log array');
 return value.map(l=>{need(l&&hex(l.address,20)&&Array.isArray(l.topics)&&l.topics.length<=4&&l.topics.every(t=>hex(t,32))&&hex(l.data),'log address/topics/data widths');
  return {address:l.address.toLowerCase(),topics:l.topics.map(t=>t.toLowerCase()),data:l.data.toLowerCase()};
 });
}
function bloomFromLogs(rows,ethers){
 const out=Buffer.alloc(256);
 for(const log of rows)for(const value of [log.address,...log.topics]){
  const hash=ethers.getBytes(ethers.keccak256(value));
  for(let i=0;i<6;i+=2){const bit=((hash[i]<<8)|hash[i+1])&2047;out[255-Math.floor(bit/8)]|=1<<(bit%8);}
 }
 return '0x'+out.toString('hex');
}
function receiptFromReference(receipt,ethers){
 need(Object.hasOwn(receipt,'logs'),'reference receipt must own logs');
 const rows=logs(receipt.logs===null?[]:receipt.logs),status=uint(receipt.status),cumulative=uint(receipt.cumulativeGasUsed);
 need(status===0n||status===1n,'binary reference status');const bloom=bloomFromLogs(rows,ethers);
 need(hex(receipt.logsBloom,256)&&receipt.logsBloom.toLowerCase()===bloom,'reference log-derived bloom');
 const integer=n=>n===0n?'0x':ethers.toBeHex(n);
 const encoded=ethers.encodeRlp([integer(status),integer(cumulative),bloom,rows.map(l=>[l.address,l.topics,l.data])]);
 // Non-secure receipt trie key is RLP(index0)=0x80: two nibbles, even leaf path0x2080.
 const singleton=ethers.keccak256(ethers.encodeRlp(['0x2080',encoded]));
 return {rows,bloom,encoded,singleton,status,cumulative};
}
function rootAdoption(receipt,tracePin,nativePin){
 const keys=['format','passed','sourceCandidate','nativeAccountingSha256','referenceTrace','referenceRootResult','matchedTransactions','commands','actualDockerExitCode','completeResultFilesVerified','allObservedFilesUnchanged','transactionsReplayedByAdoption','newReferenceExecution','newNativeExecution','consensusQualified','productionQualified'];
 need(receipt&&same(Object.keys(receipt).sort(),keys.sort())&&receipt.format==='nano-retained-t8n-receipt-reference-adoption-v1'&&receipt.passed===true,'explicit external Root reference adoption');
 need(/^[0-9a-f]{40}$/.test(receipt.sourceCandidate)&&receipt.nativeAccountingSha256===nativePin.sha256&&receipt.referenceTrace?.sha256===tracePin.sha256&&receipt.referenceTrace.bytes===tracePin.bytes,'Root adoption exact native/trace bindings');
 need(receipt.referenceRootResult&&/^[0-9a-f]{64}$/.test(receipt.referenceRootResult.sha256)&&Number.isSafeInteger(receipt.referenceRootResult.bytes)&&receipt.referenceRootResult.bytes>0,'Root actual result external pin');
 need(receipt.matchedTransactions===65&&receipt.commands===66&&receipt.actualDockerExitCode===0&&receipt.completeResultFilesVerified===65&&receipt.allObservedFilesUnchanged===true&&receipt.transactionsReplayedByAdoption===0&&receipt.newReferenceExecution===false&&receipt.newNativeExecution===false&&receipt.consensusQualified===false&&receipt.productionQualified===false,'Root retained reference scope and actual terminal');
 return receipt;
}
try{
 need(process.argv.length===15,'CLI output modules caseRoot shim shimSHA nativeAccounting nativeSHA t8nOutputRoot t8nTrace traceSHA rootAdoption adoptionSHA rootResult');
 const [out,modules,caseRoot,shim,shimSHA,nativeFile,nativeSHA,t8nRoot,traceFile,traceSHA,adoptionFile,adoptionSHA,rootResultFile]=process.argv.slice(2);
 need([out,modules,caseRoot,shim,nativeFile,t8nRoot,traceFile,adoptionFile,rootResultFile].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&[shimSHA,nativeSHA,traceSHA,adoptionSHA].every(p=>/^[0-9a-f]{64}$/.test(p)),'explicit absolute roots/external SHA pins');
 const d=await hold(path.join(caseRoot,'tests/multihost/campaign-deadline.mjs')),c=await hold(path.join(caseRoot,'tests/multihost/campaign-commands.mjs'));
 const {createCampaignDeadline}=await checked(()=>fromBytes(d.data)),{createCampaignCommands}=await checked(()=>fromBytes(c.data));
 budget=createCampaignDeadline({startedAt:started,maximumRunMs:540000,closeoutReserveMs:60000,onExpire:()=>fail(Error('receipt coverage work deadline'))});commands=createCampaignCommands({deadline:budget,spawn});
 const ethers=await checked(()=>import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href));need(ethers.version==='6.16.0','existing ethers6.16.0');
 const nativePin=await hold(nativeFile,nativeSHA),tracePin=await hold(traceFile,traceSHA),adoptionPin=await hold(adoptionFile,adoptionSHA);
 const native=JSON.parse(nativePin.data),referenceTrace=JSON.parse(tracePin.data),adoption=rootAdoption(JSON.parse(adoptionPin.data),tracePin,nativePin);
 const rootPin=await hold(rootResultFile,adoption.referenceRootResult.sha256),rootResult=JSON.parse(rootPin.data);
 need(rootPin.bytes===adoption.referenceRootResult.bytes&&rootResult.format==='nano-root-inventory-v2-independent-t8n-v1'&&rootResult.passed===true&&rootResult.firstFailure===null&&Array.isArray(rootResult.secondaryFailures)&&rootResult.secondaryFailures.length===0&&rootResult.actualDockerExitCode===0&&rootResult.sourceCandidate===adoption.sourceCandidate&&rootResult.referenceVersion==='Geth1.16.7'&&/^[0-9a-f]{64}$/.test(rootResult.evmSha256)&&rootResult.knownInputFDsOpened>0&&rootResult.knownInputFDsOpened===rootResult.knownInputFDsClosed&&rootResult.compiledNow===false&&rootResult.nativeRerun===false&&rootResult.allocationNormalization===false&&rootResult.newSignatures===false&&rootResult.consensusQualified===false&&rootResult.productionQualified===false,'raw Root actual-PASS receipt must independently match adoption');
 need(native.format==='nano-inventory-v2-native-accounting-v1'&&native.executionKind===KIND&&native.mined===false&&native.consensusValidated===false&&native.productionQualified===false&&native.transactionCount===65&&Array.isArray(native.transactions)&&native.transactions.length===65,'retained original native raw65');
 need(Array.isArray(referenceTrace)&&referenceTrace.length===65,'retained full reference trace');
 const sender=ethers.computeAddress('0x'+'0'.repeat(63)+'1').toLowerCase(),inputs=[];let sumGas=0n,lastRoot=null;
 for(const [i,row]of native.transactions.entries()){
  check();need(hex(row.rawSignedTransaction)&&ROOT(row.transactionHash)&&ROOT(row.stateRootBefore)&&ROOT(row.executionStateRoot),'retained transaction bytes and state roots');const tx=ethers.Transaction.from(row.rawSignedTransaction),t=referenceTrace[i];
  need(tx.serialized===row.rawSignedTransaction&&tx.hash===row.transactionHash&&ethers.keccak256(row.rawSignedTransaction)===row.transactionHash&&tx.from.toLowerCase()===sender&&tx.type===0&&tx.chainId===31337n&&tx.nonce===i&&tx.gasLimit===30000000n&&tx.gasPrice===1n&&tx.value===uint(row.value),'exact original signed transaction');
  need(typeof row.success==='boolean'&&uint(row.gasUsed)>0n&&uint(row.gasUsed)<=30000000n&&uint(row.executionStep)===BigInt(i+1)&&uint(row.nonceBefore)===BigInt(i)&&uint(row.nonceAfter)===BigInt(i+1)&&uint(row.senderBalanceBefore)-uint(row.senderBalanceAfter)===uint(row.gasUsed)+(row.success?tx.value:0n)&&(!lastRoot||lastRoot===row.stateRootBefore),'retained wallet/nonce/root continuity');lastRoot=row.executionStateRoot;sumGas+=uint(row.gasUsed);
  const expectedChecks=['transactionHash','status','gasUsed','cumulativeGas','nonce','wallet','coinbase','stateRoot'];
  need(t.step===i+1&&t.transactionHash===row.transactionHash&&t.native.success===row.success&&uint(t.native.gasUsed)===uint(row.gasUsed)&&t.native.stateRoot===row.executionStateRoot&&t.reference.stateRoot===row.executionStateRoot&&uint(t.reference.gasUsed)===uint(row.gasUsed)&&uint(t.reference.status)===(row.success?1n:0n)&&t.checks&&same(Object.keys(t.checks).sort(),expectedChecks.sort())&&Object.values(t.checks).every(x=>x===true),'externally pinned strict reference step');
  need(t.result&&/^[0-9a-f]{64}$/.test(t.result.sha256)&&Number.isSafeInteger(t.result.bytes)&&t.result.bytes>0,'reference result pin');
  const resultPin=await hold(path.join(t8nRoot,'step-'+String(i+1).padStart(2,'0'),'result.json'),t.result.sha256);need(resultPin.bytes===t.result.bytes,'reference result exact bytes');
  const result=JSON.parse(resultPin.data);need((result.rejected?.length??0)===0&&Array.isArray(result.receipts)&&result.receipts.length===1&&result.stateRoot===row.executionStateRoot,'one accepted retained Geth transition');
  const receipt=result.receipts[0],r=receiptFromReference(receipt,ethers);
  need(receipt.transactionHash===row.transactionHash&&uint(receipt.status)===(row.success?1n:0n)&&uint(receipt.gasUsed)===uint(row.gasUsed)&&r.cumulative===uint(row.gasUsed)&&ROOT(result.receiptsRoot)&&result.receiptsRoot===r.singleton,'reference legacy receipt/root joins');
  if(Object.hasOwn(receipt,'type'))need(uint(receipt.type)===0n,'legacy reference receipt type');
  need(hex(result.logsBloom,256)&&result.logsBloom.toLowerCase()===r.bloom,'single-transaction aggregate bloom');
  inputs.push({row,tx,reference:r,resultPin:{sha256:resultPin.sha256,bytes:resultPin.bytes}});
 }
 need(lastRoot===native.actualStateRoot&&sumGas===uint(native.summedGasUsed)&&sumGas===uint(native.actualCoinbaseFees),'retained final accounting');
 await hold(shim,shimSHA,false);try{await fs.lstat(out);throw Error('output exists');}catch(e){if(e.code!=='ENOENT')throw e;}check();await fs.mkdir(out,{mode:0o700});output=out;
 await save('input-bindings.json',{nativeAccounting:{sha256:nativePin.sha256,bytes:nativePin.bytes},referenceTrace:{sha256:tracePin.sha256,bytes:tracePin.bytes},rootAdoption:{sha256:adoptionPin.sha256,bytes:adoptionPin.bytes},actualRootResult:{sha256:rootPin.sha256,bytes:rootPin.bytes},referenceSourceCandidate:adoption.sourceCandidate,referenceEvmSha256:rootResult.evmSha256,shimSha256:shimSHA,compiledNow:false,gethRerun:false,scenarioRerun:false,newTransactionsSignedByHelperMustEqualOriginal:true});
 child=spawn(shim,[],{detached:true,stdio:['pipe','pipe','pipe'],env:{PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'}});
 child.on('error',()=>fail(Error('owned helper spawn failed')));child.once('exit',(code,signal)=>{childExit={code,signal};if(!eofRequested)fail(Error('owned helper exited before requested EOF'));});child.once('close',(code,signal)=>{childClosed=true;childClose={code,signal};if(pending)fail(Error('owned helper closed with pending request'));});
 child.stdout.on('data',stdout);child.stdout.on('error',()=>fail(Error('owned stdout failed')));child.stdin.on('error',()=>fail(Error('owned stdin failed')));
 child.stderr.on('data',b=>{if(stderr.length+b.length>16384)fail(Error('owned stderr size'));else stderr=Buffer.concat([stderr,b]);});child.stderr.on('error',()=>fail(Error('owned stderr failed')));commands.ownPersistent(child);
 const initial=await request('metadata');need(initial.chainId==='0x7a69'&&initial.evmRevision==='Shanghai'&&initial.coinbase===COINBASE&&uint(initial.executionStep)===0n&&initial.executionStateRoot===native.transactions[0].stateRootBefore&&initial.gasPrice==='0x1'&&initial.baseFee==='0x0'&&uint(initial.transactionGasLimit)===30000000n&&uint(initial.contextGasLimit)===60000000n,'same original synthetic helper prestate/context');
 let gas=0n;
 for(const [i,{row,tx,reference:r,resultPin}]of inputs.entries()){
  const step=i+1,n=await request('send',{from:sender,to:tx.to,data:tx.data,value:quantity(tx.value),gas:quantity(tx.gasLimit)});
  const checks={
   rawSignedBytes:n.rawSignedTransaction===row.rawSignedTransaction,transactionHash:n.transactionHash===row.transactionHash,
   executionStep:uint(n.executionStep)===BigInt(step),preStateRoot:n.stateRootBefore===row.stateRootBefore,postStateRoot:n.executionStateRoot===row.executionStateRoot,
   status:n.success===row.success,gasUsed:uint(n.gasUsed)===uint(row.gasUsed),gasPrice:uint(n.effectiveGasPrice)===1n,
   nonce:uint(n.nonceBefore)===BigInt(i)&&uint(n.nonceAfter)===BigInt(step),wallet:uint(n.senderBalanceBefore)===uint(row.senderBalanceBefore)&&uint(n.senderBalanceAfter)===uint(row.senderBalanceAfter),
   receiptType:uint(n.receiptType)===0n,cumulativeGasUsed:uint(n.cumulativeGasUsed)===r.cumulative,
   logs:same(logs(n.logs),r.rows),logsBloom:hex(n.logsBloom,256)&&n.logsBloom.toLowerCase()===r.bloom,
   encodedLegacyReceipt:n.encodedReceipt===r.encoded,singletonReceiptsRoot:n.singletonReceiptsRoot===r.singleton,
   noMinedMetadata:!Object.hasOwn(n,'blockHash')&&!Object.hasOwn(n,'blockNumber')&&!Object.hasOwn(n,'transactionIndex')
  };
  gas+=uint(row.gasUsed);const coinbase=await request('account',{address:COINBASE});checks.coinbaseBalance=coinbase.executionStateRoot===n.executionStateRoot&&uint(coinbase.executionStep)===BigInt(step)&&uint(coinbase.balance)===gas;
  trace.push({step,transactionHash:row.transactionHash,checks,referenceResult:resultPin,native:{executionStateRoot:n.executionStateRoot,logs:n.logs,logsBloom:n.logsBloom,encodedReceipt:n.encodedReceipt,singletonReceiptsRoot:n.singletonReceiptsRoot},reference:{logs:r.rows,logsBloom:r.bloom,encodedReceipt:r.encoded,singletonReceiptsRoot:r.singleton}});
  need(Object.values(checks).every(v=>v===true),'strict receipt/raw/accounting mismatch at step '+step+': '+Object.entries(checks).filter(([,v])=>!v).map(([k])=>k).join(','));
 }
 const final=await request('root');need(final.executionStateRoot===native.actualStateRoot&&uint(final.executionStep)===65n&&trace.length===65&&requests===132&&gas===sumGas,'complete exact65 replay');
 check();eofRequested=true;child.stdin.end();await checked(()=>new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('owned EOF/close timeout')),budget.remaining(5000));const done=()=>{clearTimeout(timer);resolve();};if(childClosed)done();else child.once('close',done);}));
 need(!pending&&carry.length===0&&stderr.length===0&&childExit?.code===0&&childExit?.signal===null&&childClose?.code===0&&childClose?.signal===null,'clean owned exit/close and drained protocol');
 await commands.terminatePersistent(child,()=>childClosed,1000);groupJoined=childClosed;check();ready=!failed;
}catch(e){retain(e);}
finally{
 if(pending)fail(failure??Error('closeout with pending request'));if(budget)try{budget.enterCloseout();}catch(e){retain(e);}
 if(commands)try{await commands.closeAll();}catch(e){retain(e);}if(child&&commands)try{await commands.terminatePersistent(child,()=>childClosed,1000);groupJoined=childClosed;}catch(e){retain(e);}
 for(const item of held)try{need(item.identity&&same(nine(await item.h.stat({bigint:true})),item.identity)&&same(nine(await fs.lstat(item.file,{bigint:true})),item.identity),'final held FD/path full-nine fence');}catch(e){retain(e);}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){retain(e);}
 try{check();need(ready&&!failed&&groupJoined&&opened===closed&&trace.length===65&&requests===132,'all source/reference/owned terminal evidence');}catch(e){retain(e);ready=false;}
 if(output)try{await save('receipt-coverage-trace.json',trace);await save('receipt-coverage-protocol.json',protocol);await save('receipt-coverage-controller.json',{format:'nano-inventory-v2-receipt-coverage-controller-v1',passed:false,terminalEvidenceReady:ready&&!failed,externalTerminalExitRequired:true,transactions:trace.length,strictMatches:trace.filter(t=>Object.values(t.checks).every(Boolean)).length,firstMismatch:trace.find(t=>!Object.values(t.checks).every(Boolean))??null,requests,childExit,childClose,ownedChildGroupJoined:groupJoined,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,failure:failed?String(failure?.message??failure):null,scenarioRerun:false,gethRerun:false,compiledNow:false,mined:false,consensusValidated:false,productionQualified:false});check();}catch(e){retain(e);ready=false;}
 try{check();need(opened===closed,'publication descriptor closure');}catch(e){retain(e);ready=false;}budget?.dispose();process.removeListener('SIGINT',signal);process.removeListener('SIGTERM',signal);
 if(interrupted||performance.now()-started>=540000){retain(Error('final deadline/interruption'));ready=false;}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-receipt-coverage-terminal-v1',passed:false,terminalEvidenceReady:ready&&!failed,actualTerminalExitRequired:true,transactions:trace.length,requests,ownedChildGroupJoined:groupJoined,openedDescriptors:opened,closedDescriptors:closed,failure:failed?String(failure?.message??failure):null,productionQualified:false})+'\n');process.exitCode=ready&&!failed?0:1;
}
