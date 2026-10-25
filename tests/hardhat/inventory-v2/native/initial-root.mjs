// One EMPTY Geth t8n transition. No client, Native, compiler or transaction replay.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,why)=>{if(!ok)throw Error('Inventory initial root: '+why);};
const sha=b=>createHash('sha256').update(b).digest('hex'),json=v=>Buffer.from(JSON.stringify(v,null,2)+'\n');
const hash=s=>typeof s==='string'&&/^[0-9a-f]{64}$/.test(s),root=s=>typeof s==='string'&&/^0x[0-9a-f]{64}$/.test(s);
const exact=(v,keys,label)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join('\0')===[...keys].sort().join('\0'),label+' exact fields');
const quantity=s=>{need(typeof s==='string'&&/^(?:0x[0-9a-f]+|0|[1-9][0-9]*)$/i.test(s),'integer quantity');return BigInt(s);};
const canonicalPath=p=>need(typeof p==='string'&&path.isAbsolute(p)&&path.resolve(p)===p&&!/[\0\r\n]/.test(p),'absolute canonical path');
const artifact=(v,label)=>{exact(v,['path','sha256','bytes'],label);canonicalPath(v.path);need(hash(v.sha256)&&Number.isSafeInteger(v.bytes)&&v.bytes>0&&v.bytes<=134217728,'bounded external Artifact');return v;};
const EVM_SHA='7bad7839da2684bbc6eeab1c5b045e2f2bedd2e9132d8971ccd4f25adc058861';
const HELPERS={'campaign-deadline.mjs':'e23f6cbad812f4316f24bf94880308c72371769b5549b5e190d5276158f1ddd0','campaign-commands.mjs':'8c742afb61c2e7ead17721bc572a83ed1be95bc886f24af6ac4e21bbe16a5ec4'};
const fixtures=['0x7e5f4552091a69125d5dfcb7b8c2659029395bdf','0x2b5ad5c4795c026514f8317c7a215e218dccd6cf','0x6813eb9362372eef6200f3b1dbc3f819671cba69'];
function verifyUnchangedPostAllocation(alloc){
 need(alloc&&typeof alloc==='object'&&!Array.isArray(alloc),'post allocation object');const accounts=new Map();
 for(const [key,value]of Object.entries(alloc)){need(/^(?:0x)?[0-9a-f]{40}$/i.test(key),'post account address');const address='0x'+key.toLowerCase().replace(/^0x/,'');need(!accounts.has(address),'no post allocation aliases');accounts.set(address,value);}
 need(same([...accounts.keys()].sort(),[...fixtures].sort()),'exactly three unchanged post fixture accounts');
 for(const address of fixtures){const a=accounts.get(address);need(a&&typeof a==='object'&&!Array.isArray(a)&&Object.keys(a).every(k=>['balance','nonce','code','storage'].includes(k))&&quantity(a.balance)===10n**24n&&(a.nonce===undefined||quantity(a.nonce)===0n)&&(a.code===undefined||a.code==='0x')&&(a.storage===undefined||a.storage&&typeof a.storage==='object'&&!Array.isArray(a.storage)&&Object.keys(a.storage).length===0),'post fixture balance/nonce/code/storage unchanged');}
}
const expectedEnvironment=json({currentCoinbase:'0x0000000000000000000000000000000000009999',currentGasLimit:'0x3938700',currentNumber:'0x1',currentTimestamp:'0x6553f10c',currentRandom:'0x'+'0'.repeat(64),currentBaseFee:'0x0',withdrawals:[],blockHashes:{}});
const started=performance.now(),held=[],commandsObserved=[];let budget,commands,output,opened=0,closed=0,interrupted=false,failure=null,completed=false,ready=false,diagnostic=null;
const retain=e=>{failure??=e;},check=()=>{need(!interrupted&&performance.now()-started<120000,'deadline/interruption');budget?.check();};
const signal=()=>{interrupted=true;commands?.cancelAll(Error('Initial root interrupted'));};process.on('SIGINT',signal);process.on('SIGTERM',signal);
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s)),same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
async function hold(file,pin,retainBytes=true){
 check();canonicalPath(file);const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);++opened;const item={file,h};held.push(item);const before=await h.stat({bigint:true});check();
 need(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=134217728n&&(!retainBytes||before.size<=16777216n),'bounded regular input');const digest=createHash('sha256'),chunks=[];let count=0;
 for(;;){check();const buffer=Buffer.alloc(65536),r=await h.read(buffer,0,buffer.length,null);check();if(!r.bytesRead)break;count+=r.bytesRead;need(BigInt(count)<=before.size,'input length');digest.update(buffer.subarray(0,r.bytesRead));if(retainBytes)chunks.push(Buffer.from(buffer.subarray(0,r.bytesRead)));}
 const actual=digest.digest('hex');need(BigInt(count)===before.size&&(!pin||actual===pin.sha256&&(pin.bytes===undefined||count===pin.bytes))&&same(nine(before),nine(await h.stat({bigint:true})))&&same(nine(before),nine(await fs.lstat(file,{bigint:true}))),'external SHA/bytes/full-nine/path fence');item.identity=nine(before);return {sha256:actual,bytes:count,data:retainBytes?Buffer.concat(chunks,count):null};
}
async function write(leaf,bytes){
 check();need(output&&/^[a-z0-9-]+\.(?:json|rlp|log)$/.test(leaf)&&Buffer.isBuffer(bytes)&&bytes.length<=16777216,'bounded new output');const file=path.join(output,leaf),h=await fs.open(file,F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;
 try{check();await h.writeFile(bytes);check();await h.sync();check();}finally{await h.close();++closed;}return {path:file,sha256:sha(bytes),bytes:bytes.length};
}
const importBytes=b=>import('data:text/javascript;base64,'+b.toString('base64'));
try{
 need(process.argv.length===4,'CLI control.json controlSHA');const [file,pin]=process.argv.slice(2);canonicalPath(file);need(hash(pin),'external control SHA');const controlPin=await hold(file,{sha256:pin}),c=JSON.parse(controlPin.data);
 exact(c,['format','output','caseRoot','evm','nativeAccounting','referenceTrace','referenceBinding','referenceResult','sourceAlloc','sourceEnv'],'control');need(c.format==='nano-inventory-v2-initial-root-control-v1','control format');canonicalPath(c.output);canonicalPath(c.caseRoot);
 const roles=['evm','nativeAccounting','referenceTrace','referenceBinding','referenceResult','sourceAlloc','sourceEnv'];for(const role of roles)artifact(c[role],role);need(new Set([file,...roles.map(k=>c[k].path)]).size===roles.length+1,'distinct control/input paths');
 need(c.evm.sha256===EVM_SHA,'exact retained official Geth binary');need(c.output!==file&&!roles.some(k=>c[k].path===c.output||c[k].path.startsWith(c.output+'/')),'new output disjoint from retained inputs');
 const helper={};for(const [name,digest]of Object.entries(HELPERS))helper[name]=await hold(path.join(c.caseRoot,'tests/multihost',name),{sha256:digest});
 const {createCampaignDeadline}=await importBytes(helper['campaign-deadline.mjs'].data),{createCampaignCommands}=await importBytes(helper['campaign-commands.mjs'].data);check();
 budget=createCampaignDeadline({startedAt:started,maximumRunMs:120000,closeoutReserveMs:20000,onExpire:()=>commands?.cancelAll(Error('Initial root work deadline'))});
 commands=createCampaignCommands({deadline:budget,spawn:(program,args,options)=>{const child=spawn(program,args,options),row={exit:null,close:null};commandsObserved.push(row);child.once('exit',(code,signal)=>{row.exit={code,signal};});child.once('close',(code,signal)=>{row.close={code,signal};});return child;}});
 const inputs={};for(const role of roles)inputs[role]=await hold(c[role].path,c[role],role!=='evm');
 const native=JSON.parse(inputs.nativeAccounting.data),trace=JSON.parse(inputs.referenceTrace.data),binding=JSON.parse(inputs.referenceBinding.data),reference=JSON.parse(inputs.referenceResult.data);
 need(native.format==='nano-inventory-v2-native-accounting-v1'&&native.executionKind==='nano-in-process-shanghai-v1'&&native.mined===false&&native.consensusValidated===false&&native.productionQualified===false&&native.transactionCount===65&&Array.isArray(native.transactions)&&native.transactions.length===65,'retained original native accounting');
 const first=native.transactions[0];need(first&&root(first.stateRootBefore)&&first.executionStep==='1'&&first.nonceBefore==='0'&&first.mined===false,'original Native pre-root at nonce zero');
 need(Array.isArray(trace)&&trace.length===65&&trace[0].step===1,'retained t8n trace shape');exact(trace[0].checks,['transactionHash','status','gasUsed','cumulativeGas','nonce','wallet','coinbase','stateRoot'],'retained first-transition checks');
 need(trace[0].transactionHash===first.transactionHash&&trace[0].native.stateRoot===first.executionStateRoot&&trace[0].reference.stateRoot===first.executionStateRoot&&Object.values(trace[0].checks).every(v=>v===true),'retained t8n first transition binding');
 need(reference.format==='nano-root-inventory-v2-independent-t8n-v1'&&reference.passed===true&&reference.actualDockerExitCode===0&&reference.evmSha256===EVM_SHA&&reference.productionQualified===false,'retained reference result');
 need(binding.evmSha256===EVM_SHA&&binding.version==='evm version 1.16.7-stable-b9f3a3d9\n'&&binding.chainId===31337&&binding.fork==='Shanghai'&&binding.nativeAccounting.sha256===inputs.nativeAccounting.sha256&&binding.nativeAccounting.bytes===inputs.nativeAccounting.bytes,'same retained Geth/version/domain/Native input');
 need(trace[0].inputAlloc.sha256===inputs.sourceAlloc.sha256&&trace[0].inputAlloc.bytes===inputs.sourceAlloc.bytes,'EXACT original step01 input allocation');
 need(inputs.sourceEnv.data.equals(expectedEnvironment),'EXACT original step01 producer environment, not current UTC');
 const alloc=JSON.parse(inputs.sourceAlloc.data);need(same(Object.keys(alloc).sort(),[...fixtures].sort()),'only three original funded public fixture accounts');for(const address of fixtures){const a=alloc[address];need(a&&quantity(a.balance)===10n**24n&&quantity(a.nonce)===0n&&a.code==='0x'&&a.storage&&Object.keys(a.storage).length===0,'original fixture funding/code/nonce/storage');}
 try{await fs.lstat(c.output);throw Error('output already exists');}catch(e){if(e.code!=='ENOENT')throw e;}check();await fs.mkdir(c.output,{mode:0o700});output=c.output;check();
 const allocation=await write('alloc.json',inputs.sourceAlloc.data),environment=await write('env.json',inputs.sourceEnv.data),transactions=await write('empty.rlp',json('0xc0'));
 for(const v of [allocation,environment,transactions])await hold(v.path,v,false);
 const args=['t8n','--state.fork=Shanghai','--state.chainid=31337','--state.reward=-1','--input.alloc='+allocation.path,'--input.env='+environment.path,'--input.txs='+transactions.path,'--output.basedir='+output,'--output.alloc=post.json','--output.result=result.json','--output.body=body.rlp'];
 for(const item of held){check();need(item.identity&&same(nine(await item.h.stat({bigint:true})),item.identity)&&same(nine(await fs.lstat(item.file,{bigint:true})),item.identity),'pre-command held FD/path full-nine fence');check();}
 const command=await commands.exec(c.evm.path,args,{timeout:30000,maxBuffer:1048576,encoding:'utf8',env:{PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'}});check();
 await write('command.json',json({args,stdout:command.stdout,stderr:command.stderr,commandCompletedAndOwnedGroupJoined:true}));
 const resultPin=await hold(path.join(output,'result.json'),null),postPin=await hold(path.join(output,'post.json'),null),bodyPin=await hold(path.join(output,'body.rlp'),null,false),result=JSON.parse(resultPin.data);verifyUnchangedPostAllocation(JSON.parse(postPin.data));check();
 need(root(result.stateRoot)&&(result.rejected==null||Array.isArray(result.rejected)&&result.rejected.length===0)&&(result.receipts==null||Array.isArray(result.receipts)&&result.receipts.length===0),'empty reference transition has no rejection or receipt');
 if(Object.hasOwn(result,'gasUsed'))need(quantity(result.gasUsed)===0n,'optional total gas field must be zero for empty transition');
 need(result.stateRoot===first.stateRootBefore,'Geth initial root differs from retained Native pre-root');
 diagnostic={nativeInitialRoot:first.stateRootBefore,gethEmptyTransitionRoot:result.stateRoot,rootsEqual:true,postAllocationSemanticallyUnchanged:true,postFixtureAccounts:3,totalGasUsedFieldPresent:Object.hasOwn(result,'gasUsed'),totalGasUsed:result.gasUsed??null,inputAlloc:{sha256:allocation.sha256,bytes:allocation.bytes},inputEnv:{sha256:environment.sha256,bytes:environment.bytes},emptyTransactions:{sha256:transactions.sha256,bytes:transactions.bytes},referenceResult:{sha256:resultPin.sha256,bytes:resultPin.bytes},referencePostAlloc:{sha256:postPin.sha256,bytes:postPin.bytes},referenceBody:{sha256:bodyPin.sha256,bytes:bodyPin.bytes},referenceVersionRetained:binding.version,evmSha256:EVM_SHA};completed=true;
}catch(e){retain(e);}
finally{
 if(budget)try{budget.enterCloseout();}catch(e){retain(e);}if(commands)try{await commands.closeAll();}catch(e){retain(e);}
 for(const item of held)try{need(item.identity&&same(nine(await item.h.stat({bigint:true})),item.identity)&&same(nine(await fs.lstat(item.file,{bigint:true})),item.identity),'final held FD/path full-nine fence');}catch(e){retain(e);}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){retain(e);}
 try{check();need(completed&&!failure&&opened===closed&&commandsObserved.length===1&&commandsObserved.every(x=>x.exit?.code===0&&x.exit?.signal===null&&x.close?.code===0&&x.close?.signal===null),'one actual command/owned closure and closed descriptors');ready=true;}catch(e){retain(e);ready=false;}
 if(output)try{await write('initial-root-controller.json',json({format:'nano-inventory-v2-initial-root-controller-v1',passed:false,terminalEvidenceReady:ready&&!failure,externalTerminalExitRequired:true,emptyTransitionsRequested:1,signedTransactionsReplayed:0,original65ReplayRepeated:false,nativeExecuted:false,compiledNow:false,referenceContext:'standalone Geth evm t8n Shanghai',diagnostic,commandsObserved,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,failure:failure?String(failure.message??failure):null,allocationNormalization:false,genesisTimestampChanged:false,consensusValidated:false,productionQualified:false}));check();}catch(e){retain(e);ready=false;}
 try{check();need(opened===closed,'publication descriptors closed');}catch(e){retain(e);ready=false;}
 budget?.dispose();process.removeListener('SIGINT',signal);process.removeListener('SIGTERM',signal);if(interrupted||performance.now()-started>=120000){retain(Error('final deadline/interruption'));ready=false;}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-initial-root-terminal-v1',passed:false,terminalEvidenceReady:ready&&!failure,actualTerminalExitRequired:true,signedTransactionsReplayed:0,emptyTransitionsRequested:1,commandsObserved:commandsObserved.length,openedDescriptors:opened,closedDescriptors:closed,failure:failure?String(failure.message??failure):null,productionQualified:false})+'\n');process.exitCode=ready&&!failure?0:1;
}
