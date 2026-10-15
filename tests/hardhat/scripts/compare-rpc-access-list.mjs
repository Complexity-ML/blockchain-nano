import {spawn,execFileSync} from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
// ROOT invokes this only with approved public binaries and a fresh output.
// Both children are loopback-only synthetic chains; no wallet or peer runs.
const binary=process.env.NANO_BINARY,anvil=process.env.ANVIL_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&anvil&&out&&[binary,anvil,out].every(path.isAbsolute),'absolute NANO_BINARY, ANVIL_BINARY and NANO_DIAGNOSTIC_REPORT_DIR required');
fs.mkdirSync(out,{recursive:false,mode:0o700});
const version=execFileSync(anvil,['--version'],{encoding:'utf8',timeout:10000,maxBuffer:16384}).trim();
if(process.env.ANVIL_EXPECTED_VERSION)assert(version.includes(process.env.ANVIL_EXPECTED_VERSION),'reference binary version mismatch');
const digest=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const addr=n=>'0x'+BigInt(n).toString(16).padStart(40,'0'),key=n=>'0x'+BigInt(n).toString(16).padStart(64,'0');
const sender=addr(0x1234),storage=addr(0xa1),writer=addr(0xa2),reverting=addr(0xa3),accounts=addr(0xa4),outer=addr(0xa5),branch=addr(0xa6),creation=addr(0xa7),refund=addr(0xa8),create2=addr(0xa9),coinbase=addr(0xaa);
const row=(address,keys=[])=>({address,storageKeys:keys.map(key)});
const code={
 [storage]:'0x600054506001545000',[writer]:'0x602a6000556000545000',[reverting]:'0x602a6000556007545060006000fd',
 [accounts]:'0x73'+addr(0xb1).slice(2)+'315073'+addr(0xb2).slice(2)+'3b5073'+addr(0xb3).slice(2)+'3f5000',
 [outer]:'0x6000600060006000600073'+reverting.slice(2)+'5af1506003545000',
 [branch]:'0x5a62012cc810600e5760005450005b6001545000',
 [creation]:'0x60096010600039600960006000f050006000545060006000f3',
 [create2]:'0x600960126000396000600960006000f550006000545060006000f3',
 [refund]:'0x600060005500',[coinbase]:'0x41315000'
};
const pairArtifact=process.env.UNISWAP_V2_PAIR_ARTIFACT;
assert(pairArtifact&&path.isAbsolute(pairArtifact),'absolute UNISWAP_V2_PAIR_ARTIFACT from existing @uniswap/v2-core 1.0.1 is required');
assert(fs.statSync(pairArtifact).size<=262144&&digest(pairArtifact)==='bb00cb1d64deea4159731b5957eeb8354531a110fb9120b9ef14edbc950e0736','public Uniswap artifact pin mismatch');
const pair=addr(0xe0),token0=addr(0xe1),token1=addr(0xe2),pairRuntime=JSON.parse(fs.readFileSync(pairArtifact,'utf8')).evm.deployedBytecode.object;
assert(/^[0-9a-fA-F]+$/.test(pairRuntime)&&pairRuntime.length%2===0);code[pair]='0x'+pairRuntime;
// Public synthetic token responses, not a production ERC20/economic fixture:
// balanceOf returns 2,000,000; transfer returns true. The AMM is the actual
// published Uniswap V2 Pair 1.0.1 bytecode, with explicit owned genesis state.
code[token0]=code[token1]='0x60003560e01c6370a0823114601957600160005260206000f35b621e848060005260206000f3';
const alloc={[sender]:{balance:'0xd3c21bcecceda1000000'}};for(const [a,c] of Object.entries(code))alloc[a]={code:c};alloc[refund].storage={[key(0)]:key(1)};
alloc[pair].storage={[key(0)]:key(100000),[key(6)]:key(0xe1),[key(7)]:key(0xe2),[key(8)]:key(1000000n+(1000000n<<112n)+(100n<<224n)),[key(12)]:key(1)};
const genesis={config:{chainId:1337,evmFork:'Shanghai'},timestamp:'0x64',alloc};
const call=(to,gas='0x186a0')=>({from:sender,to,gas,gasPrice:'0x77359400'});
const cases=[
 ['storage',call(storage)],['write',call(writer)],['refund',call(refund)],['account-opcodes',call(accounts)],
 ['revert',call(reverting)],['nested-revert',call(outer)],['monotone-gas-branch',call(branch)],
 ['create-child-storage',call(creation,'0x3d090')],['create2-child-storage',call(create2,'0x3d090')],
 ['top-level-create',{from:sender,gas:'0x3d090',gasPrice:'0x77359400',data:'0x6000545060006000f3'}],
 ['seed-dedup',{...call(storage),accessList:[row(addr(0xc2),[3,3]),row(storage,[1]),row(addr(0xc2),[2]),row(sender),row(addr(1))]}],
 ['excluded-seed-storage',{...call(storage),accessList:[row(sender,[8]),row(addr(1),[7])]}],
 ['warm-coinbase-account',call(coinbase)],
 ['amm-reserves',{...call(pair,'0xf4240'),data:'0x0902f1ac'}],
 ['amm-swap-revert',{...call(pair,'0xf4240'),data:'0x022c0d9f'+key(0).slice(2)+key(0).slice(2)+key(0x1234).slice(2)+key(128).slice(2)+key(0).slice(2)}],
 ['amm-swap-success',{...call(pair,'0xf4240'),data:'0x022c0d9f'+key(100).slice(2)+key(0).slice(2)+key(0x1234).slice(2)+key(128).slice(2)+key(0).slice(2)}]
];
async function rpc(url,method,params){const response=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(response.ok);const raw=await response.text();assert(raw.length<=1048576);return JSON.parse(raw);}
async function ok(url,method,params){const r=await rpc(url,method,params);assert(!r.error,JSON.stringify(r));return r.result;}
function canonical(list){assert(Array.isArray(list));const table=new Map();for(const r of list){assert(/^0x[0-9a-fA-F]{40}$/.test(r.address));const a=r.address.toLowerCase();if(!table.has(a))table.set(a,new Set());for(const k of r.storageKeys){assert(/^0x[0-9a-fA-F]{64}$/.test(k));table.get(a).add(k.toLowerCase());}}return [...table].sort(([a],[b])=>a.localeCompare(b)).map(([address,keys])=>({address,storageKeys:[...keys].sort()}));}
function normalized(r){assert(!r.error,'access-list transport/preparation error: '+JSON.stringify(r));const v=r.result;assert(v&&/^0x[0-9a-f]+$/.test(v.gasUsed));assert(!('error' in v)||typeof v.error==='string');const error=!v.error?null:/revert/i.test(v.error)?'revert':/out.of.gas/i.test(v.error)?'out-of-gas':'other-vm-error';return {accessList:canonical(v.accessList),gasUsed:BigInt(v.gasUsed).toString(),error};}
function start(exe,args,log){const child=spawn(exe,args,{env:{...process.env},stdio:['ignore','pipe','pipe']});const stream=fs.createWriteStream(log,{flags:'wx',mode:0o600});let text='',bytes=0;for(const fd of [child.stdout,child.stderr])fd.on('data',d=>{bytes+=d.length;if(bytes>8*1024*1024){child.kill('SIGKILL');return;}text+=d.toString();stream.write(d);});child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('child exited before readiness'));});child.stdout.on('data',()=>{const m=text.match(/listening on (?:http:\/\/)?127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(!child.pid||child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(resolve=>child.once('close',resolve));const kill=setTimeout(()=>child.kill('SIGKILL'),3000);let deadline;child.kill('SIGTERM');try{await Promise.race([done,new Promise((_,reject)=>{deadline=setTimeout(()=>reject(Error('child failed to close after SIGKILL')),5000);})]);}finally{clearTimeout(kill);clearTimeout(deadline);}}
const children=[],results=[];const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-access-list-oracle-'));let failure=null;
try{
 fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,'genesis.json'),JSON.stringify(genesis,null,2)+'\n');
 const nano=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1'],path.join(out,'nano.log'));children.push(nano.child);const nu=await nano.ready;
 const reference=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork','shanghai','--base-fee','1000000000','--gas-limit','30000000','--timestamp','100','--no-mining','--accounts','0'],path.join(out,'anvil.log'));children.push(reference.child);const au=await reference.ready;
 await ok(au,'anvil_setBalance',[sender,alloc[sender].balance]);for(const [a,c] of Object.entries(code))await ok(au,'anvil_setCode',[a,c]);for(const [a,account] of Object.entries(alloc))for(const [slot,value] of Object.entries(account.storage??{}))await ok(au,'anvil_setStorageAt',[a,slot,value]);
 const nh=await ok(nu,'eth_getBlockByNumber',['latest',false]),ah=await ok(au,'eth_getBlockByNumber',['latest',false]);assert.equal(BigInt(nh.timestamp),100n);assert.equal(BigInt(ah.timestamp),100n);assert.equal(nh.miner.toLowerCase(),ah.miner.toLowerCase(),'coinbase contexts must match before tracing warm coinbase');assert.equal(BigInt(nh.gasLimit),BigInt(ah.gasLimit));
 const snapshot=async url=>({height:await ok(url,'eth_blockNumber',[]),balance:await ok(url,'eth_getBalance',[sender,'latest']),nonce:await ok(url,'eth_getTransactionCount',[sender,'latest']),storage:await ok(url,'eth_getStorageAt',[writer,key(0),'latest']),revertingStorage:await ok(url,'eth_getStorageAt',[reverting,key(0),'latest']),refund:await ok(url,'eth_getStorageAt',[refund,key(0),'latest']),ammReserves:await ok(url,'eth_getStorageAt',[pair,key(8),'latest']),ammLock:await ok(url,'eth_getStorageAt',[pair,key(12),'latest'])});
 const beforeNano=await snapshot(nu),beforeAnvil=await snapshot(au);
 for(const [name,transaction] of cases){const [nr,ar]=await Promise.all([rpc(nu,'eth_createAccessList',[transaction,'latest']),rpc(au,'eth_createAccessList',[transaction,'latest'])]);const expectedError=name==='revert'||name==='amm-swap-revert'?'revert':null;let matches=false,n=null,a=null,reason=null;try{n=normalized(nr);a=normalized(ar);assert.equal(n.error,expectedError,'Nano VM disposition');assert.equal(a.error,expectedError,'reference VM disposition');assert.deepEqual(n,a);const repeated={...transaction,accessList:n.accessList};assert.deepEqual(normalized(await rpc(nu,'eth_createAccessList',[repeated,'latest'])),n);assert.deepEqual(normalized(await rpc(au,'eth_createAccessList',[repeated,'latest'])),a);matches=true;}catch(e){reason=e.message;}results.push({name,transaction,expectedError,nano:nr,reference:ar,normalizedNano:n,normalizedReference:a,matches,reason});}
 assert.deepEqual(await snapshot(nu),beforeNano);assert.deepEqual(await snapshot(au),beforeAnvil);assert.equal(results.length,16);assert(results.every(r=>r.matches),'access-list oracle mismatch; inspect results.json');
}catch(e){failure=e.message;}finally{const cleanup=await Promise.allSettled(children.reverse().map(stop));for(const item of cleanup)if(item.status==='rejected')failure??=item.reason.message;if(cleanup.every(item=>item.status==='fulfilled'))fs.rmSync(temp,{recursive:true,force:true});fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({format:'nano-public-shanghai-access-list-reference-comparison-v1',referenceVersion:version,referenceSha256:digest(anvil),nanoSha256:digest(binary),uniswapPairArtifactSha256:digest(pairArtifact),fork:'Shanghai',expectedComparisons:16,actualComparisons:results.length,matched:results.filter(r=>r.matches).length,failure,gasComparedExactlyAfterFinalListAndRefund:true,errorTextComparedExactly:false,privateInputsRead:false,transactionsBroadcast:0,operationalKeysUsed:false,productionQualified:false,results},null,2)+'\n');}
assert.equal(failure,null,failure??'');console.log('16/16 Shanghai access-list comparisons and repeated fixed points passed');
