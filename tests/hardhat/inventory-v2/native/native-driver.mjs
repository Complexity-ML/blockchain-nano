// One owned in-process C++ test helper, no RPC/Anvil/download/build/key file.
// Parent authenticates Core/link provenance and observes actual terminal exit.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,why)=>{if(!ok)throw Error('Native V2 qualification: '+why);};
const sha=b=>createHash('sha256').update(b).digest('hex');
const KIND='nano-in-process-shanghai-v1';
const units=[
 ['contracts/bridge-v2/API.sol','0a1b758856984dc64ad13328aab6eb70d8a7f939d6cd673fdf322952245e911d'],
 ['contracts/bridge-v2/Endpoint.sol','d1dcdbf349282700b080a083fa51d61c6f7016e63391c8ee1a2661310e35dcce'],
 ['contracts/bridge-v2/BaseInventory.sol','495d3b1399b47f01d2560119e2d1ad8b002df106a07f1e041ec192657c6db32b'],
 ['contracts/bridge-v2/NativeReserve.sol','c335c1a85724a6e9b5d2750f1e6a8edb4aa3de7debc626652cb0cb4ce9618563'],
 ['tests/contracts/InventoryV2Harness.sol','76b916a75827965f1095f5646617ee6ab4481a9166126ad958255c49b87cbebc'],
 ['tests/contracts/InventoryV2Cases.sol','a70f85609ed50c49a22baa94f7cc3348a8616e94efb002b4cf965993950906c1'],
 ['tests/contracts/InventoryV2TermsCases.sol','022c35e104a61321c1d16f47750e363b25cb4f00c55e0b81108f7e37fc523610'],
 ['tests/contracts/InventoryV2MultiClientCases.sol','295e089e74a6cfcdfe874873e484c29f24b708693d114074894b0b0885a54350']];
const oz=['utils/ReentrancyGuard.sol','token/ERC20/IERC20.sol','token/ERC20/utils/SafeERC20.sol','utils/Address.sol',
 'token/ERC20/extensions/IERC20Permit.sol','token/ERC20/ERC20.sol','token/ERC20/extensions/IERC20Metadata.sol','utils/Context.sol','interfaces/draft-IERC6093.sol'];
