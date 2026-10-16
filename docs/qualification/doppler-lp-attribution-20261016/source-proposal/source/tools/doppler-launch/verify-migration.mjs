import {createPublicClient,http,parseAbi,decodeEventLog,getAddress,zeroAddress,encodeFunctionData,decodeFunctionResult} from 'viem';
import {airlockAbi,getAddresses} from '@whetstone-research/doppler-sdk/evm';
import {readFile,writeFile} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
// Historical Base V2 module used by the private fork; do not infer Split semantics.
const V2_MIGRATOR='0x5F3bA43D44375286296Cb85F1EA2EBfa25dde731';
const TRANSFER_TOPIC='0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef';
const lpTransferAbi=parseAbi(['event Transfer(address indexed from,address indexed to,uint256 value)']);
export async function verifyMigration(rpc,{tokenAddress,numeraire,migrationHash,governanceType}) {
 const assert=(v,m)=>{if(!v)throw new Error(m);};
 assert(await rpc.getChainId()===31337,'Private Anvil chain required');
 assert(/anvil/i.test(await rpc.request({method:'web3_clientVersion'})),'Anvil required');
 const airlock=getAddresses(8453).airlock;
 // Detach the provider cache before any later state/header await.
 const receipt=structuredClone(await rpc.getTransactionReceipt({hash:migrationHash}));
 assert(receipt.status==='success','Migration receipt failed');
 const sameHash=(a,b)=>typeof a==='string'&&typeof b==='string'&&/^0x[0-9a-f]{64}$/i.test(a)&&a.toLowerCase()===b.toLowerCase();
 assert(sameHash(receipt.transactionHash,migrationHash),'Receipt transaction mismatch');
 const canonical=await rpc.getBlock({blockNumber:receipt.blockNumber});
 assert(canonical.number===receipt.blockNumber&&sameHash(canonical.hash,receipt.blockHash),'Receipt block is not canonical');
 const logIndices=new Set();
 for(const log of receipt.logs){
  assert(!log.removed&&log.blockNumber===receipt.blockNumber&&sameHash(log.blockHash,receipt.blockHash)&&sameHash(log.transactionHash,migrationHash),'Receipt log binding mismatch');
  assert(Number.isSafeInteger(log.logIndex)&&log.logIndex>=0&&!logIndices.has(log.logIndex),'Receipt log index missing or duplicated');logIndices.add(log.logIndex);
 }
 // EIP-1898 pins each state read to this exact canonical hash, including during reorgs.
 const pinnedRead=async(address,abi,functionName,args=[])=>decodeFunctionResult({abi,functionName,data:await rpc.request({method:'eth_call',params:[{to:address,data:encodeFunctionData({abi,functionName,args})},{blockHash:receipt.blockHash,requireCanonical:true}]})});
 const asset=await pinnedRead(airlock,airlockAbi,'getAssetData',[tokenAddress]);
 const events=receipt.logs.filter(l=>l.address.toLowerCase()===airlock.toLowerCase()).flatMap(l=>{try{return [{event:decodeEventLog({abi:airlockAbi,data:l.data,topics:l.topics}),log:l}];}catch{return [];}}).filter(e=>e.event.eventName==='Migrate');
 assert(events.length===1,'Exactly one canonical Airlock Migrate event required');
 const pair=asset[6];assert(getAddress(pair)!==zeroAddress,'Migration pair missing');
 assert(getAddress(events[0].event.args.asset)===getAddress(tokenAddress),'Migration event wrong token');
 assert(getAddress(events[0].event.args.pool)===getAddress(pair),'Migration event wrong pool');
 const migrator=getAddress(asset[3]),beneficiary=getAddress(asset[1]);
 assert(migrator===getAddress(V2_MIGRATOR),'Historical V2 migration module required');
 const transfers=receipt.logs.filter(l=>l.address.toLowerCase()===pair.toLowerCase()&&l.topics?.[0]?.toLowerCase()===TRANSFER_TOPIC).map(log=>{
  assert(log.topics.length===3&&log.topics.slice(1).every(t=>/^0x0{24}[0-9a-f]{40}$/i.test(t))&&/^0x[0-9a-f]{64}$/i.test(log.data),'Malformed LP Transfer event');
  let event;try{event=decodeEventLog({abi:lpTransferAbi,data:log.data,topics:log.topics,strict:true});}catch{throw new Error('Malformed LP Transfer event');}
  return {from:getAddress(event.args.from),to:getAddress(event.args.to),value:event.args.value,logIndex:log.logIndex};
 });
 const mints=transfers.filter(t=>t.from===zeroAddress&&t.to===migrator&&t.value>0n);
 const credits=transfers.filter(t=>t.from===migrator&&t.to===beneficiary&&t.from!==t.to&&t.value>0n);
 let attribution,attributionCount=0;
 for(const mint of mints)for(const credit of credits)if(mint.logIndex<credit.logIndex&&credit.logIndex<events[0].log.logIndex&&credit.value<=mint.value){
  attributionCount++;if(attributionCount===1)attribution={mint,credit};
 }
 assert(attributionCount===1,'Exactly one transaction-bound V2 LP attribution required');
 const beneficiaryLpNetCredit=transfers.reduce((sum,t)=>sum+(t.to===beneficiary?t.value:0n)-(t.from===beneficiary?t.value:0n),0n);
 const beneficiaryLpDebits=transfers.reduce((sum,t)=>sum+(t.from===beneficiary&&t.to!==beneficiary?t.value:0n),0n);
 const beneficiaryLpAttributedNetCredit=attribution.credit.value-beneficiaryLpDebits;
 assert(beneficiaryLpAttributedNetCredit>0n,'Transaction LP beneficiary attribution was cancelled');
 const abi=parseAbi(['function token0() view returns(address)','function token1() view returns(address)','function getReserves() view returns(uint112,uint112,uint32)','function totalSupply() view returns(uint256)','function balanceOf(address) view returns(uint256)']);
 const read=(functionName,args=[])=>pinnedRead(pair,abi,functionName,args);
 const [token0,token1,reserves,totalSupply,beneficiaryLP]=await Promise.all([read('token0'),read('token1'),read('getReserves'),read('totalSupply'),read('balanceOf',[asset[1]])]);
 assert([token0,token1].map(x=>x.toLowerCase()).sort().join() === [tokenAddress,numeraire].map(x=>x.toLowerCase()).sort().join(),'Pair currencies differ');
 assert(reserves[0]>0n&&reserves[1]>0n&&totalSupply>0n,'Empty migrated pool');
 assert(beneficiaryLP>0n,'No LP received by configured timelock beneficiary');
 if(governanceType==='noOp')assert(asset[1].toLowerCase()==='0x000000000000000000000000000000000000dead','NoOp beneficiary changed');
 const after=await rpc.getBlock({blockNumber:receipt.blockNumber});
 assert(after.number===receipt.blockNumber&&sameHash(after.hash,receipt.blockHash),'Canonical migration block changed during verification');
 return {canonicalBlockHashPinned:true,sourceTrust:'RPC canonicality assertion; not a cryptographic finality proof',receiptStatus:receipt.status,blockNumber:receipt.blockNumber,blockHash:receipt.blockHash,migrationHash,airlock,pair,token0,token1,reserves,totalSupply,beneficiary:asset[1],beneficiaryLP,lpAttribution:{migrator,mintedLP:attribution.mint.value,beneficiaryLpCredit:attribution.credit.value,beneficiaryLpNetCredit,beneficiaryLpAttributedNetCredit,mintLogIndex:attribution.mint.logIndex,creditLogIndex:attribution.credit.logIndex,migrateLogIndex:events[0].log.logIndex},asset,event:events[0].event,publicDeployment:false,nativeNanoTested:false};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 const [input,output]=process.argv.slice(2);
 if(!input||!output)throw new Error('Usage: node verify-migration.mjs FAILED-DRIVER-REPORT NEW-VERIFICATION.json');
 const r=JSON.parse(await readFile(input,'utf8'));
 const rpc=createPublicClient({transport:http('http://127.0.0.1:19546',{retryCount:0})});
 const result=await verifyMigration(rpc,{...r.predicted,numeraire:r.configuration.numeraire,migrationHash:r.steps.find(s=>s.name==='migrate').hash,governanceType:r.configuration.governance.type});
 await writeFile(output,JSON.stringify({originalDriverComplete:r.complete,originalDriverError:r.error,terminalVerificationPassed:true,result},(_,v)=>typeof v==='bigint'?v.toString():v,2)+'\n',{flag:'wx'});
 console.log('Private migration reserves, event and LP beneficiary verified');
}
