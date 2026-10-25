// Standalone Geth evm t8n reference; exact retained raw transactions only.
// Reuses the RLP-list file contract in tests/qualification/compare-geth-sequence.
// No node/RPC/Anvil, compiler, Native replay, Wallet, key or new signature.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,m)=>{if(!ok)throw Error('Inventory t8n reference: '+m);};
const sha=b=>createHash('sha256').update(b).digest('hex'),lower=s=>String(s).toLowerCase(),q=n=>'0x'+n.toString(16);
const root=s=>typeof s==='string'&&/^0x[0-9a-f]{64}$/.test(s),uint=s=>{need(typeof s==='string'&&/^(?:0x[0-9a-f]+|0|[1-9][0-9]*)$/i.test(s),'integer quantity');return BigInt(s);};
const COINBASE='0x0000000000000000000000000000000000009999',started=performance.now(),held=[],trace=[],commandsObserved=[];
let budget,commands,output,opened=0,closed=0,interrupted=false,hasFailure=false,failure,completed=false,ready=false,inputPin;
const retain=e=>{if(!hasFailure){hasFailure=true;failure=e;}},check=()=>{need(!interrupted&&performance.now()-started<540000,'deadline/interruption');budget?.check();};
const checked=async f=>{check();const r=await f();check();return r;},signal=()=>{interrupted=true;commands?.cancelAll(Error('t8n reference interrupted'));};
process.on('SIGINT',signal);process.on('SIGTERM',signal);
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s)),same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
async function hold(file,expected=null,retainBytes=true){
 check();const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);++opened;const item={file,h};held.push(item);const a=await h.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=134217728n&&(!retainBytes||a.size<=16777216n),'bounded regular file');
 const hash=createHash('sha256'),chunks=[];let count=0;for(;;){check();const b=Buffer.alloc(65536),r=await h.read(b,0,b.length,null);check();if(!r.bytesRead)break;count+=r.bytesRead;need(BigInt(count)<=a.size,'file size');hash.update(b.subarray(0,r.bytesRead));if(retainBytes)chunks.push(Buffer.from(b.subarray(0,r.bytesRead)));}
 const digest=hash.digest('hex');need(BigInt(count)===a.size&&(!expected||digest===expected)&&same(nine(a),nine(await h.stat({bigint:true})))&&same(nine(a),nine(await fs.lstat(file,{bigint:true}))),'file SHA/full-nine/path fence');item.identity=nine(a);return {sha256:digest,bytes:count,data:retainBytes?Buffer.concat(chunks,count):null};
}
async function write(file,value){
 check();const b=Buffer.isBuffer(value)?value:Buffer.from(JSON.stringify(value,null,2)+'\n');need(b.length<=16777216,'output size');const h=await fs.open(file,F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;
 try{check();await h.writeFile(b);check();await h.sync();check();}finally{await h.close();++closed;}return {sha256:sha(b),bytes:b.length};
}
const importBytes=b=>import('data:text/javascript;base64,'+b.toString('base64'));
const account=(alloc,where)=>{const keys=Object.keys(alloc).filter(k=>lower(k).replace(/^0x/,'')===where.slice(2));need(keys.length<=1,'unaliased alloc address');return keys.length?alloc[keys[0]]:null;};
try{
 need(process.argv.length===9,'CLI newOutput modules caseRoot evmPath evmSHA nativeAccounting nativeAccountingSHA');const [out,modules,caseRoot,evm,evmSha,nativeFile,nativeSha]=process.argv.slice(2);
 need([out,modules,caseRoot,evm,nativeFile].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&[evmSha,nativeSha].every(s=>/^[0-9a-f]{64}$/.test(s)),'absolute explicit paths and external pins');
 const d=await hold(path.join(caseRoot,'tests/multihost/campaign-deadline.mjs')),c=await hold(path.join(caseRoot,'tests/multihost/campaign-commands.mjs'));
 const {createCampaignDeadline}=await checked(()=>importBytes(d.data)),{createCampaignCommands}=await checked(()=>importBytes(c.data));budget=createCampaignDeadline({startedAt:started,maximumRunMs:540000,closeoutReserveMs:60000,onExpire:()=>commands?.cancelAll(Error('t8n budget'))});
 commands=createCampaignCommands({deadline:budget,spawn:(file,args,options)=>{const child=spawn(file,args,options),r={exit:null,close:null};commandsObserved.push(r);child.once('exit',(code,signal)=>{r.exit={code,signal};});child.once('close',(code,signal)=>{r.close={code,signal};});return child;}});
 const ethers=await checked(()=>import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href));need(ethers.version==='6.16.0','existing ethers6.16.0');inputPin=await hold(nativeFile,nativeSha);const native=JSON.parse(inputPin.data);
 need(native.format==='nano-inventory-v2-native-accounting-v1'&&native.executionKind==='nano-in-process-shanghai-v1'&&native.mined===false&&native.consensusValidated===false&&native.productionQualified===false&&native.transactionCount===65&&native.walletDebitChecks===65&&native.coinbaseFeeChecks===65&&Array.isArray(native.transactions)&&native.transactions.length===65,'retained actual native accounting');
 const fixtures=[1,2,3].map(n=>lower(ethers.computeAddress('0x'+BigInt(n).toString(16).padStart(64,'0')))),sender=fixtures[0],parsed=[];let prior=null,expectedGas=0n;
 for(const [i,row]of native.transactions.entries()){
  check();need(typeof row.rawSignedTransaction==='string'&&/^0x(?:[0-9a-f]{2})+$/.test(row.rawSignedTransaction)&&root(row.transactionHash)&&root(row.stateRootBefore)&&root(row.executionStateRoot),'retained raw/hash/roots');const tx=ethers.Transaction.from(row.rawSignedTransaction);
  need(tx.serialized===row.rawSignedTransaction&&ethers.keccak256(row.rawSignedTransaction)===row.transactionHash&&tx.hash===row.transactionHash&&lower(tx.from)===sender&&tx.type===0&&tx.chainId===31337n&&tx.nonce===i&&tx.gasLimit===30000000n&&tx.gasPrice===1n&&tx.value===uint(row.value),'exact raw signature/domain/nonce/gas/value');
  need(typeof row.success==='boolean'&&row.mined===false&&row.walletDebitVerified===true&&row.coinbaseFeeVerified===true&&uint(row.effectiveGasPrice)===1n&&uint(row.nonceBefore)===BigInt(i)&&uint(row.nonceAfter)===BigInt(i+1)&&uint(row.executionStep)===BigInt(i+1),'native nonce/fee/step evidence');
  const used=uint(row.gasUsed);need(used>0n&&used<=30000000n&&uint(row.senderBalanceBefore)-uint(row.senderBalanceAfter)===used+(row.success?tx.value:0n)&&(!prior||prior===row.stateRootBefore),'native wallet/root continuity');prior=row.executionStateRoot;expectedGas+=used;
  const decoded=ethers.decodeRlp(row.rawSignedTransaction);need(Array.isArray(decoded)&&ethers.encodeRlp(decoded)===row.rawSignedTransaction,'legacy signed RLP preserved');parsed.push({row,tx,rlp:ethers.encodeRlp([decoded])});
 }
 need(prior===native.actualStateRoot&&expectedGas===uint(native.summedGasUsed)&&expectedGas===uint(native.actualCoinbaseFees),'native final root/gas joins');await hold(evm,evmSha,false);
 try{await fs.lstat(out);throw Error('output exists');}catch(e){if(e.code!=='ENOENT')throw e;}await fs.mkdir(out,{mode:0o700});output=out;
 const env={PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'},version=(await commands.exec(evm,['--version'],{timeout:budget.remaining(10000),maxBuffer:1048576,encoding:'utf8',env})).stdout;check();need(/^evm version /im.test(version),'official standalone Geth evm entry');
 await write(path.join(out,'input-binding.json'),{nativeAccounting:{sha256:inputPin.sha256,bytes:inputPin.bytes},evmSha256:evmSha,version,formatOrigin:'tests/qualification/compare-geth-sequence.mjs:37-46',chainId:31337,fork:'Shanghai',newSignatures:false,nativeRerun:false,compiledNow:false});
 let allocBytes=Buffer.from(JSON.stringify(Object.fromEntries(fixtures.map(a=>[a,{balance:q(10n**24n),nonce:'0x0',code:'0x',storage:{}}])),null,2)+'\n'),gas=0n;
 for(const [i,{row,rlp}]of parsed.entries()){
  check();const step=i+1,dir=path.join(out,'step-'+String(step).padStart(2,'0'));await fs.mkdir(dir,{mode:0o700});check();const pre=JSON.parse(allocBytes),before=account(pre,sender);need(before&&uint(before.balance)===uint(row.senderBalanceBefore)&&uint(before.nonce??'0x0')===BigInt(i),'reference own prestate nonce/balance');
  const allocation=await write(path.join(dir,'alloc.json'),allocBytes),environment={currentCoinbase:COINBASE,currentGasLimit:q(60000000n),currentNumber:q(BigInt(step)),currentTimestamp:q(1700000000n+12n*BigInt(step)),currentRandom:'0x'+'0'.repeat(64),currentBaseFee:'0x0',withdrawals:[],blockHashes:{}};
  const environmentPin=await write(path.join(dir,'env.json'),environment),transactionPin=await write(path.join(dir,'txs.rlp'),rlp);
  await hold(path.join(dir,'alloc.json'),allocation.sha256,false);await hold(path.join(dir,'env.json'),environmentPin.sha256,false);await hold(path.join(dir,'txs.rlp'),transactionPin.sha256,false);
  const args=['t8n','--state.fork=Shanghai','--state.chainid=31337','--state.reward=-1','--input.alloc='+path.join(dir,'alloc.json'),'--input.env='+path.join(dir,'env.json'),'--input.txs='+path.join(dir,'txs.rlp'),'--output.basedir='+dir,'--output.alloc=post.json','--output.result=result.json','--output.body=body.rlp'];
  const command=await commands.exec(evm,args,{timeout:budget.remaining(30000),maxBuffer:1048576,encoding:'utf8',env});check();await write(path.join(dir,'command.json'),{args,stdout:command.stdout,stderr:command.stderr,commandCompletedAndOwnedGroupJoined:true});
  const resultPin=await hold(path.join(dir,'result.json')),postPin=await hold(path.join(dir,'post.json')),bodyPin=await hold(path.join(dir,'body.rlp'),null,false),result=JSON.parse(resultPin.data),post=JSON.parse(postPin.data);
  need((result.rejected?.length??0)===0&&Array.isArray(result.receipts)&&result.receipts.length===1&&root(result.stateRoot),'one accepted reference transition/receipt');const receipt=result.receipts[0],after=account(post,sender),fees=account(post,COINBASE);need(after&&fees,'reference sender/coinbase poststate');
  const checks={transactionHash:receipt.transactionHash===row.transactionHash,status:uint(receipt.status)===(row.success?1n:0n),gasUsed:uint(receipt.gasUsed)===uint(row.gasUsed),cumulativeGas:uint(receipt.cumulativeGasUsed)===uint(row.gasUsed),nonce:uint(after.nonce??'0x0')===BigInt(step),wallet:uint(after.balance)===uint(row.senderBalanceAfter),coinbase:uint(fees.balance)===gas+uint(row.gasUsed),stateRoot:result.stateRoot===row.executionStateRoot};
  trace.push({step,transactionHash:row.transactionHash,native:{success:row.success,gasUsed:row.gasUsed,stateRoot:row.executionStateRoot},reference:{status:receipt.status,gasUsed:receipt.gasUsed,stateRoot:result.stateRoot},checks,inputAlloc:allocation,outputAlloc:{sha256:postPin.sha256,bytes:postPin.bytes},result:{sha256:resultPin.sha256,bytes:resultPin.bytes},body:{sha256:bodyPin.sha256,bytes:bodyPin.bytes}});
  need(Object.values(checks).every(v=>v===true),'strict transition mismatch at step '+step+': '+Object.entries(checks).filter(([,ok])=>!ok).map(([k])=>k).join(','));
  gas+=uint(receipt.gasUsed);allocBytes=postPin.data; // EXACT own reference output, never Native/Anvil refreshed or normalized.
 }
 need(trace.length===65&&gas===expectedGas,'all65 strict independent transitions and gas');completed=true;
}catch(e){retain(e);}
finally{
 if(budget)try{budget.enterCloseout();}catch(e){retain(e);}if(commands)try{await commands.closeAll();}catch(e){retain(e);}
 for(const item of held)try{need(item.identity&&same(nine(await item.h.stat({bigint:true})),item.identity)&&same(nine(await fs.lstat(item.file,{bigint:true})),item.identity),'final held input/output FD/path fence');}catch(e){retain(e);}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){retain(e);}
 try{check();need(completed&&!hasFailure&&opened===closed&&commandsObserved.length===66&&commandsObserved.every(c=>c.exit?.code===0&&c.exit?.signal===null&&c.close?.code===0&&c.close?.signal===null),'all actual commands/owned closures and descriptors');ready=true;}catch(e){retain(e);ready=false;}
 if(output)try{await write(path.join(output,'t8n-reference-trace.json'),trace);await write(path.join(output,'t8n-reference-controller.json'),{format:'nano-inventory-v2-independent-t8n-reference-v1',passed:false,terminalEvidenceReady:ready&&!hasFailure,externalTerminalExitRequired:true,comparedTransactions:trace.length,matchedTransactions:trace.filter(t=>Object.values(t.checks).every(Boolean)).length,firstMismatch:trace.find(t=>!Object.values(t.checks).every(Boolean))??null,commandsObserved,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,failure:hasFailure?String(failure?.message??failure):null,nativeRerun:false,anvilUsed:false,allocationNormalization:false,newSignatures:false,compiledNow:false,mined:false,consensusValidated:false,productionQualified:false});check();}catch(e){retain(e);ready=false;}
 try{check();need(opened===closed,'publication descriptors closed');}catch(e){retain(e);ready=false;}budget?.dispose();process.removeListener('SIGINT',signal);process.removeListener('SIGTERM',signal);if(interrupted||performance.now()-started>=540000){retain(Error('final deadline/interruption'));ready=false;}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-independent-t8n-terminal-v1',passed:false,terminalEvidenceReady:ready&&!hasFailure,actualTerminalExitRequired:true,comparedTransactions:trace.length,openedDescriptors:opened,closedDescriptors:closed,failure:hasFailure?String(failure?.message??failure):null,productionQualified:false})+'\n');process.exitCode=ready&&!hasFailure?0:1;
}
