import {network,artifacts} from 'hardhat';
import {BrowserProvider,Wallet,ContractFactory,Contract,parseEther,encodeBytes32String,ZeroAddress} from 'ethers';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
import {wrappedArtifact} from './helpers/wrapped-artifact.ts';
import {failedReceipt} from './helpers/failed-receipt.ts';
const require=createRequire(import.meta.url);
const Factory=require('@uniswap/v3-core/artifacts/contracts/UniswapV3Factory.sol/UniswapV3Factory.json');
const Pool=require('@uniswap/v3-core/artifacts/contracts/UniswapV3Pool.sol/UniswapV3Pool.json');
const Router=require('@uniswap/v3-periphery/artifacts/contracts/SwapRouter.sol/SwapRouter.json');
const Manager=require('@uniswap/v3-periphery/artifacts/contracts/NonfungiblePositionManager.sol/NonfungiblePositionManager.json');
const Descriptor=require('@uniswap/v3-periphery/artifacts/contracts/NonfungibleTokenPositionDescriptor.sol/NonfungibleTokenPositionDescriptor.json');
const Library=require('@uniswap/v3-periphery/artifacts/contracts/libraries/NFTDescriptor.sol/NFTDescriptor.json');
const connection=await network.create('nano'),provider=new BrowserProvider(connection.provider);provider.pollingInterval=100;
const alice=new Wallet('0x'+'1'.padStart(64,'0'),provider),bob=new Wallet('0x'+'2'.padStart(64,'0'),provider),failedReceipts=[];
const deploy=async(a:any,args:any[]=[])=>{const c=await new ContractFactory(a.abi,a.bytecode,alice).deploy(...args);await c.waitForDeployment();return c;};
const wn=await deploy(wrappedArtifact()),quote=await deploy(await artifacts.readArtifact('PoolQuoteToken')),factory=await deploy(Factory),library=await deploy(Library);
const wnAddress=await wn.getAddress(),quoteAddress=await quote.getAddress(),factoryAddress=await factory.getAddress(),libraryAddress=await library.getAddress();
let linked=Descriptor.bytecode.slice(2);let links=0;
for(const byName of Object.values(Descriptor.linkReferences) as any[])for(const [name,refs] of Object.entries(byName) as any){assert.equal(name,'NFTDescriptor');for(const ref of refs){assert.equal(ref.length,20);linked=linked.slice(0,ref.start*2)+libraryAddress.slice(2)+linked.slice((ref.start+ref.length)*2);links++;}}
assert.equal(links,1);
const descriptor=await deploy({...Descriptor,bytecode:'0x'+linked},[wnAddress,encodeBytes32String('NANO')]);
const manager=await deploy(Manager,[factoryAddress,wnAddress,await descriptor.getAddress()]),router=await deploy(Router,[factoryAddress,wnAddress]);
const managerAddress=await manager.getAddress(),routerAddress=await router.getAddress();
const wnFirst=BigInt(wnAddress)<BigInt(quoteAddress),token0=wnFirst?wnAddress:quoteAddress,token1=wnFirst?quoteAddress:wnAddress;
const amount0=wnFirst?parseEther('100'):1000000000n,amount1=wnFirst?1000000000n:parseEther('100');
function sqrt(n:bigint){let x=n,y=(x+1n)/2n;while(y<x){x=y;y=(x+n/x)/2n;}return x;}
const initialPrice=sqrt((amount1<<192n)/amount0),fee=3000;
await (await manager.createAndInitializePoolIfNecessary(token0,token1,fee,initialPrice)).wait();
const poolAddress=await factory.getPool(token0,token1,fee);assert.notEqual(poolAddress,ZeroAddress);
const pool=new Contract(poolAddress,Pool.abi,alice),slot=await pool.slot0();assert.equal(slot.sqrtPriceX96,initialPrice);assert.equal(await pool.tickSpacing(),60n);
const center=Math.floor(Number(slot.tick)/60)*60,tickLower=center-600,tickUpper=center+600;
const latest=await provider.send('eth_getBlockByNumber',['latest',false]),deadline=BigInt(latest.timestamp)+3600n;
await (await wn.deposit({value:parseEther('100')})).wait();await (await wn.approve(managerAddress,parseEther('100'))).wait();await (await quote.approve(managerAddress,1000000000n)).wait();
const before0=wnFirst?await wn.balanceOf(alice.address):await quote.balanceOf(alice.address),before1=wnFirst?await quote.balanceOf(alice.address):await wn.balanceOf(alice.address);
const minted=await (await manager.mint({token0,token1,fee,tickLower,tickUpper,amount0Desired:amount0,amount1Desired:amount1,amount0Min:0,amount1Min:0,recipient:alice.address,deadline})).wait();
const events=(receipt:any)=>receipt.logs.filter((l:any)=>l.address.toLowerCase()===managerAddress.toLowerCase()).map((l:any)=>{try{return manager.interface.parseLog(l);}catch{return null;}}).filter(Boolean);
const mint=events(minted).find((e:any)=>e.name==='IncreaseLiquidity');assert(mint);const tokenId=mint.args.tokenId;
assert.equal(await manager.ownerOf(tokenId),alice.address);const position=await manager.positions(tokenId);assert(position.liquidity>0n);assert.equal(position.liquidity,mint.args.liquidity);assert.equal(position.tickLower,BigInt(tickLower));assert.equal(position.tickUpper,BigInt(tickUpper));assert.equal(await pool.liquidity(),position.liquidity);
assert.equal(before0-(wnFirst?await wn.balanceOf(alice.address):await quote.balanceOf(alice.address)),mint.args.amount0);assert.equal(before1-(wnFirst?await quote.balanceOf(alice.address):await wn.balanceOf(alice.address)),mint.args.amount1);
assert.equal((await pool.ticks(tickLower)).liquidityGross,position.liquidity);assert.equal((await pool.ticks(tickUpper)).liquidityGross,position.liquidity);

