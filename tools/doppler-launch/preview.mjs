import { readFile, writeFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { DopplerSDK, getAddresses, airlockAbi } from '@whetstone-research/doppler-sdk/evm';
import { createPublicClient, custom, encodeFunctionData, getAddress, keccak256, zeroAddress, decodeFunctionResult } from 'viem';
import { base } from 'viem/chains';
import {createLaunchRpc} from '../bridge-source/rpc.mjs';

export const USDC = '0x833589fCD6eDb6E08f4c7C32D4f71b54bdA02913';
const integer = (x, name) => {
  if (!Number.isSafeInteger(x) || x < 0) throw new Error(`${name}: unsigned safe integer required`);
  return x;
};
const amount = (x, name) => {
  if (typeof x !== 'string' || !/^(0|[1-9][0-9]*)$/.test(x)) throw new Error(`${name}: base-unit decimal string required`);
  return BigInt(x);
};
const fields = (o, allowed, name) => {
  if (!o || typeof o !== 'object' || Array.isArray(o)) throw new Error(`${name}: object required`);
  for (const k of Object.keys(o)) if (!allowed.includes(k)) throw new Error(`${name}: unsupported field ${k}`);
};
export function parameters(c) {
  fields(c,['chainId','mode','nativeAssetRelationship','creator','numeraire','token','initialSupply','numTokensToSell','yearlyMintRate','auction','pool','governance','migration','referenceTimestamp','startTimeOffset'],'config');
  fields(c.token,['name','symbol','tokenURI'],'token');
  fields(c.auction,['duration','epochLength','startTick','endTick','minProceeds','maxProceeds','numPdSlugs','gamma'],'auction');
  fields(c.pool,['fee','tickSpacing'],'pool');
  fields(c.governance,['type','multisig'],'governance');
  fields(c.migration,['type','fee','tickSpacing','streamableFees'],'migration');
  if (c.chainId !== 8453 || c.mode !== 'dynamic') throw new Error('Only Base Dynamic is supported');
  if (!c.nativeAssetRelationship?.trim()) throw new Error('Describe the Base token relationship to native Nano');
  const creator = getAddress(c.creator);
  if (creator === zeroAddress) throw new Error('Creator cannot be zero');
  if (getAddress(c.numeraire) !== USDC) throw new Error('Native Base USDC required');
  if (!c.token?.name?.trim() || !c.token?.symbol?.trim() || !c.token?.tokenURI?.trim()) throw new Error('Token identity required');
  const initialSupply = amount(c.initialSupply, 'initialSupply');
  const numTokensToSell = amount(c.numTokensToSell, 'numTokensToSell');
  if (numTokensToSell <= 0n || initialSupply <= numTokensToSell) throw new Error('Supply must include a positive sale and migration remainder');
  const yearlyMintRate = amount(c.yearlyMintRate, 'yearlyMintRate');
  const a = c.auction;
  integer(a.gamma,'gamma');
  if (!a.gamma || a.gamma > 8388607) throw new Error('gamma: positive int24 required; no SDK default');
  for (const key of ['duration', 'epochLength', 'numPdSlugs']) integer(a[key], key);
  if (!a.duration || !a.epochLength || a.duration % a.epochLength || !a.numPdSlugs) throw new Error('Invalid auction epochs/slugs');
  for (const key of ['startTick', 'endTick']) if (!Number.isSafeInteger(a[key]) || Math.abs(a[key]) > 887272) throw new Error(`Invalid ${key}`);
  const minProceeds = amount(a.minProceeds, 'minProceeds');
  const maxProceeds = amount(a.maxProceeds, 'maxProceeds');
  if (minProceeds <= 0n || maxProceeds < minProceeds) throw new Error('Invalid USDC proceeds');
  const pool = { fee: integer(c.pool.fee, 'fee'), tickSpacing: integer(c.pool.tickSpacing, 'tickSpacing') };
  if (!pool.tickSpacing || pool.tickSpacing > 30 || pool.fee > 1000000) throw new Error('Invalid Dynamic pool tickSpacing/fee');
  const normalizedEpoch = BigInt(a.epochLength) * 10n**18n / BigInt(a.duration);
  if (a.gamma % pool.tickSpacing || normalizedEpoch * BigInt(a.gamma) / 10n**18n === 0n) throw new Error('InvalidGamma: explicit total-curve gamma must be spacing-aligned and yield a nonzero upper slug');
  if (!['noOp', 'launchpad'].includes(c.governance.type)) throw new Error('Explicit noOp/launchpad governance required');
  const governance = c.governance.type === 'launchpad' ? {type:'launchpad', multisig:getAddress(c.governance.multisig)} : {type:'noOp'};
  if (governance.multisig === zeroAddress) throw new Error('Multisig cannot be zero');
  // No implicit migration, module overrides, proceeds split, inflation or wallet.
  if (!['uniswapV2', 'uniswapV4'].includes(c.migration.type)) throw new Error('Explicit Uniswap migration required');
  let migration = {type:c.migration.type};
  if (migration.type === 'uniswapV4') {
    const s = c.migration.streamableFees;
    if (!s || !Array.isArray(s.beneficiaries) || !s.beneficiaries.length) throw new Error('Explicit V4 fee beneficiaries/lock required');
    fields(s,['lockDuration','beneficiaries'],'streamableFees');
    const beneficiaries = s.beneficiaries.map(b => ({beneficiary:getAddress(b.beneficiary), shares:amount(b.shares,'shares')}));
    if (beneficiaries.some(b => b.beneficiary === zeroAddress || b.shares <= 0n) || beneficiaries.reduce((v,b) => v+b.shares,0n) !== 10n**18n) throw new Error('Fee shares must sum to 1e18');
    migration = {...migration, fee:integer(c.migration.fee,'migration fee'), tickSpacing:integer(c.migration.tickSpacing,'migration spacing'), streamableFees:{lockDuration:integer(s.lockDuration,'lockDuration'),beneficiaries}};
  }
  return {token:{type:'standard', name:c.token.name, symbol:c.token.symbol, tokenURI:c.token.tokenURI, yearlyMintRate}, sale:{initialSupply,numTokensToSell,numeraire:USDC}, auction:{...a,minProceeds,maxProceeds}, pool, governance, migration, userAddress:creator, blockTimestamp:integer(c.referenceTimestamp,'referenceTimestamp'), startTimeOffset:integer(c.startTimeOffset,'startTimeOffset')};
}

export async function preview(c, rpc) {
  // Capture the reviewed terms before SDK/RPC awaits can observe caller reloads.
  c = structuredClone(c);
  const params = parameters(c);
  const rpcErrors=[];
  const launchRpc = rpc ? createLaunchRpc(rpc,{onRpcError:diagnostic=>rpcErrors.push(diagnostic)}) : null;
  const client = createPublicClient({chain:base, transport:custom({request:async ({method,params:args}) => {
    if (!launchRpc) throw new Error(`RPC disabled: ${method}`);
    return launchRpc.request({method,params:args??[]});
  }},{retryCount:0})});
  const sdk = new DopplerSDK({publicClient:client,chainId:8453});
  const {createParams,tokenAddress,hookAddress} = await sdk.factory.encodeCreateDynamicAuctionParams(params);
  const to = getAddresses(8453).airlock;
  const data = encodeFunctionData({abi:airlockAbi,functionName:'create',args:[createParams]});
  const result = {schema:1,sdkVersion:'1.0.43',status:'unsigned-preview',productionReady:false,nativeAssetRelationship:c.nativeAssetRelationship,config:c,transaction:{chainId:8453,from:params.userAddress,to,value:'0',data},calldataHash:keccak256(data),predicted:{tokenAddress,hookAddress},createParams,simulation:{performed:false}};
  if (rpc) {
    try {
    if (await client.getChainId() !== 8453) throw new Error('RPC chain mismatch');
    const observed=await client.getBlock({blockTag:'latest'});
    // Do not keep a provider/cache header object across the following awaits.
    const block={number:observed.number,hash:observed.hash,timestamp:observed.timestamp};
    if (typeof block.number!=='bigint' || block.number<0n || typeof block.timestamp!=='bigint' || !/^0x[0-9a-f]{64}$/i.test(block.hash ?? '')) throw new Error('Invalid reference block');
    const selector={blockHash:block.hash,requireCanonical:true};
    if (BigInt(params.blockTimestamp + params.startTimeOffset) <= block.timestamp) throw new Error('Launch start is not in the future; refresh referenceTimestamp and review new calldata');
    const modules = {};
    for (const [name,address] of Object.entries({airlock:to,numeraire:USDC,tokenFactory:createParams.tokenFactory,governanceFactory:createParams.governanceFactory,poolInitializer:createParams.poolInitializer,liquidityMigrator:createParams.liquidityMigrator})) {
      const code=await client.request({method:'eth_getCode',params:[address,selector]});
      if (!code || code==='0x') throw new Error(`Missing deployed code: ${name}`);
      modules[name]={address,codeHash:keccak256(code)};
    }
    const decimals = await client.request({method:'eth_call',params:[{to:USDC,data:'0x313ce567'},selector]});
    if (!decimals || BigInt(decimals)!==6n) throw new Error('Unexpected USDC decimals');
    result.simulation={performed:true,success:false,referenceHashPinned:false,gasEstimateHashExtensionRequired:true,blockNumber:block.number,blockHash:block.hash,modules};
    const call={from:params.userAddress,to,data,value:'0x0'};
    const simulated=decodeFunctionResult({abi:airlockAbi,functionName:'create',data:await client.request({method:'eth_call',params:[call,selector]})});
    if (getAddress(simulated[0]) !== getAddress(tokenAddress) || getAddress(simulated[1]) !== getAddress(hookAddress)) throw new Error('Simulated token/hook differ from SDK predictions');
    // Hash selection for estimateGas is a required provider extension, not EIP-1898.
    // Refuse unsupported providers; never fall back to a height or latest state.
    const estimate=await client.request({method:'eth_estimateGas',params:[call,selector]});
    if (typeof estimate!=='string' || !/^0x[1-9a-f][0-9a-f]{0,63}$/i.test(estimate)) throw new Error('Invalid gas estimate');
    const gas=BigInt(estimate),after=await client.getBlock({blockNumber:block.number});
    if (after.number!==block.number || after.hash!==block.hash) throw new Error('Reference block changed');
    result.simulation={performed:true,success:true,referenceHashPinned:true,gasEstimateHashExtensionRequired:true,gasEstimateHashExtensionAccepted:true,blockNumber:block.number,blockHash:block.hash,gas,modules,returnedAddresses:simulated};
    } catch(e) {
      result.status='simulation-rejected';
      result.simulation={...result.simulation,performed:true,success:false,referenceHashPinned:false,error:e.shortMessage ?? e.message.split('\n')[0],rpcErrors};
    }
  }
  return result;
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  try {
    const [input,output,...rest]=process.argv.slice(2);
    if (!input || !output || rest.length) throw new Error('Usage: node preview.mjs config.json output.json (optional BASE_RPC_URL for read-only simulation)');
    const result=await preview(JSON.parse(await readFile(input,'utf8')),process.env.BASE_RPC_URL);
    await writeFile(output,JSON.stringify(result,(_,v)=>typeof v==='bigint'?v.toString():v,2)+'\n',{flag:'wx'});
    console.log(`Unsigned review saved: ${output}; status=${result.status}`);
    if (result.status==='simulation-rejected') process.exitCode=1;
  } catch(e) {console.error(e.message);process.exitCode=1;}
}
