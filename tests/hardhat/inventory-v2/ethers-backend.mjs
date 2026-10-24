// Optional adapter for an ALREADY CREATED and explicitly approved private
// provider/signer. It creates no provider, URL, wallet, keys or process itself.
// The caller owns external process isolation, deadline and final cleanup.
export async function createInventoryV2EthersBackend({provider,signer,ethers,artifacts,
  approvedSyntheticChainId,gasLimit,check}) {
  const need=(ok,why)=>{if(!ok)throw Error('Inventory V2 injected backend: '+why);};
  need(typeof check==='function','caller deadline/check function');check();
  need(provider && signer && signer.provider===provider,'one explicitly injected provider');
  need(typeof approvedSyntheticChainId==='bigint' && approvedSyntheticChainId>0n,'explicit private chain id');
  // This demonstration must never be used on Base mainnet/Sepolia or Ethereum.
  need(![1n,8453n,84532n,11155111n].includes(approvedSyntheticChainId),'public deployment excluded');
  need(typeof gasLimit==='bigint' && gasLimit>0n && gasLimit<=60_000_000n,'explicit per-transaction cap');
  const network=await provider.getNetwork();check();need(network.chainId===approvedSyntheticChainId,'approved chain id observed');
  const sender=(await signer.getAddress()).toLowerCase();check();
  const interfaces=Object.fromEntries(Object.entries(artifacts).map(([n,a])=>[n,new ethers.Interface(a.abi)]));
  async function fence(){check();const n=await provider.getNetwork();check();need(n.chainId===approvedSyntheticChainId,'chain changed');}
  function normalize(r){
    need(r && (r.status===0 || r.status===1) && r.hash && r.blockHash && Array.isArray(r.logs),'actual mined receipt');
    need(typeof r.gasUsed==='bigint' && typeof r.gasPrice==='bigint','actual receipt gas accounting');
    need(r.from.toLowerCase()===sender,'same approved sender');
    return {success:r.status===1,transactionHash:r.hash,blockHash:r.blockHash,
      gasUsed:r.gasUsed.toString(),effectiveGasPrice:r.gasPrice.toString(),from:r.from.toLowerCase(),
      logs:r.logs.map(l=>({address:l.address,topics:[...l.topics],data:l.data}))};
  }
  async function mined(tx){
    // Do not turn RPC/estimate/transport failures into an expected refusal.
    let r;try{r=await tx.wait(1);}catch(e){if(!e?.receipt)throw e;r=e.receipt;}
    check();return normalize(r);
  }
  return Object.freeze({syntheticPrivateOnly:true,
    async deploy(name,args,{value}){
      await fence();const factory=new ethers.ContractFactory(artifacts[name].abi,artifacts[name].bytecode,signer);
      const request=await factory.getDeployTransaction(...args,{value:BigInt(value),gasLimit});check();
      // Explicit limit deliberately bypasses estimateGas: a failed assertion
      // must yield its real status-0 receipt rather than a synthetic error.
      const tx=await signer.sendTransaction(request);check();const receipt=await mined(tx);
      need(receipt.success,'deployment receipt failed');
      return {address:ethers.getCreateAddress({from:sender,nonce:tx.nonce}),receipt};
    },
    async send(where,name,method,args){
      await fence();const data=interfaces[name].encodeFunctionData(method,args);
      const tx=await signer.sendTransaction({to:where,data,gasLimit,value:0n});check();return await mined(tx);
    },
    async read(where,name,method,args){
      await fence();const data=interfaces[name].encodeFunctionData(method,args);
      const result=await provider.call({to:where,data});check();
      return interfaces[name].decodeFunctionResult(method,result);
    },
    async code(where){await fence();const code=await provider.getCode(where);check();return code;},
    async balance(where){await fence();const n=await provider.getBalance(where);check();return n.toString();}
  });
}
