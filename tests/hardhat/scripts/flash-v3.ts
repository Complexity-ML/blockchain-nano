import {network,artifacts} from 'hardhat';
import {BrowserProvider,Wallet,ContractFactory,Contract,parseEther,encodeBytes32String,ZeroAddress,AbiCoder} from 'ethers';
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

const borrower=await deploy(await artifacts.readArtifact('DeveloperV3FlashBorrower'),[poolAddress]),borrowerAddress=await borrower.getAddress();
const asset0=wnFirst?wn:quote,asset1=wnFirst?quote:wn;
await (await wn.deposit({value:parseEther('1')})).wait();
await (await wn.transfer(borrowerAddress,parseEther('1'))).wait();
await (await quote.transfer(borrowerAddress,1000000n)).wait();
assert.equal((await pool.slot0()).feeProtocol,0n);
const snapshot=async()=>({pool0:await asset0.balanceOf(poolAddress),pool1:await asset1.balanceOf(poolAddress),borrower0:await asset0.balanceOf(borrowerAddress),borrower1:await asset1.balanceOf(borrowerAddress),growth0:await pool.feeGrowthGlobal0X128(),growth1:await pool.feeGrowthGlobal1X128(),protocol:Array.from(await pool.protocolFees()),liquidity:await pool.liquidity(),slot:Array.from(await pool.slot0()),calls:await borrower.calls(),observed0:await borrower.observed0(),observed1:await borrower.observed1(),nested:await borrower.nestedResult(),nestedSucceeded:await borrower.nestedSucceeded()});
const baseline=await snapshot(),a0=10000n,a1=20000n;
const feeFor=(amount:bigint)=>(amount*BigInt(fee)+999999n)/1000000n;
const f0=feeFor(a0),f1=feeFor(a1);
for(const [label,send] of [
 ['forged-callback',()=>borrower.uniswapV3FlashCallback(0,0,'0x',{gasLimit:1000000})],
 ['no-repayment',()=>borrower.borrow(a0,a1,0,0,false,{gasLimit:1000000})],
 ['short-token0',()=>borrower.borrow(a0,a1,a0+f0-1n,a1+f1,false,{gasLimit:1000000})],
 ['short-token1',()=>borrower.borrow(a0,a1,a0+f0,a1+f1-1n,false,{gasLimit:1000000})]
] as const){failedReceipts.push(await failedReceipt(provider,label,send));assert.deepEqual(await snapshot(),baseline,label+' rollback');}
const successes=[];
let cumulative0=0n,cumulative1=0n,growth0=baseline.growth0,growth1=baseline.growth1;
for(const [amount0,amount1,reenter] of [[a0,a1,true],[1n,1n,false]] as const){
 const before=await snapshot(),fee0=feeFor(amount0),fee1=feeFor(amount1);
 const receipt=await (await borrower.borrow(amount0,amount1,amount0+fee0,amount1+fee1,reenter)).wait();assert.equal(receipt.status,1);
 const event=receipt.logs.filter((l:any)=>l.address.toLowerCase()===poolAddress.toLowerCase()).map((l:any)=>{try{return pool.interface.parseLog(l);}catch{return null;}}).find((e:any)=>e?.name==='Flash');assert(event);
 assert.equal(event.args.sender,borrowerAddress);assert.equal(event.args.recipient,borrowerAddress);
 assert.equal(event.args.amount0,amount0);assert.equal(event.args.amount1,amount1);assert.equal(event.args.paid0,fee0);assert.equal(event.args.paid1,fee1);
 cumulative0+=fee0;cumulative1+=fee1;growth0+=(fee0<<128n)/position.liquidity;growth1+=(fee1<<128n)/position.liquidity;
 const after=await snapshot();assert.equal(after.pool0,baseline.pool0+cumulative0);assert.equal(after.pool1,baseline.pool1+cumulative1);
 assert.equal(after.borrower0,baseline.borrower0-cumulative0);assert.equal(after.borrower1,baseline.borrower1-cumulative1);
 assert.equal(after.growth0,growth0);assert.equal(after.growth1,growth1);assert.equal(after.calls,before.calls+1n);
 assert.equal(after.observed0,before.borrower0+amount0);assert.equal(after.observed1,before.borrower1+amount1);
 assert.equal(after.liquidity,baseline.liquidity);assert.deepEqual(after.slot,baseline.slot);
 if(reenter){assert.equal(after.nestedSucceeded,false);assert.equal(after.nested.slice(0,10),'0x08c379a0');assert.equal(AbiCoder.defaultAbiCoder().decode(['string'],'0x'+after.nested.slice(10))[0],'LOK');}
 successes.push({amount0,amount1,fee0,fee1,transactionHash:receipt.hash});
}

