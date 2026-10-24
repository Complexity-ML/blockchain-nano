// Injected JSONL request callback only; no process, provider, socket or key.
const KIND='nano-in-process-shanghai-v1';
const need=(ok,why)=>{if(!ok)throw Error('Inventory native backend: '+why);};
const lower=x=>String(x).toLowerCase();
const uint=x=>{need(typeof x==='string'&&/^(?:0x[0-9a-f]+|0|[1-9][0-9]*)$/i.test(x),'integer encoding');return BigInt(x);};
const hash=x=>need(typeof x==='string'&&/^0x[0-9a-f]{64}$/.test(x),'actual bytes32');
const addr=x=>{need(typeof x==='string'&&/^0x[0-9a-f]{40}$/.test(x),'address');return x;};
const forbidden=['blockHash','blockNumber','number','status','block_hash','block_number'];
function evidence(r){
 need(r&&r.executionKind===KIND&&r.mined===false&&r.syntheticContext===true&&r.consensusValidated===false,'explicit non-mined native evidence');
 need(forbidden.every(k=>!Object.hasOwn(r,k)),'no mined/synthetic block receipt substitution');
 hash(r.executionStateRoot);return r;
}
export async function createInventoryV2NativeBackend({request,ethers,artifacts,check}){
 need(typeof request==='function'&&typeof check==='function','parent request and deadline callbacks');
 const invoke=async(action,fields={})=>{check();const r=evidence(await request(action,fields));check();return r;};
 const meta=await invoke('metadata');need(meta.evmRevision==='Shanghai'&&uint(meta.chainId)===31337n&&uint(meta.executionStep)===0n,'fresh explicit Shanghai state');
 need(Array.isArray(meta.signers)&&meta.signers.length===3,'public signer fixtures');
 const sender=addr(meta.signers[0]),coinbase=addr(meta.coinbase);
 need(sender==='0x7e5f4552091a69125d5dfcb7b8c2659029395bdf','public fixture key1 only');
 need(coinbase==='0x0000000000000000000000000000000000009999'&&uint(meta.gasPrice)===1n&&uint(meta.baseFee)===0n&&uint(meta.transactionGasLimit)===30000000n&&uint(meta.contextGasLimit)===60000000n,'fixed synthetic fee/context');
 const ifaces=Object.fromEntries(Object.entries(artifacts).map(([n,a])=>[n,new ethers.Interface(a.abi)]));
 let currentRoot=meta.executionStateRoot,currentStep=0n;const transactions=[];let summedGas=0n;
 async function root(){const r=await invoke('root');need(r.executionStateRoot===currentRoot&&uint(r.executionStep)===currentStep&&uint(r.chainId)===31337n,'retained root/step continuity');return r;}
 async function account(where){const a=await invoke('account',{address:addr(lower(where))});need(a.address===lower(where)&&a.executionStateRoot===currentRoot&&uint(a.executionStep)===currentStep,'read-only account root');
  need(typeof a.code==='string'&&/^0x(?:[0-9a-f]{2})*$/.test(a.code)&&a.codeHash===ethers.keccak256(a.code)&&typeof a.exists==='boolean','actual code/hash');uint(a.balance);uint(a.nonce);return a;}
 await root();const initial=await account(sender),initialFees=await account(coinbase);
 need(uint(initial.balance)===1000000000000000000000000n&&uint(initial.nonce)===0n&&initial.code==='0x'&&uint(initialFees.balance)===0n,'fresh public fixture funding, not host funds');
 async function send(to,data,value='0'){
  check();need(transactions.length<80,'eighty native transactions maximum');
  need(typeof data==='string'&&/^0x(?:[0-9a-fA-F]{2})*$/.test(data),'ABI transaction data');
  const amount=uint(value);await root();const before=await account(sender),feeBefore=await account(coinbase),beforeRoot=currentRoot;
  const r=await invoke('send',{from:sender,to:to===null?null:addr(lower(to)),data:lower(data),value:amount.toString(),gas:'30000000'});
  need(typeof r.success==='boolean'&&Array.isArray(r.logs)&&r.stateRootBefore===beforeRoot,'actual engine result/root before');
  hash(r.transactionHash);need(typeof r.rawSignedTransaction==='string'&&/^0x(?:[0-9a-f]{2})+$/.test(r.rawSignedTransaction),'actual signed transaction');
  const tx=ethers.Transaction.from(r.rawSignedTransaction),used=uint(r.gasUsed),price=uint(r.effectiveGasPrice);
  need(tx.hash===r.transactionHash&&ethers.keccak256(r.rawSignedTransaction)===r.transactionHash&&lower(tx.from)===sender,'real signed hash/sender recovery');
  need(tx.type===0&&tx.chainId===31337n&&BigInt(tx.nonce)===uint(before.nonce)&&tx.value===amount&&tx.gasLimit===30000000n&&tx.gasPrice===1n,'signed fee/nonce/value/domain fields');
  need((tx.to===null?null:lower(tx.to))===(to===null?null:lower(to))&&lower(tx.data)===lower(data),'signed target and calldata');
  need(r.from===sender&&(r.to===null?null:lower(r.to))===(to===null?null:lower(to))&&uint(r.value)===amount,'engine request identity');
  need(price===1n&&used>0n&&used<=30000000n&&uint(r.nonceBefore)===uint(before.nonce)&&uint(r.nonceAfter)===uint(before.nonce)+1n,'actual gas and one consumed nonce');
  need(uint(r.senderBalanceBefore)===uint(before.balance)&&uint(r.senderBalanceBefore)-uint(r.senderBalanceAfter)===used*price+(r.success?amount:0n),'actual engine wallet debit');
  need(uint(r.executionStep)===currentStep+1n&&r.executionStateRoot!==beforeRoot,'one execution step and changed actual state root');
  if(!r.success)need(r.logs.length===0&&r.contractAddress===null&&used<30000000n,'expected refusal has no retained effects or full-frame OOG/invalid-opcode substitute');
  currentRoot=r.executionStateRoot;currentStep=uint(r.executionStep);
  await root();const after=await account(sender),feeAfter=await account(coinbase);
  need(uint(after.balance)===uint(r.senderBalanceAfter)&&uint(after.nonce)===uint(r.nonceAfter),'separate actual account readback');
  need(uint(before.balance)-uint(after.balance)===used*price+(r.success?amount:0n)&&uint(feeAfter.balance)-uint(feeBefore.balance)===used*price,'independent wallet and actual coinbase fee checks');
  if(to===null&&r.success)need(lower(r.contractAddress)===lower(ethers.getCreateAddress({from:sender,nonce:tx.nonce})),'actual CREATE address');
  for(const log of r.logs)need(log&&typeof log.address==='string'&&Array.isArray(log.topics)&&typeof log.data==='string'&&!Object.hasOwn(log,'blockHash'),'actual EVM log shape');
  summedGas+=used;transactions.push({transactionHash:r.transactionHash,executionStep:currentStep.toString(),stateRootBefore:beforeRoot,executionStateRoot:currentRoot,
   gasUsed:used.toString(),effectiveGasPrice:price.toString(),senderBalanceBefore:uint(before.balance).toString(),senderBalanceAfter:uint(after.balance).toString(),
   nonceBefore:uint(before.nonce).toString(),nonceAfter:uint(after.nonce).toString(),walletDebitVerified:true,coinbaseFeeVerified:true,
   rawSignedTransaction:r.rawSignedTransaction,success:r.success,value:amount.toString(),mined:false});
  return {...r,gasUsed:used.toString(),effectiveGasPrice:price.toString(),nonceBefore:uint(r.nonceBefore).toString(),nonceAfter:uint(r.nonceAfter).toString(),
   senderBalanceBefore:uint(r.senderBalanceBefore).toString(),senderBalanceAfter:uint(r.senderBalanceAfter).toString(),executionStep:currentStep.toString()};
 }
 return Object.freeze({syntheticPrivateOnly:true,executionEvidenceKind:KIND,
  async deploy(name,args,{value}){need(artifacts[name]?.bytecode,'artifact');const data=artifacts[name].bytecode+ifaces[name].encodeDeploy(args).slice(2),r=await send(null,data,value);need(r.success&&r.contractAddress,'actual native deployment success');return {address:r.contractAddress,receipt:r};},
  async send(where,name,method,args){return await send(where,ifaces[name].encodeFunctionData(method,args),'0');},
  async read(where,name,method,args){await root();const r=await invoke('call',{from:sender,to:addr(lower(where)),data:ifaces[name].encodeFunctionData(method,args),value:'0',gas:'30000000'});
   need(r.success===true&&r.stateDiscarded===true&&r.executionStateRoot===currentRoot&&typeof r.returnData==='string','successful discarded native simulation');await root();return ifaces[name].decodeFunctionResult(method,r.returnData);},
  async code(where){return (await account(where)).code;},
  async balance(where){return uint((await account(where)).balance).toString();},
  async finish(){await root();const end=await account(sender),fees=await account(coinbase);need(currentStep===BigInt(transactions.length)&&uint(end.nonce)===BigInt(transactions.length)&&uint(fees.balance)===summedGas,'whole execution/fee reconciliation');
   return {format:'nano-inventory-v2-native-accounting-v1',executionKind:KIND,transactions:[...transactions],transactionCount:transactions.length,actualStateRoot:currentRoot,
    actualCoinbaseFees:uint(fees.balance).toString(),summedGasUsed:summedGas.toString(),walletDebitChecks:transactions.length,coinbaseFeeChecks:transactions.length,
    mined:false,consensusValidated:false,activatedNetworkForkQualified:false,productionQualified:false};}
 });
}
