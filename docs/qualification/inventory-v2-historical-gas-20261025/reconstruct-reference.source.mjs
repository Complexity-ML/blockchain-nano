// Offline retrospective only: no EVM, RPC, child process or transaction submission; known public Anvil fixture signing only.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const E='/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261024';
const R='/Users/boris/Dev/blockchain-nano';
const OUT=path.join(path.dirname(new URL(import.meta.url).pathname),'result-public.json');
const sha=b=>createHash('sha256').update(b).digest('hex'),need=(b,m)=>{if(!b)throw Error(m);};
const started=performance.now(),held=[];let result,failed=null,closed=0;
const check=()=>need(performance.now()-started<30000,'bounded offline computation');
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
async function read(file,pin){
 check();const fd=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);const heldRow={fd,file};held.push(heldRow);const a=await fd.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=16777216n,'bounded regular source input');const b=await fd.readFile();check();heldRow.material=nine(a);const digest=sha(b);
 need(BigInt(b.length)===a.size&&(!pin||digest===pin)&&JSON.stringify(heldRow.material)===JSON.stringify(nine(await fd.stat({bigint:true})))&&JSON.stringify(heldRow.material)===JSON.stringify(nine(await fs.lstat(file,{bigint:true}))),'source SHA/full9/path fence');return {data:b,pin:{path:file,bytes:b.length,sha256:digest}};
}
try{
 const ethers=await import(pathToFileURL(R+'/tests/hardhat/node_modules/ethers/lib.esm/index.js').href);need(ethers.version==='6.16.0','existing ethers version');
 const nativeFile=await read(E+'/inventory-v2-native-runtime-root-01/retained-public/outputs--native-run--native-accounting.json','6f92730dd60640c8f02a6808bd30118cf2345f2e542229d706215687e4e250fc');
 const referenceFile=await read(R+'/docs/qualification/closed-inventory-v2-20261025/reference-scenario-result.json','db11cfb4282df66db32c7a680ddca0dbefeaaab2904696baf0ff428613e6e8b5');
 const walletFile=await read(R+'/docs/qualification/closed-inventory-v2-20261025/reference-wallet-debit-checks.json','1f70e2d78b4763ab4fc6fd5619c75f44ca8bed40beca5384494a48a1a389a61d');
 const sourceFiles=[];for(const file of ['tests/contracts/InventoryV2Harness.sol','contracts/bridge-v2/API.sol','contracts/bridge-v2/Endpoint.sol'])sourceFiles.push((await read(R+'/'+file)).pin);
 const native=JSON.parse(nativeFile.data),reference=JSON.parse(referenceFile.data),walletRows=JSON.parse(walletFile.data);
 need(native.transactions?.length===65&&reference.receipts?.length===65&&walletRows.length===65&&reference.scenarioCompleted===true,'original complete paired cohorts');
 const wallet=ethers.HDNodeWallet.fromPhrase('test test test test test test test test test test test junk'),oldSender=wallet.address.toLowerCase();need(reference.receipts.every(r=>r.from===oldSender),'explicit original public Anvil fixture only');
 const txs=native.transactions.map((row,i)=>{const tx=ethers.Transaction.from(row.rawSignedTransaction);need(tx.type===0&&tx.nonce===i&&tx.chainId===31337n&&tx.gasLimit===30000000n&&tx.gasPrice===1n&&tx.hash===row.transactionHash,'pinned original native envelope');return tx;});
 const sender=txs[0].from.toLowerCase(),words=new Map(),addressPairs=[];const word=a=>'0x'+a.slice(2).toLowerCase().padStart(64,'0');
 const map=(a,b)=>{need(!words.has(a)||words.get(a)===b,'unique deterministic word mapping');words.set(a,b);};
 const addressPair=(a,b)=>{a=a.toLowerCase();b=b.toLowerCase();map(word(a),word(b));addressPairs.push({native:a,reference:b});};
 addressPair(sender,oldSender);let pairedNative,pairedReference;
 for(const [i,tx]of txs.entries())if(tx.to===null){
  need(reference.receipts[i].label.startsWith('deploy '),'original reference creation label');const a=ethers.getCreateAddress({from:sender,nonce:i}).toLowerCase(),b=ethers.getCreateAddress({from:oldSender,nonce:i}).toLowerCase();addressPair(a,b);
  if(reference.receipts[i].label==='deploy InventoryV2Fixture'){
   for(let nonce=1;nonce<=5;++nonce)addressPair(ethers.getCreateAddress({from:a,nonce}),ethers.getCreateAddress({from:b,nonce}));
   pairedNative=a;pairedReference=b;
  }
 }
 need(Object.keys(reference.finalFinancial.baseBalances).length===1&&Object.hasOwn(reference.finalFinancial.baseBalances,pairedReference),'historical final paired fixture matches reconstructed CREATE address');
 const abi=ethers.AbiCoder.defaultAbiCoder(),termsTypes=['uint256','bytes32','address','address','uint256','bytes32','address','address','uint256','uint256','uint256','bytes32','bytes32','uint256'];
 const termsHash=ethers.id('NanoInventoryTermsV2(uint256 baseChainId,bytes32 baseDomain,address baseEndpoint,address baseToken,uint256 nativeChainId,bytes32 nativeDomain,address nativeEndpoint,address nativeAsset,uint256 totalBaseSupply,uint256 baseLot,uint256 nativeLot,bytes32 baseToNativeVerifierDomain,bytes32 nativeToBaseVerifierDomain,uint256 maxProofBytes)');
 const routeHash=ethers.id('NanoInventoryRouteV2(uint256 baseChainId,bytes32 baseDomain,address baseEndpoint,address baseToken,uint256 nativeChainId,bytes32 nativeDomain,address nativeEndpoint,address nativeAsset)');
 const terms=f=>[31337n,ethers.id('synthetic-base-v2'),ethers.getCreateAddress({from:f,nonce:4}),ethers.getCreateAddress({from:f,nonce:1}),31337n,ethers.id('synthetic-native-v2'),ethers.getCreateAddress({from:f,nonce:5}),ethers.ZeroAddress,1000n,2n,3n,ethers.id('synthetic-base-proof-v2'),ethers.id('synthetic-native-proof-v2'),1024n];
 const nt=terms(pairedNative),rt=terms(pairedReference),nh=ethers.keccak256(abi.encode(['bytes32',...termsTypes],[termsHash,...nt])),rh=ethers.keccak256(abi.encode(['bytes32',...termsTypes],[termsHash,...rt]));
 const nr=ethers.keccak256(abi.encode(['bytes32',...termsTypes.slice(0,8)],[routeHash,...nt.slice(0,8)])),rr=ethers.keccak256(abi.encode(['bytes32',...termsTypes.slice(0,8)],[routeHash,...rt.slice(0,8)]));map(nh,rh);map(nr,rr);
 for(let nonce=1;nonce<=65;++nonce)for(const kind of [0,2,4]){
  const types=['bytes32','bytes32','address','uint256','uint8'];map(ethers.keccak256(abi.encode(types,[nr,nh,nt[2],nonce,kind])),ethers.keccak256(abi.encode(types,[rr,rh,rt[2],nonce,kind])));
 }
 const transformed=(data,to)=>{
  if(to===null)return data;need((data.length-10)%64===0,'closed fixture fixed-word calldata');let out=data.slice(0,10);for(let at=10;at<data.length;at+=64){const w='0x'+data.slice(at,at+64);out+=(words.get(w)??w).slice(2);}return out;
 };
 const addressMap=new Map(addressPairs.map(r=>[r.native,r.reference])),rows=[];let observed=0n,explained=0n;
 const intrinsic=(data,creation)=>{const bytes=ethers.getBytes(data);let zeros=0;for(const b of bytes)if(b===0)++zeros;return {bytes:bytes.length,zeros,nonzero:bytes.length-zeros,gas:21000n+(creation?32000n+2n*BigInt(Math.ceil(bytes.length/32)):0n)+4n*BigInt(zeros)+16n*BigInt(bytes.length-zeros)};};
 for(const [i,tx]of txs.entries()){
  check();const row=native.transactions[i],ref=reference.receipts[i],fee=BigInt(ref.effectiveGasPrice),to=tx.to===null?null:addressMap.get(tx.to.toLowerCase());need(tx.to===null||to,'mapped original deployment target');
  const data=transformed(tx.data,tx.to),raw=await wallet.signTransaction({type:2,chainId:31337,nonce:i,gasLimit:30000000,to,data,value:tx.value,maxFeePerGas:fee+1000000000n,maxPriorityFeePerGas:0n,accessList:[]});check();const hash=ethers.keccak256(raw),a=intrinsic(tx.data,tx.to===null),b=intrinsic(data,to===null),nativeGas=BigInt(row.gasUsed),referenceGas=BigInt(ref.gasUsed),delta=referenceGas-nativeGas,byteDelta=b.gas-a.gas;
  need(walletRows[i].transactionHash===ref.transactionHash&&walletRows[i].walletDebitVerified===true&&BigInt(walletRows[i].value)===tx.value&&BigInt(walletRows[i].gasCost)===referenceGas*fee,'original wallet and value bindings');
  const checks={originalReferenceHashRecovered:hash===ref.transactionHash,statusIdentical:row.success===ref.success,calldataLengthIdentical:a.bytes===b.bytes,gasDeltaEqualsIntrinsic:delta===byteDelta,executionGasIdentical:referenceGas-b.gas===nativeGas-a.gas,nonnegativeExecutionGas:referenceGas>=b.gas&&nativeGas>=a.gas};
  observed+=delta;explained+=byteDelta;rows.push({step:i+1,label:ref.label,checks,referenceHash:ref.transactionHash,recoveredReferenceRaw:raw,nativeHash:row.transactionHash,referenceGas:referenceGas.toString(),nativeGas:nativeGas.toString(),observedGasDelta:delta.toString(),intrinsicGasDelta:byteDelta.toString(),nativeCalldata:{...a,gas:a.gas.toString()},referenceCalldata:{...b,gas:b.gas.toString()},nativeExecutionGas:(nativeGas-a.gas).toString(),referenceExecutionGas:(referenceGas-b.gas).toString()});
 }
 check();const matches=rows.filter(r=>Object.values(r.checks).every(Boolean));
 result={format:'nano-inventory-v2-historical-gas-reconstruction-v1',passed:matches.length===65&&observed===324n&&explained===observed,sourceInputs:[nativeFile.pin,referenceFile.pin,walletFile.pin,...sourceFiles],ethersVersion:ethers.version,nodeVersion:process.version,matchedTransactions:matches.length,transactions:rows.length,observedGasDelta:observed.toString(),intrinsicGasDelta:explained.toString(),sumNonzeroByteDelta:rows.reduce((n,r)=>n+r.referenceCalldata.nonzero-r.nativeCalldata.nonzero,0),zeroNoncePublicFixtureAddress:oldSender,addressPairs,rows,evmExecuted:false,transactionsSent:false,originalRunsRepeated:false,newKeysGenerated:false,activatedForkQualified:false,consensusQualified:false,productionQualified:false};
}catch(e){failed=String(e.message??e);}
finally{
 for(const h of held)try{need(h.material&&JSON.stringify(h.material)===JSON.stringify(nine(await h.fd.stat({bigint:true})))&&JSON.stringify(h.material)===JSON.stringify(nine(await fs.lstat(h.file,{bigint:true}))),'final source full9/path fence');}catch(e){failed??=String(e.message??e);}
 try{check();}catch(e){failed??=String(e.message??e);}
 for(const h of held)try{await h.fd.close();++closed;}catch(e){failed??=String(e.message??e);}
 try{check();}catch(e){failed??=String(e.message??e);}
 const report={...(result??{format:'nano-inventory-v2-historical-gas-reconstruction-v1',passed:false}),elapsedMs:Math.ceil(performance.now()-started),sourceFDsOpened:held.length,sourceFDsClosed:closed,failure:failed};if(failed)report.passed=false;
 const ready=report.passed;report.terminalEvidenceReady=ready;report.passed=false;report.actualTerminalExitRequired=true;
 await fs.writeFile(OUT,JSON.stringify(report,null,2)+'\n',{flag:'wx',mode:0o600});check();console.log(JSON.stringify({passed:false,terminalEvidenceReady:ready,actualTerminalExitRequired:true,matchedTransactions:report.matchedTransactions,observedGasDelta:report.observedGasDelta,intrinsicGasDelta:report.intrinsicGasDelta,firstMismatch:report.rows?.find(r=>!Object.values(r.checks).every(Boolean))?.step??null,sourceFDsOpened:held.length,sourceFDsClosed:closed,failure:failed}));process.exitCode=ready?0:1;
}