const asset0=wnFirst?wn:quote,asset1=wnFirst?quote:wn;
await (await wn.deposit({value:parseEther('1100')})).wait();
await (await wn.approve(managerAddress,parseEther('100'))).wait();
await (await quote.approve(managerAddress,1000000000n)).wait();
const wideLower=center-1200,wideUpper=center+1200;
const wideReceipt=await (await manager.mint({token0,token1,fee,tickLower:wideLower,tickUpper:wideUpper,amount0Desired:amount0,amount1Desired:amount1,amount0Min:0,amount1Min:0,recipient:alice.address,deadline})).wait();
const wideMint=events(wideReceipt).find((e:any)=>e.name==='IncreaseLiquidity');assert(wideMint);
const wideId=wideMint.args.tokenId,wide=await manager.positions(wideId);assert(wide.liquidity>0n);
const combined=position.liquidity+wide.liquidity;assert.equal(await pool.liquidity(),combined);
for(const [tick,liquidity,sign] of [[tickLower,position.liquidity,1n],[tickUpper,position.liquidity,-1n],[wideLower,wide.liquidity,1n],[wideUpper,wide.liquidity,-1n]] as const){const t=await pool.ticks(tick);assert.equal(t.liquidityGross,liquidity);assert.equal(t.liquidityNet,liquidity*sign);}
await (await wn.approve(routerAddress,parseEther('1000'))).wait();
await (await quote.approve(routerAddress,10000000000n)).wait();
await (await pool.increaseObservationCardinalityNext(8)).wait();
const snapshot=async()=>({pool0:await asset0.balanceOf(poolAddress),pool1:await asset1.balanceOf(poolAddress),user0:await asset0.balanceOf(alice.address),user1:await asset1.balanceOf(alice.address),growth0:await pool.feeGrowthGlobal0X128(),growth1:await pool.feeGrowthGlobal1X128(),slot:Array.from(await pool.slot0()),liquidity:await pool.liquidity(),ticks:await Promise.all([wideLower,tickLower,tickUpper,wideUpper].map(async t=>Array.from(await pool.ticks(t)))),positions:[Array.from(await manager.positions(tokenId)),Array.from(await manager.positions(wideId))],observations:await Promise.all(Array.from({length:8},(_,i)=>pool.observations(i).then((o:any)=>Array.from(o))))});
const swapParams=(quoteIn:boolean,limit:bigint)=>({tokenIn:quoteIn?quoteAddress:wnAddress,tokenOut:quoteIn?wnAddress:quoteAddress,fee,recipient:alice.address,deadline,amountIn:quoteIn?10000000000n:parseEther('1000'),amountOutMinimum:0,sqrtPriceLimitX96:limit});
const firstLimit=initialPrice*(wnFirst?104n:96n)/100n,secondLimit=initialPrice*(wnFirst?96n:104n)/100n;
const beforeFailure=await snapshot();
failedReceipts.push(await failedReceipt(provider,'crossing-slippage-rollback',()=>router.exactInputSingle({...swapParams(true,firstLimit),amountOutMinimum:(1n<<256n)-1n},{gasLimit:2000000})));
assert.deepEqual(await snapshot(),beforeFailure);
const pendingGas=async(method:any,args:any[])=>{
 const tx=await method.populateTransaction(...args);
 return BigInt(await provider.send('eth_estimateGas',[{from:alice.address,to:tx.to,data:tx.data},'pending']));
};
// Read the fee accumulators and calculate each NFT's entitlement off-chain.
const positionFees=async(id:bigint)=>{
 const p=await manager.positions(id),current=(await pool.slot0()).tick;
 const lower=await pool.ticks(p.tickLower),upper=await pool.ticks(p.tickUpper);
 const globals=[await pool.feeGrowthGlobal0X128(),await pool.feeGrowthGlobal1X128()];
 const outsideLower=[lower.feeGrowthOutside0X128,lower.feeGrowthOutside1X128],outsideUpper=[upper.feeGrowthOutside0X128,upper.feeGrowthOutside1X128];
 const last=[p.feeGrowthInside0LastX128,p.feeGrowthInside1LastX128],owed=[p.tokensOwed0,p.tokensOwed1];
 const inside=globals.map((global,i)=>{
  const below=current>=p.tickLower?outsideLower[i]:BigInt.asUintN(256,global-outsideLower[i]);
  const above=current<p.tickUpper?outsideUpper[i]:BigInt.asUintN(256,global-outsideUpper[i]);
  return BigInt.asUintN(256,global-below-above);
 });
 const fees=inside.map((growth,i)=>owed[i]+((BigInt.asUintN(256,growth-last[i])*p.liquidity)>>128n));
 return {id,inside,fees,liquidity:p.liquidity,tick:current,lower:p.tickLower,upper:p.tickUpper};
};
const swaps=[],oracleChecks=[],feeAccrualChecks=[];let lastSwapBlock=0;
const outsideLimit=firstLimit*(wnFirst?1001n:999n)/1000n;
for(const [quoteIn,limit,expectedLiquidity] of [[true,firstLimit,wide.liquidity],[true,outsideLimit,wide.liquidity],[false,secondLimit,wide.liquidity],[true,initialPrice,combined]] as const){
 // One intervening block ensures a genuine interior oracle query later.
 if(swaps.length===1)await (await alice.sendTransaction({to:bob.address,value:1n})).wait();
 const before=await snapshot(),params=swapParams(quoteIn,limit);
 const feesBefore=await Promise.all([tokenId,wideId].map(positionFees));
 const observationBefore=await pool.observations(before.slot[2]);
 const estimate=await router.exactInputSingle.estimateGas(params);
 const pendingEstimate=await pendingGas(router.exactInputSingle,[params]);
 const tx=await router.exactInputSingle(params,{gasLimit:pendingEstimate});
 let receipt;
 try {receipt=await tx.wait();}catch(error){console.log(JSON.stringify({diagnostic:'v3-crossing-failure',swapIndex:swaps.length,params,gasLimit:tx.gasLimit,rawTransaction:await provider.send('eth_getTransactionByHash',[tx.hash]),receipt:await provider.send('eth_getTransactionReceipt',[tx.hash]),retryEstimate:await router.exactInputSingle.estimateGas(params)},(_,v)=>typeof v==='bigint'?v.toString():v));throw error;}
 assert.equal(receipt.status,1);
 console.log(JSON.stringify({progress:'v3-crossing-swap',index:swaps.length,estimate,pendingEstimate,gasLimit:tx.gasLimit,gasUsed:receipt.gasUsed},(_,v)=>typeof v==='bigint'?v.toString():v));
 const swap=receipt.logs.filter((l:any)=>l.address.toLowerCase()===poolAddress.toLowerCase()).map((l:any)=>{try{return pool.interface.parseLog(l);}catch{return null;}}).find((e:any)=>e?.name==='Swap');assert(swap);
 const after=await snapshot(),amountIn=quoteIn?10000000000n:parseEther('1000'),input0=quoteIn?!wnFirst:wnFirst;
 assert.equal(after.slot[0],limit);assert.equal(after.liquidity,expectedLiquidity);assert.equal(swap.args.sqrtPriceX96,limit);assert.equal(swap.args.liquidity,expectedLiquidity);assert.equal(swap.args.tick,after.slot[1]);
 assert.equal(after.pool0-before.pool0,swap.args.amount0);assert.equal(after.pool1-before.pool1,swap.args.amount1);
 assert.equal(before.user0-after.user0,swap.args.amount0);assert.equal(before.user1-after.user1,swap.args.amount1);
 const consumed=input0?swap.args.amount0:swap.args.amount1,output=-(input0?swap.args.amount1:swap.args.amount0);assert(consumed>0n&&consumed<amountIn);assert(output>0n);
 assert.equal(await asset0.balanceOf(routerAddress),0n);assert.equal(await asset1.balanceOf(routerAddress),0n);
 const tick=Number(after.slot[1]);assert(tick>wideLower&&tick<wideUpper);
 if(expectedLiquidity===combined)assert(tick>=tickLower&&tick<tickUpper);else assert(tick<tickLower||tick>=tickUpper);
 assert(input0?after.growth0>before.growth0:after.growth1>before.growth1);
 assert.equal((await manager.positions(tokenId)).liquidity,position.liquidity);assert.equal((await manager.positions(wideId)).liquidity,wide.liquidity);
 const feesAfter=await Promise.all([tokenId,wideId].map(positionFees));
 for(let i=0;i<2;i++)for(let asset=0;asset<2;asset++)assert(feesAfter[i].fees[asset]>=feesBefore[i].fees[asset],'accrued fees cannot decrease');
 if(swaps.length===1){
  assert(before.slot[1]<BigInt(tickLower)||before.slot[1]>=BigInt(tickUpper));
  assert.deepEqual(feesAfter[0].inside,feesBefore[0].inside,'out-of-range position earns no new fee growth');
  assert.deepEqual(feesAfter[0].fees,feesBefore[0].fees,'out-of-range position earns no fees');
  assert(feesAfter[1].fees[input0?0:1]>feesBefore[1].fees[input0?0:1],'active wide position earns input-asset fees');
 }
 feeAccrualChecks.push({swapIndex:swaps.length,before:feesBefore,after:feesAfter});
 const included=await provider.send('eth_getBlockByNumber',[receipt.blockNumber.toString(16).replace(/^/,'0x'),false]);
 const timestamp=BigInt(included.timestamp),observation=await pool.observations(after.slot[2]);
 const elapsed=BigInt.asUintN(32,timestamp-observationBefore.blockTimestamp);
 const expectedTick=BigInt.asIntN(56,observationBefore.tickCumulative+before.slot[1]*elapsed);
 const expectedSeconds=BigInt.asUintN(160,observationBefore.secondsPerLiquidityCumulativeX128+(elapsed<<128n)/(before.liquidity||1n));
 assert.equal(observation.blockTimestamp,BigInt.asUintN(32,timestamp));assert.equal(observation.initialized,true);
 assert.equal(observation.tickCumulative,expectedTick,'oracle tick cumulative');assert.equal(observation.secondsPerLiquidityCumulativeX128,expectedSeconds,'oracle seconds/liquidity cumulative');
 assert.equal(after.slot[3],8n,'oracle cardinality grows on write');
 oracleChecks.push({timestamp,tickCumulative:expectedTick,secondsPerLiquidityCumulativeX128:expectedSeconds,elapsed,priorTick:before.slot[1],priorLiquidity:before.liquidity});lastSwapBlock=receipt.blockNumber;
 swaps.push({fromTick:before.slot[1],toTick:after.slot[1],price:limit,liquidity:expectedLiquidity,consumed,output,input0,estimate,pendingEstimate,gasLimit:tx.gasLimit,gasUsed:receipt.gasUsed,hash:receipt.hash});
}
const [leftObservation,rightObservation]=oracleChecks;
const span=rightObservation.timestamp-leftObservation.timestamp;assert(span>=2n,'oracle interpolation requires an interior timestamp');
const target=leftObservation.timestamp+span/2n,offset=target-leftObservation.timestamp;
const queryTimes=[leftObservation.timestamp,target,rightObservation.timestamp,oracleChecks.at(-1)!.timestamp];
const observed=await pool.observe.staticCall(queryTimes.map(t=>oracleChecks.at(-1)!.timestamp-t),{blockTag:lastSwapBlock});
const interpolatedTick=leftObservation.tickCumulative+(rightObservation.tickCumulative-leftObservation.tickCumulative)/span*offset;
const interpolatedSeconds=leftObservation.secondsPerLiquidityCumulativeX128+(rightObservation.secondsPerLiquidityCumulativeX128-leftObservation.secondsPerLiquidityCumulativeX128)*offset/span;
assert.deepEqual(Array.from(observed[0]),[leftObservation.tickCumulative,interpolatedTick,rightObservation.tickCumulative,oracleChecks.at(-1)!.tickCumulative]);
assert.deepEqual(Array.from(observed[1]),[leftObservation.secondsPerLiquidityCumulativeX128,interpolatedSeconds,rightObservation.secondsPerLiquidityCumulativeX128,oracleChecks.at(-1)!.secondsPerLiquidityCumulativeX128]);
assert.equal((await pool.slot0()).tick,slot.tick);
const collected=[];
for(const [id,liquidity] of [[tokenId,position.liquidity],[wideId,wide.liquidity]] as const){
 const expectedFees=await positionFees(id);
 const removal={tokenId:id,liquidity,amount0Min:0,amount1Min:0,deadline};
 const removed=await (await manager.decreaseLiquidity(removal,{gasLimit:await pendingGas(manager.decreaseLiquidity,[removal])})).wait();
 const principal=events(removed).find((e:any)=>e.name==='DecreaseLiquidity');assert(principal);
 const before=await snapshot(),collection={tokenId:id,recipient:alice.address,amount0Max:(1n<<128n)-1n,amount1Max:(1n<<128n)-1n};const receipt=await (await manager.collect(collection,{gasLimit:await pendingGas(manager.collect,[collection])})).wait();
 const event=events(receipt).find((e:any)=>e.name==='Collect');assert(event);const after=await snapshot();
 assert.equal(after.user0-before.user0,event.args.amount0);assert.equal(after.user1-before.user1,event.args.amount1);
 assert.equal(before.pool0-after.pool0,event.args.amount0);assert.equal(before.pool1-after.pool1,event.args.amount1);
 const earned0=event.args.amount0-principal.args.amount0,earned1=event.args.amount1-principal.args.amount1;assert(earned0>0n&&earned1>0n);assert.equal(earned0,expectedFees.fees[0]);assert.equal(earned1,expectedFees.fees[1]);
 const pos=await manager.positions(id);assert.equal(pos.tokensOwed0,0n);assert.equal(pos.tokensOwed1,0n);
 collected.push({id,earned0,earned1,expectedFees});
}
for(const id of [tokenId,wideId])await (await manager.burn(id)).wait();
assert.equal(await pool.liquidity(),0n);assert.equal(await manager.balanceOf(alice.address),0n);
for(const tick of [wideLower,tickLower,tickUpper,wideUpper]){const t=await pool.ticks(tick);assert.equal(t.liquidityGross,0n);assert.equal(t.liquidityNet,0n);assert.equal(t.initialized,false);}
console.log(JSON.stringify({scenario:'hardhat-v3-crossing',fork:process.env.NANO_TEST_EVM_FORK,ticks:[wideLower,tickLower,tickUpper,wideUpper],swaps,collected,feeAccrualChecks,oracleChecks,oracleInterpolation:{target,span,interpolatedTick,interpolatedSeconds,blockNumber:lastSwapBlock},failedReceipts,allPositionsBurned:true,syntheticQuoteAsset:true},(_,v)=>typeof v==='bigint'?v.toString():v));
await connection.close();
