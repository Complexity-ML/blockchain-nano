import {createPublicClient,http,parseAbi,decodeEventLog,getAddress,zeroAddress,encodeFunctionData,decodeFunctionResult} from 'viem';
import {airlockAbi,getAddresses} from '@whetstone-research/doppler-sdk/evm';
import {readFile,writeFile} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
export async function verifyMigration(rpc,{tokenAddress,numeraire,migrationHash,governanceType}) {
 const assert=(v,m)=>{if(!v)throw new Error(m);};
 assert(await rpc.getChainId()===31337,'Private Anvil chain required');
 assert(/anvil/i.test(await rpc.request({method:'web3_clientVersion'})),'Anvil required');
 const airlock=getAddresses(8453).airlock;
 const receipt=await rpc.getTransactionReceipt({hash:migrationHash});
 assert(receipt.status==='success','Migration receipt failed');
 const sameHash=(a,b)=>typeof a==='string'&&typeof b==='string'&&/^0x[0-9a-f]{64}$/i.test(a)&&a.toLowerCase()===b.toLowerCase();
 assert(sameHash(receipt.transactionHash,migrationHash),'Receipt transaction mismatch');
 const canonical=await rpc.getBlock({blockNumber:receipt.blockNumber});
 assert(canonical.number===receipt.blockNumber&&sameHash(canonical.hash,receipt.blockHash),'Receipt block is not canonical');
 for(const log of receipt.logs)assert(!log.removed&&log.blockNumber===receipt.blockNumber&&sameHash(log.blockHash,receipt.blockHash)&&sameHash(log.transactionHash,migrationHash),'Receipt log binding mismatch');
 // EIP-1898 pins each state read to this exact canonical hash, including during reorgs.
 const pinnedRead=async(address,abi,functionName,args=[])=>decodeFunctionResult({abi,functionName,data:await rpc.request({method:'eth_call',params:[{to:address,data:encodeFunctionData({abi,functionName,args})},{blockHash:receipt.blockHash,requireCanonical:true}]})});
 const asset=await pinnedRead(airlock,airlockAbi,'getAssetData',[tokenAddress]);
 const events=receipt.logs.filter(l=>l.address.toLowerCase()===airlock.toLowerCase()).flatMap(l=>{try{return [decodeEventLog({abi:airlockAbi,data:l.data,topics:l.topics})];}catch{return [];}}).filter(e=>e.eventName==='Migrate');
 assert(events.length===1,'Exactly one canonical Airlock Migrate event required');
 const pair=asset[6];assert(getAddress(pair)!==zeroAddress,'Migration pair missing');
 assert(getAddress(events[0].args.asset)===getAddress(tokenAddress),'Migration event wrong token');
 assert(getAddress(events[0].args.pool)===getAddress(pair),'Migration event wrong pool');
 const abi=parseAbi(['function token0() view returns(address)','function token1() view returns(address)','function getReserves() view returns(uint112,uint112,uint32)','function totalSupply() view returns(uint256)','function balanceOf(address) view returns(uint256)']);
 const read=(functionName,args=[])=>pinnedRead(pair,abi,functionName,args);
 const [token0,token1,reserves,totalSupply,beneficiaryLP]=await Promise.all([read('token0'),read('token1'),read('getReserves'),read('totalSupply'),read('balanceOf',[asset[1]])]);
 assert([token0,token1].map(x=>x.toLowerCase()).sort().join() === [tokenAddress,numeraire].map(x=>x.toLowerCase()).sort().join(),'Pair currencies differ');
 assert(reserves[0]>0n&&reserves[1]>0n&&totalSupply>0n,'Empty migrated pool');
 assert(beneficiaryLP>0n,'No LP received by configured timelock beneficiary');
 if(governanceType==='noOp')assert(asset[1].toLowerCase()==='0x000000000000000000000000000000000000dead','NoOp beneficiary changed');
 const after=await rpc.getBlock({blockNumber:receipt.blockNumber});
 assert(after.number===receipt.blockNumber&&sameHash(after.hash,receipt.blockHash),'Canonical migration block changed during verification');
 return {canonicalBlockHashPinned:true,sourceTrust:'RPC canonicality assertion; not a cryptographic finality proof',receiptStatus:receipt.status,blockNumber:receipt.blockNumber,blockHash:receipt.blockHash,migrationHash,airlock,pair,token0,token1,reserves,totalSupply,beneficiary:asset[1],beneficiaryLP,asset,event:events[0],publicDeployment:false,nativeNanoTested:false};
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
