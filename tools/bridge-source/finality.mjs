// Read-only Base receipt gate. No wallet, signing, minting or relaying.
import {createHash} from 'node:crypto';
const check=(v,m)=>{if(!v)throw Error(m);};
const hash=v=>typeof v==='string'&&/^0x[0-9a-f]{64}$/.test(v);
const address=v=>typeof v==='string'&&/^0x[0-9a-f]{40}$/.test(v)&&v!=='0x'+'0'.repeat(40);
const data=v=>typeof v==='string'&&/^0x(?:[0-9a-f]{2})*$/.test(v)&&v.length<=131074;
function quantity(v){check(typeof v==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(v)&&v.length<=66,'Noncanonical/oversized source quantity');return BigInt(v);}
function header(v){check(v&&hash(v.hash),'Missing source header');quantity(v.number);return {hash:v.hash,number:v.number};}
function validateLog(log,receipt){
 check(log&&log.removed===false&&address(log.address)&&hash(log.transactionHash)&&hash(log.blockHash)&&data(log.data)&&Array.isArray(log.topics)&&log.topics.length<=4&&log.topics.every(hash),'Malformed/removed source log');
 check(log.transactionHash===receipt.transactionHash&&log.blockHash===receipt.blockHash&&log.blockNumber===receipt.blockNumber,'Source log receipt binding differs');
 quantity(log.logIndex);quantity(log.transactionIndex);
 check(log.transactionIndex===receipt.transactionIndex,'Source log transaction index differs');
}
function receipt(v,tx){
 check(v&&v.status==='0x1'&&v.transactionHash===tx&&hash(v.blockHash),'Missing/failed/wrong source receipt');
 quantity(v.blockNumber);quantity(v.transactionIndex);
 check(Array.isArray(v.logs)&&v.logs.length<=256,'Source receipt log budget');
 const indices=new Set();for(const log of v.logs){validateLog(log,v);check(!indices.has(log.logIndex),'Duplicate source log index');indices.add(log.logIndex);}
 // Keep only verified fields, detached from mutable provider cache objects.
 return {status:v.status,transactionHash:v.transactionHash,blockHash:v.blockHash,blockNumber:v.blockNumber,transactionIndex:v.transactionIndex,
  logs:v.logs.map(l=>({address:l.address,topics:[...l.topics],data:l.data,logIndex:l.logIndex,transactionIndex:l.transactionIndex,blockNumber:l.blockNumber,blockHash:l.blockHash,transactionHash:l.transactionHash,removed:l.removed}))};
}
function projection(v){return JSON.stringify([v.status,v.transactionHash,v.blockHash,v.blockNumber,v.transactionIndex,v.logs.map(l=>[l.address,l.topics,l.data,l.logIndex,l.transactionIndex,l.blockNumber,l.blockHash,l.transactionHash,l.removed])]);}
export async function observeBaseFinalizedReceipt(rpc,{transactionHash,event}={}){
 check(hash(transactionHash),'Canonical transaction hash required');
 check(event&&address(event.emitter)&&Array.isArray(event.topics)&&event.topics.length>0&&event.topics.length<=4&&event.topics.every(hash)&&data(event.data)&&/^[0-9a-f]{64}$/.test(event.codeSha256??''),'Exact reviewed event and emitter code SHA-256 required');
 quantity(event.logIndex);
 // A configuration reload must not change the reviewed event during awaits.
 event={emitter:event.emitter,topics:[...event.topics],data:event.data,logIndex:event.logIndex,codeSha256:event.codeSha256};
 const request=(method,params)=>rpc.request({method,params});
 // Project each response as it arrives, before any subsequent RPC await.
 const requestHeader=tag=>Promise.resolve(request('eth_getBlockByNumber',[tag,false])).then(header);
 const requestReceipt=()=>Promise.resolve(request('eth_getTransactionReceipt',[transactionHash])).then(value=>receipt(value,transactionHash));
 check(await request('eth_chainId',[])==='0x2105','Base mainnet chain ID required');
 // Base finalized is derived from L1 finality; never substitute safe/latest
 // or a fixed number of sequencer confirmations.
 const finalized=await requestHeader('finalized');
 const first=await requestReceipt();
 check(quantity(first.blockNumber)<=quantity(finalized.number),'Receipt is newer than finalized source head');
 const canonical=await requestHeader(first.blockNumber);
 check(canonical.number===first.blockNumber&&canonical.hash===first.blockHash,'Source receipt block is not canonical');
 const candidates=first.logs.filter(l=>l.logIndex===event.logIndex);
 check(candidates.length===1,'Exactly one selected source log required');
 const selected=candidates[0];check(selected.address===event.emitter&&selected.data===event.data&&JSON.stringify(selected.topics)===JSON.stringify(event.topics),'Source event differs from reviewed event');
 const code=await request('eth_getCode',[event.emitter,{blockHash:first.blockHash,requireCanonical:true}]);
 check(data(code)&&code!=='0x','Source emitter code unavailable at canonical receipt hash');
 const digest=createHash('sha256').update(Buffer.from(code.slice(2),'hex')).digest('hex');check(digest===event.codeSha256,'Source emitter code SHA-256 differs');
 const [after,finalizedAtHeight,latestFinalized,second,chain]=await Promise.all([
  requestHeader(first.blockNumber),
  requestHeader(finalized.number),
  requestHeader('finalized'),
  requestReceipt(),request('eth_chainId',[])
 ]);
 check(chain==='0x2105','Source chain ID changed during observation');
 check(after.number===first.blockNumber&&after.hash===first.blockHash,'Canonical source block changed during observation');
 check(finalizedAtHeight.number===finalized.number&&finalizedAtHeight.hash===finalized.hash,'Finalized source checkpoint changed during observation');
 check(quantity(latestFinalized.number)>=quantity(finalized.number),'Finalized source height regressed');
 if(latestFinalized.number===finalized.number)check(latestFinalized.hash===finalized.hash,'Finalized source hash changed at same height');
 else{const currentCanonical=await requestHeader(latestFinalized.number);check(currentCanonical.number===latestFinalized.number&&currentCanonical.hash===latestFinalized.hash,'Advanced finalized source checkpoint is not canonical');}
 check(projection(second)===projection(first),'Source receipt changed during observation');
 return {format:'nanobees-base-source-observation-v1',chainId:8453,transactionHash,blockNumber:first.blockNumber,blockHash:first.blockHash,sourceEventKey:`8453:${transactionHash}:${selected.logIndex}`,event:selected,emitterCodeSha256:digest,finalizedNumber:finalized.number,finalizedHash:finalized.hash,rpcFinalizedObservation:true,canonicalHashPinned:true,cryptographicFinalityVerified:false,eligibleForMint:false,sourceTrust:'RPC chain/canonicality/finality assertions only; no authenticated L1 derivation or receipt inclusion proof',productionQualified:false};
}