// Separate protocol/LP accounting, including a donation with no borrowed assets.
for(const [label,send] of [
 ['protocol-fee-not-owner',()=>(pool.connect(bob) as any).setFeeProtocol(4,5,{gasLimit:300000})],
 ['protocol-fee-too-small',()=>pool.setFeeProtocol(3,5,{gasLimit:300000})],
 ['protocol-fee-too-large',()=>pool.setFeeProtocol(4,11,{gasLimit:300000})],
] as const){const before=await snapshot();failedReceipts.push(await failedReceipt(provider,label,send));assert.deepEqual(await snapshot(),before,label+' rollback');}
await (await pool.setFeeProtocol(4,5)).wait();assert.equal((await pool.slot0()).feeProtocol,84n);
const protocolSuccesses=[];
for(const [amount0,amount1,donation0,donation1] of [[a0,a1,0n,0n],[0n,0n,17n,23n],[1n,0n,0n,0n],[0n,1n,0n,0n]] as const){
 const before=await snapshot(),paid0=feeFor(amount0)+donation0,paid1=feeFor(amount1)+donation1;
 const protocol0=paid0/4n,protocol1=paid1/5n;
 const receipt=await (await borrower.borrow(amount0,amount1,amount0+paid0,amount1+paid1,false)).wait();assert.equal(receipt.status,1);
 const after=await snapshot();
 assert.equal(after.pool0,before.pool0+paid0);assert.equal(after.pool1,before.pool1+paid1);
 assert.equal(after.borrower0,before.borrower0-paid0);assert.equal(after.borrower1,before.borrower1-paid1);
 assert.deepEqual(after.protocol,[before.protocol[0]+protocol0,before.protocol[1]+protocol1]);
 assert.equal(after.growth0,before.growth0+((paid0-protocol0)<<128n)/position.liquidity);
 assert.equal(after.growth1,before.growth1+((paid1-protocol1)<<128n)/position.liquidity);
 assert.equal(after.calls,before.calls+1n);assert.equal(after.observed0,before.borrower0+amount0);assert.equal(after.observed1,before.borrower1+amount1);
 assert.equal(after.liquidity,before.liquidity);assert.deepEqual(after.slot,before.slot);
 const logs=receipt.logs.filter((l:any)=>l.address.toLowerCase()===poolAddress.toLowerCase()).map((l:any)=>pool.interface.parseLog(l));
 const event=logs.find((e:any)=>e?.name==='Flash');assert(event);assert.equal(event.args.paid0,paid0);assert.equal(event.args.paid1,paid1);
 assert.equal(event.args.amount0,amount0);assert.equal(event.args.amount1,amount1);
 protocolSuccesses.push({amount0,amount1,donation0,donation1,paid0,paid1,protocol0,protocol1,hash:receipt.hash});
}
const beforeCollect=await snapshot();
failedReceipts.push(await failedReceipt(provider,'protocol-collect-not-owner',()=>(pool.connect(bob) as any).collectProtocol(bob.address,(1n<<128n)-1n,(1n<<128n)-1n,{gasLimit:300000})));
assert.deepEqual(await snapshot(),beforeCollect,'unauthorized collection rollback');
const owner0=await asset0.balanceOf(alice.address),owner1=await asset1.balanceOf(alice.address);
const collectedProtocol=await (await pool.collectProtocol(alice.address,(1n<<128n)-1n,(1n<<128n)-1n)).wait();assert.equal(collectedProtocol.status,1);
const taken0=beforeCollect.protocol[0]-1n,taken1=beforeCollect.protocol[1]-1n;assert(taken0>0n&&taken1>0n);
const afterCollect=await snapshot();assert.deepEqual(afterCollect.protocol,[1n,1n]);
assert.equal(afterCollect.pool0,beforeCollect.pool0-taken0);assert.equal(afterCollect.pool1,beforeCollect.pool1-taken1);
assert.equal(await asset0.balanceOf(alice.address),owner0+taken0);assert.equal(await asset1.balanceOf(alice.address),owner1+taken1);
assert.equal(afterCollect.growth0,beforeCollect.growth0);assert.equal(afterCollect.growth1,beforeCollect.growth1);
assert.equal(afterCollect.borrower0,beforeCollect.borrower0);assert.equal(afterCollect.borrower1,beforeCollect.borrower1);
assert.equal(afterCollect.calls,beforeCollect.calls);assert.deepEqual(afterCollect.slot,beforeCollect.slot);
const collectEvent=collectedProtocol.logs.filter((l:any)=>l.address.toLowerCase()===poolAddress.toLowerCase()).map((l:any)=>pool.interface.parseLog(l)).find((e:any)=>e?.name==='CollectProtocol');
assert(collectEvent);assert.equal(collectEvent.args.amount0,taken0);assert.equal(collectEvent.args.amount1,taken1);assert.equal(collectEvent.args.recipient,alice.address);
await (await pool.collectProtocol(alice.address,0,0)).wait();assert.deepEqual(await snapshot(),afterCollect,'zero collection preserves pool and callback state');
// Price and active ticks never changed: inside growth equals global growth.
const beforeLP=await snapshot(),lpPosition=await manager.positions(tokenId);
const expectedLP0=((beforeLP.growth0-lpPosition.feeGrowthInside0LastX128)*lpPosition.liquidity)>>128n;
const expectedLP1=((beforeLP.growth1-lpPosition.feeGrowthInside1LastX128)*lpPosition.liquidity)>>128n;
assert(expectedLP0>0n&&expectedLP1>0n);assert.equal(lpPosition.tokensOwed0,0n);assert.equal(lpPosition.tokensOwed1,0n);
const collectLP={tokenId,recipient:alice.address,amount0Max:(1n<<128n)-1n,amount1Max:(1n<<128n)-1n};
const lpCollections=[];
for(const [expected0,expected1] of [[expectedLP0,expectedLP1],[0n,0n]]){
 const before=await snapshot(),wallet0=await asset0.balanceOf(alice.address),wallet1=await asset1.balanceOf(alice.address);
 const payload=await manager.collect.populateTransaction(collectLP);
 const gas=BigInt(await provider.send('eth_estimateGas',[{from:alice.address,to:managerAddress,data:payload.data},'pending']));
 const receipt=await (await manager.collect(collectLP,{gasLimit:gas})).wait();assert.equal(receipt.status,1);
 const event=events(receipt).find((e:any)=>e.name==='Collect');assert(event);assert.equal(event.args.tokenId,tokenId);
 assert.equal(event.args.amount0,expected0);assert.equal(event.args.amount1,expected1);
 assert.equal(await asset0.balanceOf(alice.address),wallet0+expected0);assert.equal(await asset1.balanceOf(alice.address),wallet1+expected1);
 const after=await snapshot();assert.equal(after.pool0,before.pool0-expected0);assert.equal(after.pool1,before.pool1-expected1);
 assert.equal(after.growth0,before.growth0);assert.equal(after.growth1,before.growth1);assert.deepEqual(after.protocol,[1n,1n]);
 assert.equal(after.borrower0,before.borrower0);assert.equal(after.borrower1,before.borrower1);assert.equal(after.calls,before.calls);
 assert.equal(after.liquidity,before.liquidity);assert.deepEqual(after.slot,before.slot);
 const updated=await manager.positions(tokenId);assert.equal(updated.liquidity,position.liquidity);
 assert.equal(updated.tokensOwed0,0n);assert.equal(updated.tokensOwed1,0n);
 assert.equal(updated.feeGrowthInside0LastX128,beforeLP.growth0);assert.equal(updated.feeGrowthInside1LastX128,beforeLP.growth1);
 assert.equal(await manager.ownerOf(tokenId),alice.address);
 lpCollections.push({expected0,expected1,estimatedGas:gas,gasUsed:receipt.gasUsed,hash:receipt.hash});
}
const certified=await provider.send('nano_networkBlocks',['0x0']);assert(certified.blocks.length>0&&certified.blocks.every((b:any)=>b.bftCertificate?.votes.length>0));
assert.equal(feeFor(1n),1n);
console.log(JSON.stringify({scenario:'hardhat-v3-flash',fork:process.env.NANO_TEST_EVM_FORK,failedReceipts,successes,protocolSuccesses,lpCollections,protocolCollected:{amount0:taken0,amount1:taken1,residual0:1n,residual1:1n},certifiedBlocks:true,rollbackBothAssets:true,reentrancyError:'LOK',exactFeeGrowth:true,syntheticQuoteAsset:true},(_,v)=>typeof v==='bigint'?v.toString():v));
await connection.close();
