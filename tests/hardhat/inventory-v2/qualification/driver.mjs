// Private synthetic qualification only. No downloads, secrets, Docker or SSH.
// Root supplies RO dependencies/source and observes the real terminal exit.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawn} from 'node:child_process';
const need=(ok,why)=>{if(!ok)throw Error('Inventory qualification: '+why);};
const sha=b=>createHash('sha256').update(b).digest('hex');
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
 'token/ERC20/extensions/IERC20Permit.sol','token/ERC20/ERC20.sol','token/ERC20/extensions/IERC20Metadata.sol',
 'utils/Context.sol','interfaces/draft-IERC6093.sol'];
const start=performance.now(),held=[],transactions=[],commandsObserved=[];
let budget,commands,provider,child,childClosed=false,childExit=null,childClose=null,logFd,output;
let interrupted=false,failed=null,scenario=null,compiled=null,anvilJoined=false,opened=0,closed=0,ready=false,stopRequested=false;
const check=()=>{need(!interrupted&&performance.now()-start<540000,'deadline/interruption');budget?.check();};
const checked=async f=>{check();const value=await f();check();return value;};
const signal=()=>{interrupted=true;commands?.cancelAll(Error('Inventory interrupted'));};
process.on('SIGINT',signal);process.on('SIGTERM',signal);
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
async function hold(file,expected=null,retain=true){
 check();const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);++opened;const item={h};held.push(item);
 const a=await h.stat({bigint:true});check();need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=134217728n,'regular bounded source');
 need(!retain||a.size<=33554432n,'retained byte bound');const hash=createHash('sha256'),chunks=[];let count=0;
 while(true){check();const buffer=Buffer.alloc(65536),r=await h.read(buffer,0,buffer.length,null);check();if(!r.bytesRead)break;
  count+=r.bytesRead;need(BigInt(count)<=a.size,'size did not grow');hash.update(buffer.subarray(0,r.bytesRead));if(retain)chunks.push(Buffer.from(buffer.subarray(0,r.bytesRead)));}
 const digest=hash.digest('hex'),z=await h.stat({bigint:true});check();
 need(BigInt(count)===a.size&&JSON.stringify(nine(a))===JSON.stringify(nine(z))&&(!expected||digest===expected),'source SHA/full-nine fence');
 item.identity=nine(a);return {path:file,sha256:digest,bytes:count,data:retain?Buffer.concat(chunks,count):null};
}
async function save(name,data){
 check();need(output&&/^[a-z0-9-]+\.(json|mjs)$/.test(name),'owned output name');
 const bytes=Buffer.isBuffer(data)?data:Buffer.from(JSON.stringify(data,null,2)+'\n');need(bytes.length<=16777216,'bounded artifact');
 const h=await fs.open(path.join(output,name),F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;
 try{check();await h.writeFile(bytes);check();await h.sync();check();}finally{await h.close();++closed;}
 return {path:path.join(output,name),sha256:sha(bytes),bytes:bytes.length};
}
const importBytes=bytes=>import('data:text/javascript;base64,'+bytes.toString('base64'));
function artifactsFrom(result){
 need(result.format==='nano-inventory-v2-compiler-output-v1'&&result.compilationSucceeded===true&&/^0\.8\.28\+commit\./.test(result.compilerVersion),'successful solc0.8.28');
 need(result.compiled&&typeof result.compiled.contracts==='object'&&!(result.compiled.errors??[]).some(e=>e.severity==='error'),'no compiler error');
 const artifacts={},bounds=[];
 for(const [unit,rows]of Object.entries(result.compiled.contracts))for(const [name,c]of Object.entries(rows)){
  need(!Object.hasOwn(artifacts,name),'unique artifact names');const init=c.evm?.bytecode?.object??'',runtime=c.evm?.deployedBytecode?.object??'';
  need(/^(?:[a-fA-F0-9]{2})*$/.test(init)&&/^(?:[a-fA-F0-9]{2})*$/.test(runtime),'fully linked code');
  need(init.length/2<=49152&&runtime.length/2<=24576,'strict EIP3860/170 limit, no exception');
  need(Object.keys(c.evm?.bytecode?.linkReferences??{}).length===0&&Array.isArray(c.abi),'linked ABI');
  artifacts[name]={abi:c.abi,bytecode:'0x'+init};bounds.push({unit,name,initcodeBytes:init.length/2,runtimeBytes:runtime.length/2});
 }
 for(const name of ['InventoryV2Fixture','InventoryV2CasesA','InventoryV2CasesB','InventoryV2TermsCases','InventoryV2MultiClientCases','InventoryV2OpenBase','InventoryV2OpenNative','InventoryV2TokenMock'])need(artifacts[name]?.bytecode.length>2,'concrete artifact '+name);
 return {artifacts,bounds};
}
try{
 need(process.argv.length===7,'CLI outputRoot nodeModules caseRoot anvilPath anvilSHA');
 const [out,modules,root,anvilFile,anvilSha]=process.argv.slice(2);
 need([out,modules,root,anvilFile].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&/^[0-9a-f]{64}$/.test(anvilSha),'absolute explicit input paths');
 const deadlineSource=await hold(path.join(root,'tests/multihost/campaign-deadline.mjs'));
 const commandSource=await hold(path.join(root,'tests/multihost/campaign-commands.mjs'));
 const {createCampaignDeadline}=await checked(()=>importBytes(deadlineSource.data));
 const {createCampaignCommands}=await checked(()=>importBytes(commandSource.data));
 budget=createCampaignDeadline({startedAt:start,maximumRunMs:540000,closeoutReserveMs:60000,onExpire:()=>commands?.cancelAll(Error('Inventory deadline'))});check();
 commands=createCampaignCommands({deadline:budget,spawn:(program,args,options)=>{const p=spawn(program,args,options),r={exit:null,close:null};commandsObserved.push(r);
  p.once('exit',(code,signal)=>{r.exit={code,signal};});p.once('close',(code,signal)=>{r.close={code,signal};});return p;}});
 try{await fs.lstat(out);throw Error('Inventory qualification: output exists');}catch(e){if(e.code!=='ENOENT')throw e;}
 check();await fs.mkdir(out,{mode:0o700});output=out;check();
 const sources={};for(const [unit,digest]of units)sources[unit]={content:(await hold(path.join(root,unit),digest)).data.toString('utf8')};
 for(const unit of oz)sources['@openzeppelin/contracts/'+unit]={content:(await hold(path.join(modules,'@openzeppelin/contracts',unit))).data.toString('utf8')};
 const solcEntry=await hold(path.join(modules,'solc/index.js'));
 const workerSource=await hold(fileURLToPath(new URL('./compile-worker.mjs',import.meta.url)));
 const worker=await save('compile-worker.mjs',workerSource.data);
 const input={language:'Solidity',sources,settings:{evmVersion:'shanghai',viaIR:false,optimizer:{enabled:true,runs:200},
  outputSelection:{'*':{'*':['abi','evm.bytecode.object','evm.bytecode.linkReferences','evm.deployedBytecode.object','evm.deployedBytecode.immutableReferences']}}}};
 const job=await save('compiler-job.json',{format:'nano-inventory-v2-compiler-job-v1',solcEntry:{path:solcEntry.path,sha256:solcEntry.sha256,bytes:solcEntry.bytes},input});
 process.stdout.write(JSON.stringify({stage:'compile',profile:'shanghai',optimizerRuns:200})+'\n');
 const env={PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'};let raw;
 try{raw=(await commands.exec(process.execPath,[worker.path,job.path,job.sha256],{timeout:budget.remaining(180000),maxBuffer:8388608,encoding:'utf8',env})).stdout;check();}
 catch(e){if(typeof e.stdout==='string'&&e.stdout.length)await save('compiler-failure.json',JSON.parse(e.stdout));throw e;}
 compiled=JSON.parse(raw);need(compiled.inputSha256===sha(Buffer.from(JSON.stringify(input))),'actual compiler input binding');
 await save('compiler-output.json',compiled);const {artifacts,bounds}=artifactsFrom(compiled);await save('artifact-size-bounds.json',bounds);
 const scenarioSource=await hold(path.join(root,'tests/hardhat/inventory-v2/scenario.mjs'),'ef273dc8090d0789088a507065bad399b350f273cb4643eb3cfd62aec80e12c7');
 const backendSource=await hold(path.join(root,'tests/hardhat/inventory-v2/ethers-backend.mjs'),'385ae722cbeb5d1e2266d75ad88674f158df9cfab531b5ceedcd3bcb6b45597c');
 const modelSource=await hold(path.join(root,'tools/bridge-inventory-v2/inventory-model.mjs'),'8f248a6c57647a1df0d805b1f141e0765cc19efcfe6b8bc10cde36f5496eb8f4');
 const {runInventoryV2Scenario}=await checked(()=>importBytes(scenarioSource.data));
 const {createInventoryV2EthersBackend}=await checked(()=>importBytes(backendSource.data));
 const {createInventoryModel}=await checked(()=>importBytes(modelSource.data));
 const ethers=await checked(()=>import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href));need(ethers.version==='6.16.0','actual ethers6.16.0');
 await hold(anvilFile,anvilSha,false);check();
 logFd=await fs.open(path.join(output,'anvil.log'),F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);++opened;check();
 child=spawn(anvilFile,['--host','127.0.0.1','--port','18545','--chain-id','31337','--hardfork','shanghai','--gas-limit','60000000',
  '--accounts','1','--balance','10000','--mnemonic','test test test test test test test test test test test junk','--silent'],{detached:true,stdio:['ignore',logFd.fd,logFd.fd],env});
 child.on('error',e=>{failed??=Error('Owned Anvil spawn failed '+(e.code??'UNKNOWN'));});
 child.once('exit',(code,signal)=>{childExit={code,signal};if(!stopRequested)failed??=Error('Owned Anvil exited before requested closeout');});child.once('close',(code,signal)=>{childClosed=true;childClose={code,signal};});commands.ownPersistent(child);
 const req=new ethers.FetchRequest('http://127.0.0.1:18545/');req.timeout=3000;
 provider=new ethers.JsonRpcProvider(req,undefined,{batchMaxCount:1,cacheTimeout:-1});provider.pollingInterval=100;
 const rpc=async(method,params=[])=>await checked(()=>provider.send(method,params));
 const until=Math.min(budget.workEnd,performance.now()+15000);let healthy=false;
 while(!healthy){check();need(!failed&&!childClosed,'owned Anvil alive');try{const v=await rpc('web3_clientVersion');need(typeof v==='string'&&/^anvil\//i.test(v),'Anvil client version');healthy=true;}
  catch(e){check();if(performance.now()>=until)throw e;await checked(()=>new Promise(r=>setTimeout(r,100)));}}
 need(BigInt(await rpc('eth_chainId'))===31337n&&BigInt(await rpc('eth_blockNumber'))===0n,'fresh synthetic chain');
 const accounts=await rpc('eth_accounts'),sender='0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266';
 need(Array.isArray(accounts)&&accounts.length===1&&accounts[0].toLowerCase()===sender,'public Anvil fixture account only');
 need(BigInt(await rpc('eth_getTransactionCount',[sender,'latest']))===0n,'fresh fixture nonce');
 const signer=await checked(()=>provider.getSigner(0));
 const base=await createInventoryV2EthersBackend({provider,signer,ethers,artifacts,approvedSyntheticChainId:31337n,gasLimit:30000000n,check});
 const wallet=async()=>({balance:BigInt(await rpc('eth_getBalance',[sender,'latest'])),nonce:BigInt(await rpc('eth_getTransactionCount',[sender,'latest'])),block:BigInt(await rpc('eth_blockNumber'))});
 async function transaction(f){check();need(transactions.length<80,'at most eighty actual transactions');const before=await wallet(),result=await checked(f),r=result.receipt??result;
  const receipt=await rpc('eth_getTransactionReceipt',[r.transactionHash]),tx=await rpc('eth_getTransactionByHash',[r.transactionHash]);
  const block=await rpc('eth_getBlockByHash',[r.blockHash,false]),after=await wallet();
  need(receipt&&tx&&block&&BigInt(receipt.status)===(r.success?1n:0n)&&BigInt(receipt.gasUsed)===BigInt(r.gasUsed)&&BigInt(receipt.effectiveGasPrice)===BigInt(r.effectiveGasPrice),'real mined receipt, exact status/gas');
  const gas=BigInt(r.gasUsed)*BigInt(r.effectiveGasPrice),value=BigInt(tx.value);
  need(before.balance-after.balance===gas+value&&after.nonce===before.nonce+1n&&BigInt(tx.nonce)===before.nonce,'exact observed wallet debit and nonce');
  need(after.block===before.block+1n&&BigInt(tx.blockNumber)===after.block&&BigInt(block.number)===after.block,'one actual automined block');
  need(block.hash.toLowerCase()===r.blockHash.toLowerCase()&&receipt.blockHash.toLowerCase()===r.blockHash.toLowerCase()&&block.transactions.length===1&&block.transactions[0].toLowerCase()===r.transactionHash.toLowerCase(),'actual committed transaction');
  need(tx.hash.toLowerCase()===r.transactionHash.toLowerCase()&&tx.from.toLowerCase()===sender&&r.from===sender,'actual fixture sender');
  transactions.push({transactionHash:r.transactionHash,gasCost:gas.toString(),value:value.toString(),walletDebitVerified:true});return result;
 }
 const backend={syntheticPrivateOnly:true,deploy:(...args)=>transaction(()=>base.deploy(...args)),send:(...args)=>transaction(()=>base.send(...args)),read:base.read,code:base.code,balance:base.balance};
 process.stdout.write(JSON.stringify({stage:'scenario',assertions:24,actualEOAGasDebits:true})+'\n');
 scenario=await checked(()=>runInventoryV2Scenario({backend,artifacts,ethers,createInventoryModel}));
 need(scenario.scenarioCompleted===true&&scenario.cases.length===24&&scenario.expectedRefusals===3&&scenario.pairedChecks>=20,'complete scenario and trace');
 for(let i=0;i<15;i++)need(scenario.cases.filter(x=>x.caseId===i&&x.passed===true).length===1,'one actual economic case');
 for(let i=0;i<8;i++)need(scenario.cases.filter(x=>x.termsCaseId===i&&x.passed===true).length===1,'one actual terms case');
 need(scenario.cases.filter(x=>x.multiClientCaseId===0&&x.passed&&x.twoDistinctBaseRecipients&&x.twoDistinctNativeFunders).length===1,'actual two-client case');
 need(scenario.receipts.length===transactions.length&&transactions.every((t,i)=>t.transactionHash===scenario.receipts[i].transactionHash),'all receipts joined to wallet debit');
 need(transactions.reduce((n,t)=>n+BigInt(t.gasCost),0n).toString()===scenario.receiptCalculatedOuterGasCost,'exact total receipt gas');
 for(const key of ['sourceFinalityAuthenticated','economicTermsAdopted','realAssetsUsed','productionQualified'])need(scenario[key]===false,'no promotion '+key);
 await save('scenario-result.json',scenario);await save('wallet-debit-checks.json',transactions);
}catch(e){failed??=e;}
finally{
 if(budget)try{budget.enterCloseout();}catch(e){failed??=e;}
 if(commands)try{await commands.closeAll();}catch(e){failed??=e;}
 if(provider)try{provider.destroy();}catch(e){failed??=e;}
 if(child&&commands)try{if(childClosed||child.exitCode!==null||child.signalCode!==null)failed??=Error('Owned Anvil was already stopped before requested closeout');stopRequested=true;await commands.terminatePersistent(child,()=>childClosed,1000);anvilJoined=childClosed;need(anvilJoined,'owned Anvil close and group joined');}catch(e){failed??=e;}
 if(logFd){try{await logFd.sync();}catch(e){failed??=e;}finally{try{await logFd.close();++closed;}catch(e){failed??=e;}}}
 for(const item of held)try{const a=await item.h.stat({bigint:true});need(item.identity&&JSON.stringify(item.identity)===JSON.stringify(nine(a)),'held source final fence');}catch(e){failed??=e;}
 for(const item of held.reverse())try{await item.h.close();++closed;}catch(e){failed??=e;}
 try{check();need(opened===closed&&anvilJoined&&scenario?.scenarioCompleted===true,'complete closure and scenario');
  need(commandsObserved.length===1&&commandsObserved[0].exit?.code===0&&commandsObserved[0].exit?.signal===null&&commandsObserved[0].close?.code===0&&commandsObserved[0].close?.signal===null,'compiler actual exit/close zero');ready=!failed;
 }catch(e){failed??=e;}
 if(output)try{await save('controller-result.json',{format:'nano-inventory-v2-terminal-evidence-v1',passed:false,terminalEvidenceReady:ready,externalTerminalExitRequired:true,
  assertionTransactions:scenario?.cases?.length??0,transactionCount:transactions.length,openedDescriptorsBeforePublication:opened,closedDescriptorsBeforePublication:closed,
  compilerCommand:commandsObserved,compilerVersion:compiled?.compilerVersion??null,anvilExit:childExit,anvilClose:childClose,ownedAnvilGroupJoined:anvilJoined,
  elapsedMs:Math.ceil(performance.now()-start),failure:failed?String(failed.message??'failure'):null,
  dependencyClosureAuthority:'Parent-supplied read-only pinned 91-package tree; no automatic requalification by this driver',
  sourceFinalityAuthenticated:false,economicTermsAdopted:false,realAssetsUsed:false,productionQualified:false});check();}catch(e){failed??=e;ready=false;}
 try{check();need(opened===closed,'publication FDs closed');}catch(e){failed??=e;ready=false;}
 budget?.dispose();process.removeListener('SIGINT',signal);process.removeListener('SIGTERM',signal);
 if(interrupted||performance.now()-start>=540000){ready=false;failed??=Error('Final deadline/interruption');}
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-terminal-summary-v1',passed:false,terminalEvidenceReady:ready&&!failed,actualTerminalExitRequired:true,
  assertionTransactions:scenario?.cases?.length??0,transactionCount:transactions.length,openedDescriptors:opened,closedDescriptors:closed,ownedAnvilGroupJoined:anvilJoined,
  failure:failed?String(failed.message??'failure'):null,sourceFinalityAuthenticated:false,productionQualified:false})+'\n');
 process.exitCode=ready&&!failed?0:1;
}