const started=performance.now(),held=[],transport=[];let budget,commands,child,childClosed=false,childExit=null,childClose=null,stderr=Buffer.alloc(0),carry=Buffer.alloc(0),pending=null;
let opened=0,closed=0,requests=0,stdoutBytes=0,eofRequested=false,groupJoined=false,interrupted=false,failed=null,ready=false,output,scenario,accounting;
const check=()=>{need(!interrupted&&performance.now()-started<540000,'budget/interruption');budget?.check();};
function rejectPending(error){if(!failed)failed=error;if(pending){const p=pending;pending=null;clearTimeout(p.timer);p.reject(error);}}
const onSignal=()=>{interrupted=true;rejectPending(Error('Native V2 interrupted'));commands?.cancelAll(Error('Native V2 interrupted'));};
process.on('SIGINT',onSignal);process.on('SIGTERM',onSignal);
const checked=async f=>{check();const r=await f();check();return r;};
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
async function hold(file,expected=null,retain=true){
 check();const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);++opened;const item={h};held.push(item);
 const a=await h.stat({bigint:true});check();need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=134217728n,'regular bounded pinned input');need(!retain||a.size<=33554432n,'retained byte cap');
 const hash=createHash('sha256'),parts=[];let count=0;
 while(true){check();const buffer=Buffer.alloc(65536),r=await h.read(buffer,0,buffer.length,null);check();if(!r.bytesRead)break;count+=r.bytesRead;need(BigInt(count)<=a.size,'input length fence');hash.update(buffer.subarray(0,r.bytesRead));if(retain)parts.push(Buffer.from(buffer.subarray(0,r.bytesRead)));}
 const digest=hash.digest('hex'),z=await h.stat({bigint:true});check();need(BigInt(count)===a.size&&(!expected||digest===expected)&&JSON.stringify(nine(a))===JSON.stringify(nine(z)),'input SHA/full-nine fence');
 item.identity=nine(a);return {sha256:digest,bytes:count,data:retain?Buffer.concat(parts,count):null};
}
async function save(name,value){
 check();need(output&&/^[a-z0-9-]+\.json$/.test(name),'owned result leaf');const bytes=Buffer.from(JSON.stringify(value,null,2)+'\n');need(bytes.length<=16777216,'result byte cap');
 const h=await fs.open(path.join(output,name),F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;
 try{check();await h.writeFile(bytes);check();await h.sync();check();}finally{await h.close();++closed;}
}
const fromBytes=bytes=>import('data:text/javascript;base64,'+bytes.toString('base64'));
function artifactsFrom(r){
 need(r.format==='nano-inventory-v2-compiler-output-v1'&&r.compilationSucceeded===true&&/^0\.8\.28\+commit\./.test(r.compilerVersion)&&r.compiled&&typeof r.compiled.contracts==='object','qualified compiler output');
 need(!(r.compiled.errors??[]).some(e=>e.severity==='error'),'no compiler error');const artifacts={},bounds=[];
 for(const [unit,rows]of Object.entries(r.compiled.contracts))for(const [name,c]of Object.entries(rows)){
  const init=c.evm?.bytecode?.object??'',run=c.evm?.deployedBytecode?.object??'';
  need(!Object.hasOwn(artifacts,name)&&/^(?:[a-fA-F0-9]{2})*$/.test(init)&&/^(?:[a-fA-F0-9]{2})*$/.test(run),'unique fully linked artifact');
  need(init.length/2<=49152&&run.length/2<=24576&&Object.keys(c.evm?.bytecode?.linkReferences??{}).length===0&&Array.isArray(c.abi),'strict code bounds and ABI');
  artifacts[name]={abi:c.abi,bytecode:'0x'+init};bounds.push({unit,name,initcodeBytes:init.length/2,runtimeBytes:run.length/2});
 }
 for(const name of ['InventoryV2CasesA','InventoryV2CasesB','InventoryV2Fixture','InventoryV2TermsCases','InventoryV2MultiClientCases','InventoryV2OpenBase','InventoryV2OpenNative','InventoryV2TokenMock'])need(artifacts[name]?.bytecode.length>2,'required concrete artifact');
 return {artifacts,bounds};
}
function request(action,fields={}){
 check();need(child&&!childClosed&&!pending&&!failed&&!eofRequested,'one live owned request');need(requests<5000,'request count cap');
 const id=++requests,bytes=Buffer.from(JSON.stringify({id,chain:0,action,...fields})+'\n');need(bytes.length<=2097152,'request line cap');
 return new Promise((resolve,reject)=>{
  const timer=setTimeout(()=>rejectPending(Error('Native request timeout')),budget.remaining(30000));pending={id,action,resolve,reject,timer};
  try{check();child.stdin.write(bytes,error=>{if(error)rejectPending(Error('Native stdin write failed'));});}catch(e){rejectPending(e);}
 });
}
function stdout(chunk){
 try{check();stdoutBytes+=chunk.length;need(stdoutBytes<=33554432,'stdout total cap');carry=Buffer.concat([carry,chunk]);
  while(true){const end=carry.indexOf(10);if(end<0){need(carry.length<=2097152,'response line cap');break;}
   need(end<=2097152,'response line cap');const line=carry.subarray(0,end);carry=Buffer.from(carry.subarray(end+1));
   const r=JSON.parse(line.toString('utf8')),p=pending;need(p&&r.id===p.id&&typeof r.ok==='boolean','one matched protocol response');
   if(!r.ok){pending=null;clearTimeout(p.timer);const e=Error('Native protocol/admission refusal: '+String(r.error).slice(0,240));failed??=e;p.reject(e);continue;}
   need(r.result&&typeof r.result==='object','typed actual result');transport.push({id:p.id,action:p.action,ok:true,executionKind:r.result.executionKind,
    executionStateRoot:r.result.executionStateRoot,executionStep:r.result.executionStep,success:r.result.success});pending=null;clearTimeout(p.timer);p.resolve(r.result);
  }
 }catch(e){rejectPending(e);}
}
try{
 need(process.argv.length===10,'CLI output modules caseRoot shim shimSHA compilerOutput compilerOutputSHA compilerInputSHA');
 const [out,modules,root,shim,shimSHA,compiledFile,compiledSHA,inputSHA]=process.argv.slice(2);
 need([out,modules,root,shim,compiledFile].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&[shimSHA,compiledSHA,inputSHA].every(s=>/^[0-9a-f]{64}$/.test(s)),'explicit absolute paths/external SHA pins');
 const d=await hold(path.join(root,'tests/multihost/campaign-deadline.mjs')),p=await hold(path.join(root,'tests/multihost/campaign-commands.mjs'));
 const {createCampaignDeadline}=await checked(()=>fromBytes(d.data)),{createCampaignCommands}=await checked(()=>fromBytes(p.data));
 budget=createCampaignDeadline({startedAt:started,maximumRunMs:540000,closeoutReserveMs:60000,onExpire:()=>rejectPending(Error('Native work budget expired'))});check();
 commands=createCampaignCommands({deadline:budget,spawn});
 try{await fs.lstat(out);throw Error('Native output exists');}catch(e){if(e.code!=='ENOENT')throw e;}
 check();await fs.mkdir(out,{mode:0o700});output=out;check();
 const sources={};for(const [unit,pin]of units)sources[unit]={content:(await hold(path.join(root,unit),pin)).data.toString('utf8')};
 for(const unit of oz)sources['@openzeppelin/contracts/'+unit]={content:(await hold(path.join(modules,'@openzeppelin/contracts',unit))).data.toString('utf8')};
 const input={language:'Solidity',sources,settings:{evmVersion:'shanghai',viaIR:false,optimizer:{enabled:true,runs:200},
  outputSelection:{'*':{'*':['abi','evm.bytecode.object','evm.bytecode.linkReferences','evm.deployedBytecode.object','evm.deployedBytecode.immutableReferences']}}}};
 need(sha(Buffer.from(JSON.stringify(input)))===inputSHA,'exact retained GEN03 Standard JSON input/settings');
 const compiled=JSON.parse((await hold(compiledFile,compiledSHA)).data);need(compiled.inputSha256===inputSHA,'externally retained compiler/input binding');
 const {artifacts,bounds}=artifactsFrom(compiled);await save('artifact-reuse.json',{compilerOutputSha256:compiledSHA,compilerInputSha256:inputSHA,compilerVersion:compiled.compilerVersion,compiledNow:false,bounds});
 const here=path.dirname(fileURLToPath(import.meta.url));
 const sc=await hold(path.join(here,'scenario-native.mjs'),'9aa2a869339120f745cfe95e86f375745d54567d9dbf6eebe6e18e11546ba3de');
 const bc=await hold(path.join(here,'native-backend.mjs'),'b93db6f5a262092c8db94dafa3b3f358e8397a5e66dd34ace6e56878af675009');
 const mc=await hold(path.join(root,'tools/bridge-inventory-v2/inventory-model.mjs'),'8f248a6c57647a1df0d805b1f141e0765cc19efcfe6b8bc10cde36f5496eb8f4');
 const {runInventoryV2Scenario}=await checked(()=>fromBytes(sc.data)),{createInventoryV2NativeBackend}=await checked(()=>fromBytes(bc.data)),{createInventoryModel}=await checked(()=>fromBytes(mc.data));
 const ethers=await checked(()=>import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href));need(ethers.version==='6.16.0','existing ethers6.16.0');
 await hold(shim,shimSHA,false);check();
 child=spawn(shim,[],{detached:true,stdio:['pipe','pipe','pipe'],env:{PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'}});
 child.on('error',()=>rejectPending(Error('Owned native helper spawn failed')));
 child.once('exit',(code,signal)=>{childExit={code,signal};if(!eofRequested)rejectPending(Error('Native helper exited before EOF'));});
 child.once('close',(code,signal)=>{childClosed=true;childClose={code,signal};if(pending)rejectPending(Error('Native helper closed with pending request'));});
 child.stdout.on('data',stdout);child.stdout.on('error',()=>rejectPending(Error('Native stdout failed')));child.stdin.on('error',()=>rejectPending(Error('Native stdin failed')));
 child.stderr.on('data',chunk=>{if(stderr.length+chunk.length>16384)rejectPending(Error('Native stderr cap'));else stderr=Buffer.concat([stderr,chunk]);});
 child.stderr.on('error',()=>rejectPending(Error('Native stderr failed')));commands.ownPersistent(child);
 const backend=await createInventoryV2NativeBackend({request,ethers,artifacts,check});
 scenario=await checked(()=>runInventoryV2Scenario({backend,artifacts,ethers,createInventoryModel,evidenceMode:KIND}));
 accounting=await checked(()=>backend.finish());
 need(scenario.scenarioCompleted===true&&scenario.executionKind===KIND&&scenario.mined===false&&scenario.cases.length===24&&scenario.pairedChecks===22&&scenario.expectedRefusals===3,'complete exact native scenario');
 for(let i=0;i<15;i++)need(scenario.cases.filter(c=>c.caseId===i&&c.passed===true).length===1,'original economic case');
 for(let i=0;i<8;i++)need(scenario.cases.filter(c=>c.termsCaseId===i&&c.passed===true).length===1,'original terms case');
 need(scenario.cases.filter(c=>c.multiClientCaseId===0&&c.passed&&c.twoDistinctBaseRecipients&&c.twoDistinctNativeFunders).length===1,'actual two-client case');
 need(accounting.transactionCount===65&&accounting.walletDebitChecks===65&&accounting.coinbaseFeeChecks===65&&scenario.receipts.length===65,'all sixty-five actual native transactions and fees');
 need(accounting.transactions.every((t,i)=>t.walletDebitVerified&&t.coinbaseFeeVerified&&t.transactionHash===scenario.receipts[i].transactionHash&&t.executionStateRoot===scenario.receipts[i].executionStateRoot),'actual native receipt/root joins');
 need(accounting.summedGasUsed===scenario.gasUsed&&accounting.actualCoinbaseFees===scenario.receiptCalculatedOuterGasCost,'all actual gas charged at price one');
 need(scenario.receipts.filter(r=>r.success===false).length===3&&scenario.receipts.every(r=>!Object.hasOwn(r,'blockHash')),'actual failures, no mined evidence');
 await save('native-scenario-result.json',scenario);await save('native-accounting.json',accounting);await save('native-transport.json',transport);
 check();eofRequested=true;child.stdin.end();
 await checked(()=>new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Native EOF/close timeout')),budget.remaining(5000));
  const done=()=>{clearTimeout(timer);resolve();};if(childClosed)done();else child.once('close',done);}));
 need(carry.length===0&&!pending&&childExit?.code===0&&childExit?.signal===null&&childClose?.code===0&&childClose?.signal===null&&stderr.length===0,'actual clean EOF exit/close, drained protocol/stderr');
 await commands.terminatePersistent(child,()=>childClosed,1000);groupJoined=childClosed;check();ready=!failed;
}catch(e){failed??=e;}
finally{
 if(pending)rejectPending(failed??Error('Native closeout with pending request'));
 if(budget)try{budget.enterCloseout();}catch(e){failed??=e;}
 if(commands)try{await commands.closeAll();}catch(e){failed??=e;}
 if(child&&commands)try{await commands.terminatePersistent(child,()=>childClosed,1000);groupJoined=childClosed;}catch(e){failed??=e;}
 for(const item of held)try{const a=await item.h.stat({bigint:true});need(item.identity&&JSON.stringify(item.identity)===JSON.stringify(nine(a)),'held input final full-nine fence');}catch(e){failed??=e;}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){failed??=e;}
 try{check();need(ready&&!failed&&groupJoined&&opened===closed,'complete native terminal/fences');}catch(e){failed??=e;ready=false;}
 if(output)try{await save('native-controller-result.json',{format:'nano-inventory-v2-native-controller-evidence-v1',passed:false,terminalEvidenceReady:ready&&!failed,externalTerminalExitRequired:true,
  executionKind:KIND,mined:false,consensusValidated:false,assertions:scenario?.cases?.length??0,pairedChecks:scenario?.pairedChecks??0,transactions:accounting?.transactionCount??0,
  requests,stdoutBytes,stderrBytes:stderr.length,childExit,childClose,ownedChildGroupJoined:groupJoined,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,
  elapsedMs:Math.ceil(performance.now()-started),failure:failed?String(failed.message??'failure'):null,
  coreAndLinkProvenanceVerifiedByDriver:false,activatedNetworkForkQualified:false,sourceFinalityAuthenticated:false,economicTermsAdopted:false,realAssetsUsed:false,productionQualified:false});check();}catch(e){failed??=e;ready=false;}
 try{check();need(opened===closed,'publication descriptor closure');}catch(e){failed??=e;ready=false;}
 budget?.dispose();process.removeListener('SIGINT',onSignal);process.removeListener('SIGTERM',onSignal);
 if(interrupted||performance.now()-started>=540000){ready=false;failed??=Error('Native final deadline/interruption');}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-native-terminal-summary-v1',passed:false,terminalEvidenceReady:ready&&!failed,actualTerminalExitRequired:true,
  executionKind:KIND,mined:false,assertions:scenario?.cases?.length??0,pairedChecks:scenario?.pairedChecks??0,transactions:accounting?.transactionCount??0,
  requests,ownedChildGroupJoined:groupJoined,openedDescriptors:opened,closedDescriptors:closed,failure:failed?String(failed.message??'failure'):null,productionQualified:false})+'\n');
 process.exitCode=ready&&!failed?0:1;
}
