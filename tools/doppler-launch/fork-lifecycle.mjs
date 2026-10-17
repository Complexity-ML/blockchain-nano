// Local-only synthetic financial lifecycle; never accepts a public write RPC.
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {createPublicClient,createWalletClient,http,parseAbi,maxUint256,encodeAbiParameters,encodeFunctionData,toFunctionSelector} from 'viem';
import {DopplerSDK,getAddresses,airlockAbi,dopplerHookAbi} from '@whetstone-research/doppler-sdk/evm';
// Published package index has extensionless ESM imports; use its explicit files.
import {CommandBuilder} from 'doppler-router/dist/CommandBuilder.js';
import {V4ActionBuilder,V4ActionType} from 'doppler-router/dist/V4ActionBuilder.js';
import {parameters,USDC} from './preview.mjs';
import {verifyMigration} from './verify-migration.mjs';
import {requireFreshSyntheticBaseFork,verifySwapBalanceDelta} from './fork-invariants.mjs';
const url='http://127.0.0.1:19546';
const chain={id:31337,name:'Nanobees isolated Base fork',nativeCurrency:{name:'Test ETH',symbol:'ETH',decimals:18},rpcUrls:{default:{http:[url]}}};
const rpc=createPublicClient({chain,transport:http(url,{retryCount:0,timeout:30000})});
const root=process.argv[2];if(!root)throw new Error('Usage: node fork-lifecycle.mjs NEW-OUTPUT-DIRECTORY');
await mkdir(root);
const report={schema:1,scope:'synthetic private Anvil Base fork only',publicDeployment:false,nativeNanoTested:false,steps:[],complete:false};
const save=()=>writeFile(root+'/result.json',JSON.stringify(report,(_,v)=>typeof v==='bigint'?v.toString():v,2)+'\n');
const record=async(name,details)=>{report.steps.push({name,...details});await save();console.log(name);};
const assert=(ok,msg)=>{if(!ok)throw new Error(msg);};
const tokenAbi=parseAbi(['function owner() view returns(address)','function totalSupply() view returns(uint256)','function transferOwnership(address)','function balanceOf(address) view returns(uint256)','function approve(address,uint256) returns(bool)','function masterMinter() view returns(address)','function configureMinter(address,uint256) returns(bool)','function mint(address,uint256) returns(bool)']);
const routerAbi=parseAbi(['function execute(bytes,bytes[]) payable']);
const permitAbi=parseAbi(['function approve(address,address,uint160,uint48)']);
try {
 // All anchor reads finish before wallet creation, funding or impersonation.
 report.anvil=await requireFreshSyntheticBaseFork(rpc);
 const c=JSON.parse(await readFile(new URL('./example.synthetic.json',import.meta.url)));
 c.referenceTimestamp=Number((await rpc.getBlock()).timestamp);c.startTimeOffset=7200;
 c.auction={...c.auction,startTick:-338490,endTick:-361500,gamma:23010,duration:600,epochLength:10,minProceeds:'100000000',maxProceeds:'1000000000'};
 const account=c.creator;const sdk=new DopplerSDK({publicClient:rpc,chainId:8453});
 const addresses=getAddresses(8453);
 const {createParams,tokenAddress,hookAddress}=await sdk.factory.encodeCreateDynamicAuctionParams(parameters(c));
 report.configuration=c;report.predicted={tokenAddress,hookAddress};report.baseModules=addresses;
 const wallet=createWalletClient({chain,transport:http(url,{retryCount:0}),account});
 const impersonate=async(address)=>{await rpc.request({method:'anvil_impersonateAccount',params:[address]});await rpc.request({method:'anvil_setBalance',params:[address,'0x56bc75e2d63100000']});};
 await impersonate(account);
 const nextTimestamp=async()=>{const block=await rpc.getBlock();await rpc.request({method:'evm_setNextBlockTimestamp',params:[Number(block.timestamp)+1]});};
 const tx=async(name,address,abi,functionName,args,sender=account)=>{
  await nextTimestamp();
  const hash=await wallet.writeContract({address,abi,functionName,args,account:sender,chain,gas:15000000n});
  const receipt=await rpc.waitForTransactionReceipt({hash});
  await record(name,{hash,status:receipt.status,gasUsed:receipt.gasUsed,blockNumber:receipt.blockNumber});
  assert(receipt.status==='success',name+' reverted');return receipt;
 };
 const pairAbi=parseAbi(['function getReserves() view returns(uint112,uint112,uint32)','function totalSupply() view returns(uint256)','function balanceOf(address) view returns(uint256)']);
 const snapshot=async()=>{
  const asset=await rpc.readContract({address:addresses.airlock,abi:airlockAbi,functionName:'getAssetData',args:[tokenAddress]});
  const pair=asset[6];const code=await rpc.getCode({address:pair});
  return {
   asset,
   owner:await rpc.readContract({address:tokenAddress,abi:tokenAbi,functionName:'owner'}),
   supply:await rpc.readContract({address:tokenAddress,abi:tokenAbi,functionName:'totalSupply'}),
   hookState:await rpc.readContract({address:hookAddress,abi:dopplerHookAbi,functionName:'state'}),
   balances:await Promise.all([account,hookAddress,addresses.airlock,pair].flatMap(holder=>[USDC,tokenAddress].map(address=>rpc.readContract({address,abi:tokenAbi,functionName:'balanceOf',args:[holder]})))),
   pool:code&&code!=='0x'?{
    reserves:await rpc.readContract({address:pair,abi:pairAbi,functionName:'getReserves'}),
    lpSupply:await rpc.readContract({address:pair,abi:pairAbi,functionName:'totalSupply'}),
    beneficiaryLP:await rpc.readContract({address:pair,abi:pairAbi,functionName:'balanceOf',args:[asset[1]]})
   }:null
  };
 };
 const serialize=v=>JSON.stringify(v,(_,x)=>typeof x==='bigint'?x.toString():x);
 const rejected=async(name,address,abi,functionName,args,sender,selectors)=>{
  const before=await snapshot();const data=encodeFunctionData({abi,functionName,args});
  let revertData,error;try {await rpc.request({method:'eth_call',params:[{from:sender,to:address,data,gas:'0xe4e1c0'},'latest']});}
  catch(e){error=e.shortMessage??e.message;for(let c=e;c;c=c.cause){if(typeof c.data==='string'&&/^0x[0-9a-f]+$/i.test(c.data)){revertData=c.data;break;}}}
  assert(error&&revertData,'Expected explicit contract revert data: '+name);
  assert(selectors.includes(revertData.slice(0,10)),'Unexpected revert selector '+revertData+' for '+name);
  await nextTimestamp();
  const hash=await wallet.writeContract({address,abi,functionName,args,account:sender,chain,gas:15000000n});
  const receipt=await rpc.waitForTransactionReceipt({hash});
  const after=await snapshot();const unchanged=serialize(before)===serialize(after);
  await record(name,{hash,status:receipt.status,gasUsed:receipt.gasUsed,revertData,error,before,after,unchanged});
  assert(receipt.status==='reverted',name+' unexpectedly succeeded');
  assert(receipt.gasUsed<15000000n,name+' consumed all gas; contract rejection not established');
  assert(unchanged,name+' changed contract state or balances');
 };
 const unauthorized=[toFunctionSelector('Unauthorized()'),toFunctionSelector('OwnableUnauthorizedAccount(address)')];
 await tx('create',addresses.airlock,airlockAbi,'create',[createParams]);
 assert((await rpc.getCode({address:tokenAddress}))!=='0x','Token not deployed');
 assert((await rpc.getCode({address:hookAddress}))!=='0x','Hook not deployed');
 await rejected('reject-premature-migration',addresses.airlock,airlockAbi,'migrate',[tokenAddress],account,[toFunctionSelector('CannotMigrate()')]);
 const attacker='0x2222222222222222222222222222222222222222';await impersonate(attacker);
 await rejected('reject-attacker-ownership',tokenAddress,tokenAbi,'transferOwnership',[attacker],attacker,unauthorized);
 const master=await rpc.readContract({address:USDC,abi:tokenAbi,functionName:'masterMinter'});
 await impersonate(master);
 await tx('synthetic-USDC-enable-minter',USDC,tokenAbi,'configureMinter',[account,10000000000n],master);
 await tx('synthetic-USDC-mint',USDC,tokenAbi,'mint',[account,10000000000n]);
 const start=await rpc.readContract({address:hookAddress,abi:dopplerHookAbi,functionName:'startingTime'});
 await rpc.request({method:'evm_setNextBlockTimestamp',params:[Number(start)+1]});await rpc.request({method:'evm_mine',params:[]});
 const key=await rpc.readContract({address:hookAddress,abi:dopplerHookAbi,functionName:'poolKey'});
 const poolKey={currency0:key[0],currency1:key[1],fee:key[2],tickSpacing:key[3],hooks:key[4]};
 const balance=(address)=>rpc.readContract({address,abi:tokenAbi,functionName:'balanceOf',args:[account]});
 const swap=async(name,input,output,amount)=>{
  await tx(name+'-erc20-approve',input,tokenAbi,'approve',[addresses.permit2,maxUint256]);
  await tx(name+'-permit2-approve',addresses.permit2,permitAbi,'approve',[input,addresses.universalRouter,2n**160n-1n,2**48-1]);
  const latest=await rpc.getBlock();const epochLength=BigInt(c.auction.epochLength);
  const quoteTime=start+((latest.timestamp-start)/epochLength+1n)*epochLength+1n;
  await rpc.request({method:'evm_setNextBlockTimestamp',params:[Number(quoteTime)]});await rpc.request({method:'evm_mine',params:[]});
  const zeroForOne=input.toLowerCase()===poolKey.currency0.toLowerCase();
  const quote=await sdk.quoter.quoteExactInputV4({poolKey,zeroForOne,exactAmount:amount,hookData:'0x'});
  assert(quote.amountOut>0n,'Zero quote');
  const minimumOutput=quote.amountOut*99n/100n;
  const before={input:await balance(input),output:await balance(output)};
  await record(name+'-quote',{quoteTime,amountIn:amount,amountOut:quote.amountOut,minimumOutput});
  const actions=new V4ActionBuilder();actions.addSwapExactInSingle(poolKey,zeroForOne,amount,minimumOutput,'0x');
  actions.addAction(V4ActionType.SETTLE_ALL,[input,amount]);actions.addAction(V4ActionType.TAKE_ALL,[output,minimumOutput]);
  const [a,p]=actions.build();const commands=new CommandBuilder();commands.addV4Swap(a,p);const [cmd,args]=commands.build();
  await tx(name,addresses.universalRouter,routerAbi,'execute',[cmd,args]);
  const after={input:await balance(input),output:await balance(output)};
  const deltas=verifySwapBalanceDelta({amountIn:amount,minimumOutput,before,after});
  await record(name+'-balances',{amount,quote:quote.amountOut,before,after,...deltas});
 };
 await swap('buy',USDC,tokenAddress,400000000n);
 const acquired=await balance(tokenAddress);await swap('sell',tokenAddress,USDC,acquired/10n);
 const state=await rpc.readContract({address:hookAddress,abi:dopplerHookAbi,functionName:'state'});
 const proceeds=state[3];assert(proceeds>=100000000n,'Net proceeds below minimum');
 await record('net-proceeds',{state,proceeds});
 const end=await rpc.readContract({address:hookAddress,abi:dopplerHookAbi,functionName:'endingTime'});
 await rpc.request({method:'evm_setNextBlockTimestamp',params:[Number(end)+1]});await rpc.request({method:'evm_mine',params:[]});
 const migrated=await tx('migrate',addresses.airlock,airlockAbi,'migrate',[tokenAddress]);
 const migrationProof=await verifyMigration(rpc,{tokenAddress,numeraire:USDC,migrationHash:migrated.transactionHash,governanceType:c.governance.type});
 await record('migration-state',migrationProof);
 await rejected('reject-double-migration',addresses.airlock,airlockAbi,'migrate',[tokenAddress],account,unauthorized);
 const proofAfterReplay=await verifyMigration(rpc,{tokenAddress,numeraire:USDC,migrationHash:migrated.transactionHash,governanceType:c.governance.type});
 assert(serialize(proofAfterReplay)===serialize(migrationProof),'Migration evidence changed after replay rejection');
 report.adversarial={prematureMigrationRejected:true,ownershipTheftRejected:true,doubleMigrationRejected:true,contractStateAndBalancesUnchanged:true};
 report.complete=true;await save();
} catch(e){report.error=e.shortMessage??e.message;await save();console.error(report.error);process.exitCode=1;}
