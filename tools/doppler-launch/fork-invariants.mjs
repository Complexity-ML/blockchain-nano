// Guards for the existing private synthetic fixture, never a public write RPC.
const need=(ok,why)=>{if(!ok)throw Error('Private fork: '+why);};
const sameHash=(a,b)=>typeof a==='string'&&typeof b==='string'&&/^0x[0-9a-f]{64}$/i.test(a)&&a.toLowerCase()===b.toLowerCase();
// Retained initial header in the public attempt-b fixture; no new chain claim.
export const SYNTHETIC_BASE_FORK=Object.freeze({sourceChainId:8453,blockNumber:52173454n,
 blockHash:'0xba5cc2e9e91ce6f64babb91e8bbf5a38d71e9f2fd01bc17473519a5c3dc6be5d'});

export async function requireFreshSyntheticBaseFork(rpc){
 need(await rpc.getChainId()===31337,'local chain ID must be 31337');
 const version=await rpc.request({method:'web3_clientVersion'});
 need(typeof version==='string'&&/anvil/i.test(version),'Anvil required');
 const info=structuredClone(await rpc.request({method:'anvil_nodeInfo'}));
 need(info?.environment?.chainId===31337&&info?.forkConfig?.forkBlockNumber===Number(SYNTHETIC_BASE_FORK.blockNumber),
   'expected fork configuration');
 need(typeof info.currentBlockNumber==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(info.currentBlockNumber)&&
   BigInt(info.currentBlockNumber)===SYNTHETIC_BASE_FORK.blockNumber&&sameHash(info.currentBlockHash,SYNTHETIC_BASE_FORK.blockHash),
   'fresh initial fork head required');
 const observed=await rpc.getBlock({blockNumber:SYNTHETIC_BASE_FORK.blockNumber});
 const anchor={number:observed.number,hash:observed.hash};
 need(anchor.number===SYNTHETIC_BASE_FORK.blockNumber&&sameHash(anchor.hash,SYNTHETIC_BASE_FORK.blockHash),'Base anchor header mismatch');
 const latest=await rpc.getBlock({blockTag:'latest'});
 need(latest.number===anchor.number&&sameHash(latest.hash,anchor.hash),'initial fork head changed');
 return {version,info,anchor:{sourceChainId:8453,blockNumber:anchor.number,blockHash:anchor.hash},
   freshInitialHeadMatched:true,sourceTrust:'RPC assertion against retained public header; not cryptographic finality'};
}

export function verifySwapBalanceDelta({amountIn,minimumOutput,before,after}){
 need(typeof amountIn==='bigint'&&amountIn>0n&&typeof minimumOutput==='bigint'&&minimumOutput>=0n,'explicit input/minimum amounts');
 for(const s of [before,after])need(s&&['input','output'].every(k=>typeof s[k]==='bigint'&&s[k]>=0n),'nonnegative balance observations');
 const inputDelta=before.input-after.input,amountOutReceived=after.output-before.output;
 need(inputDelta===amountIn,'input delta differs from exact amount');
 need(amountOutReceived>0n,'no output received');
 need(amountOutReceived>=minimumOutput,'received output below encoded minimum');
 return {amountIn,inputDelta,minimumOutput,amountOutReceived};
}
