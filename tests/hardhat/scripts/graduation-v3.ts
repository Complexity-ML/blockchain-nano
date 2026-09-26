import {network,artifacts} from 'hardhat';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
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
const reverse=process.env.NANO_TEST_REVERSE_ASSETS==='1';
let wn:any,quote:any;
if(reverse){quote=await deploy(await artifacts.readArtifact('PoolQuoteToken'));wn=await deploy(wrappedArtifact());}
else{wn=await deploy(wrappedArtifact());quote=await deploy(await artifacts.readArtifact('PoolQuoteToken'));}
const factory=await deploy(Factory),library=await deploy(Library);
const wnAddress=await wn.getAddress(),quoteAddress=await quote.getAddress(),factoryAddress=await factory.getAddress(),libraryAddress=await library.getAddress();
let linked=Descriptor.bytecode.slice(2);let links=0;
for(const byName of Object.values(Descriptor.linkReferences) as any[])for(const [name,refs] of Object.entries(byName) as any){assert.equal(name,'NFTDescriptor');for(const ref of refs){assert.equal(ref.length,20);linked=linked.slice(0,ref.start*2)+libraryAddress.slice(2)+linked.slice((ref.start+ref.length)*2);links++;}}
assert.equal(links,1);
const descriptor=await deploy({...Descriptor,bytecode:'0x'+linked},[wnAddress,encodeBytes32String('NANO')]);
const manager=await deploy(Manager,[factoryAddress,wnAddress,await descriptor.getAddress()]),router=await deploy(Router,[factoryAddress,wnAddress]);
const managerAddress=await manager.getAddress(),routerAddress=await router.getAddress();
const wnFirst=BigInt(wnAddress)<BigInt(quoteAddress),token0=wnFirst?wnAddress:quoteAddress,token1=wnFirst?quoteAddress:wnAddress;
let amount0=wnFirst?parseEther('100'):1000000000n,amount1=wnFirst?1000000000n:parseEther('100');
function sqrt(n:bigint){let x=n,y=(x+1n)/2n;while(y<x){x=y;y=(x+n/x)/2n;}return x;}
const initialPrice=sqrt((amount1<<192n)/amount0),fee=3000;
await (await manager.createAndInitializePoolIfNecessary(token0,token1,fee,initialPrice)).wait();
const poolAddress=await factory.getPool(token0,token1,fee);assert.notEqual(poolAddress,ZeroAddress);
const pool=new Contract(poolAddress,Pool.abi,alice),slot=await pool.slot0();assert.equal(slot.sqrtPriceX96,initialPrice);assert.equal(await pool.tickSpacing(),60n);
const center=Math.floor(Number(slot.tick)/60)*60,tickLower=center-600,tickUpper=center+600;
const simulation=JSON.parse(execFileSync('python3',[fileURLToPath(new URL('../../../scripts/simulation/graduation_v3.py',import.meta.url)),
 '--usdc','1','--sqrt-price-x96',initialPrice.toString(),'--quote-index',wnFirst?'1':'0',
 '--tick-lower',String(tickLower),'--tick-upper',String(tickUpper),'--tick-spacing','60'],{encoding:'utf8'}));
const allocation=BigInt(simulation.allocationCRaw);
amount0=wnFirst?allocation:1000000n;amount1=wnFirst?1000000n:allocation;
const latest=await provider.send('eth_getBlockByNumber',['latest',false]),deadline=BigInt(latest.timestamp)+3600n;
await (await wn.deposit({value:parseEther('100')})).wait();await (await wn.approve(managerAddress,parseEther('100'))).wait();await (await quote.approve(managerAddress,1000000000n)).wait();
const before0=wnFirst?await wn.balanceOf(alice.address):await quote.balanceOf(alice.address),before1=wnFirst?await quote.balanceOf(alice.address):await wn.balanceOf(alice.address);
const minted=await (await manager.mint({token0,token1,fee,tickLower,tickUpper,amount0Desired:amount0,amount1Desired:amount1,amount0Min:0,amount1Min:0,recipient:alice.address,deadline})).wait();
const events=(receipt:any)=>receipt.logs.filter((l:any)=>l.address.toLowerCase()===managerAddress.toLowerCase()).map((l:any)=>{try{return manager.interface.parseLog(l);}catch{return null;}}).filter(Boolean);
const mint=events(minted).find((e:any)=>e.name==='IncreaseLiquidity');assert(mint);const tokenId=mint.args.tokenId;
assert.equal(await manager.ownerOf(tokenId),alice.address);const position=await manager.positions(tokenId);assert(position.liquidity>0n);assert.equal(position.liquidity,mint.args.liquidity);assert.equal(position.tickLower,BigInt(tickLower));assert.equal(position.tickUpper,BigInt(tickUpper));assert.equal(await pool.liquidity(),position.liquidity);
assert.equal(before0-(wnFirst?await wn.balanceOf(alice.address):await quote.balanceOf(alice.address)),mint.args.amount0);assert.equal(before1-(wnFirst?await quote.balanceOf(alice.address):await wn.balanceOf(alice.address)),mint.args.amount1);
assert.equal((await pool.ticks(tickLower)).liquidityGross,position.liquidity);assert.equal((await pool.ticks(tickUpper)).liquidityGross,position.liquidity);
assert.equal(position.liquidity,BigInt(simulation.liquidity));
assert.equal(wnFirst?mint.args.amount0:mint.args.amount1,allocation);
const used=wnFirst?mint.args.amount1:mint.args.amount0;
assert.equal(used,BigInt(simulation.usdcUsedRaw));
assert.equal((await pool.slot0()).sqrtPriceX96,initialPrice);
assert.equal((await pool.slot0()).tick,slot.tick);
console.log(JSON.stringify({scenario:'graduation-v3-python-vs-nano',fork:process.env.NANO_TEST_EVM_FORK,
 core:'1.0.1',periphery:'1.4.4',simulation,actual:{liquidity:position.liquidity.toString(),
 allocationRaw:allocation.toString(),quoteUsedRaw:used.toString()},priceAndTickUnchanged:true,syntheticAssets:true}));
await connection.close();
