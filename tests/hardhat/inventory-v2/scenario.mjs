// Injected private execution ONLY. No RPC URL, wallet, filesystem, process,
// installation, default chain or test result is created by importing this file.
// The parent must bind the actual backend, compiler closure and chain isolation.
export async function runInventoryV2Scenario({backend, artifacts, ethers, createInventoryModel}) {
  const need = (ok, why) => { if (!ok) throw Error('Inventory V2 scenario: ' + why); };
  const names = ['InventoryV2CasesA', 'InventoryV2CasesB', 'InventoryV2TermsCases', 'InventoryV2MultiClientCases', 'InventoryV2Fixture',
    'InventoryV2OpenBase', 'InventoryV2OpenNative', 'InventoryV2TokenMock'];
  for (const method of ['deploy','read','send','code','balance']) need(typeof backend?.[method] === 'function', 'backend ' + method);
  need(typeof createInventoryModel === 'function', 'independent oracle required');
  need(backend.syntheticPrivateOnly === true, 'explicit injected private backend');
  for (const name of names) need(Array.isArray(artifacts?.[name]?.abi), 'artifact ' + name);
  const interfaces = Object.fromEntries(names.map(name => [name, new ethers.Interface(artifacts[name].abi)]));
  const lower = value => String(value).toLowerCase();
  const address = value => { need(/^0x[0-9a-fA-F]{40}$/.test(value), 'address'); return lower(value); };
  const uint = value => { const s = String(value); need(/^(?:0|[1-9][0-9]*)$/.test(s), 'atomic decimal'); return BigInt(s); };
  const scalar = value => Array.isArray(value) || (value && Object.hasOwn(value,'0')) ? value[0] : value;
  const read = async (where, name, method, args = []) => scalar(await backend.read(where,name,method,args));
  const abi = ethers.AbiCoder.defaultAbiCoder();
  const fields = ['kind','routeId','termsHash','sourceChainId','sourceDomain','sourceEndpoint',
    'destinationChainId','destinationDomain','destinationEndpoint','nonce','transferId','sender',
    'recipient','baseAmount','nativeAmount','verifierDomain'];
  const types = ['uint8','bytes32','bytes32','uint256','bytes32','address','uint256','bytes32','address',
    'uint256','bytes32','address','address','uint256','uint256','bytes32'];
  const typeHash = ethers.id('NanoInventoryMessageV2(uint8 kind,bytes32 routeId,bytes32 termsHash,uint256 sourceChainId,bytes32 sourceDomain,address sourceEndpoint,uint256 destinationChainId,bytes32 destinationDomain,address destinationEndpoint,uint256 nonce,bytes32 transferId,address sender,address recipient,uint256 baseAmount,uint256 nativeAmount,bytes32 verifierDomain)');
  const plainMessage = value => Object.fromEntries(fields.map((k,i) => [k,
    types[i] === 'uint8' ? Number(value[k] ?? value[i]) : types[i] === 'uint256' ? String(value[k] ?? value[i]) : lower(value[k] ?? value[i])]));
  const digest = m => ethers.keccak256(abi.encode(['bytes32',...types],[typeHash,...fields.map(k=>m[k])]));
  const sourceKey = m => ethers.keccak256(abi.encode(['uint256','bytes32','address','uint256'],
    [m.sourceChainId,m.sourceDomain,m.sourceEndpoint,m.nonce]));
  const reports = [], receipts = [], hashes = new Set(), deploymentBounds = [];
  let gas = 0n, gasCost = 0n, expectedRefusals = 0, pairedChecks = 0;
  function receipt(r, expected, label) {
    need(r && r.success === expected && Array.isArray(r.logs), 'receipt status ' + label);
    need(/^0x[0-9a-fA-F]{64}$/.test(r.transactionHash) && /^0x[0-9a-fA-F]{64}$/.test(r.blockHash), 'receipt identifiers');
    need(!hashes.has(lower(r.transactionHash)), 'distinct executed transaction'); hashes.add(lower(r.transactionHash));
    const used = uint(r.gasUsed), price = uint(r.effectiveGasPrice); address(r.from);
    gas += used; gasCost += used * price;
    receipts.push({label,transactionHash:r.transactionHash,blockHash:r.blockHash,
      success:r.success,gasUsed:used.toString(),effectiveGasPrice:price.toString(),from:lower(r.from)});
    if (!expected) { need(r.logs.length === 0, 'failed transaction emitted no retained log'); ++expectedRefusals; }
    return r;
  }
  async function deploy(name,args = [],value = 0n) {
    const bytecode = artifacts[name].bytecode;
    need(typeof bytecode === 'string' && /^0x(?:[0-9a-fA-F]{2})+$/.test(bytecode), 'linked creation bytecode ' + name);
    const initBytes = (bytecode.length - 2 + interfaces[name].encodeDeploy(args).length - 2)/2;
    need(initBytes <= 49152, 'EIP-3860 creation bound ' + name);
    const d = await backend.deploy(name,args,{value:value.toString()}); receipt(d.receipt,true,'deploy ' + name);
    const where = address(d.address), code = await backend.code(where);
    need(typeof code === 'string' && /^0x(?:[0-9a-fA-F]{2})+$/.test(code), 'deployed code ' + name);
    const bytes = (code.length - 2)/2; need(bytes <= 24576, 'EIP-170 runtime bound ' + name);
    deploymentBounds.push({name,initBytes,runtimeBytes:bytes}); return where;
  }
  const send = async (where,name,method,args = [],ok = true) => receipt(
    await backend.send(where,name,method,args),ok,name+'.'+method);
  function matching(r,where,name,event) {
    const found=[];
    for (const log of r.logs) if (lower(log.address)===where) {
      const p = interfaces[name].parseLog(log); need(p,'decodable log'); if (p.name===event) found.push(p);
    }
    return found;
  }
  const suiteA = await deploy('InventoryV2CasesA'),suiteB=await deploy('InventoryV2CasesB');
  need(uint(await read(suiteA,'InventoryV2CasesA','CASE_COUNT'))===8n && uint(await read(suiteA,'InventoryV2CasesA','FIRST_CASE_ID'))===0n,'original cases 0..7');
  need(uint(await read(suiteB,'InventoryV2CasesB','CASE_COUNT'))===7n && uint(await read(suiteB,'InventoryV2CasesB','FIRST_CASE_ID'))===8n,'original cases 8..14');
  for (let i=0;i<15;i++) {
    const suite=i<8?suiteA:suiteB,name=i<8?'InventoryV2CasesA':'InventoryV2CasesB';
    const fixture = await deploy('InventoryV2Fixture',[1000n,2n,3n],10000n);
    const r = await send(suite,name,'runCase',[i,fixture]);
    const events = matching(r,suite,name,'CasePassed');
    need(events.length===1 && Number(events[0].args.caseId)===i && address(events[0].args.fixture)===fixture,'one exact case completion');
    need(events[0].args.finalState===await read(fixture,'InventoryV2Fixture','stateHash'),'final committed fixture state');
    reports.push({caseId:i,passed:true});
  }
  const termsSuite = await deploy('InventoryV2TermsCases');
  need(uint(await read(termsSuite,'InventoryV2TermsCases','CASE_COUNT'))===8n,'all eight terms cases');
  for (let i=0;i<8;i++) {
    const r=await send(termsSuite,'InventoryV2TermsCases','runCase',[i]);
    const events=matching(r,termsSuite,'InventoryV2TermsCases','TermsCasePassed');
    need(events.length===1 && Number(events[0].args.caseId)===i,'one exact terms completion');
    reports.push({termsCaseId:i,passed:true});
  }
  const multiSuite=await deploy('InventoryV2MultiClientCases');
  need(uint(await read(multiSuite,'InventoryV2MultiClientCases','CASE_COUNT'))===1n,'one actual multi-client case');
  const multiFixture=await deploy('InventoryV2Fixture',[1000n,2n,3n],10000n);
  const multiReceipt=await send(multiSuite,'InventoryV2MultiClientCases','runCase',[0,multiFixture]);
  const multiEvents=matching(multiReceipt,multiSuite,'InventoryV2MultiClientCases','MultiClientCasePassed');
  need(multiEvents.length===1 && Number(multiEvents[0].args.caseId)===0 && address(multiEvents[0].args.fixture)===multiFixture,'one exact multi-client completion');
  const multi=multiEvents[0].args,clientA=address(multi.clientA),clientB=address(multi.clientB);
  need(clientA!==clientB && clientA!==multiFixture && clientB!==multiFixture,'distinct actual Base clients');
  need(multi.finalState===await read(multiSuite,'InventoryV2MultiClientCases','stateHash'),'complete multi-client readback state');
  for(const client of [clientA,clientB]){
    const code=await backend.code(client);need(/^0x(?:[0-9a-fA-F]{2})+$/.test(code) && (code.length-2)/2<=24576,'actual bounded client runtime');
  }
  reports.push({multiClientCaseId:0,passed:true,twoDistinctBaseRecipients:true,twoDistinctNativeFunders:true});

  // Distinct paired trace: each successful public transaction is joined to an
  // independent journal and then to actual readbacks; refused transactions do
  // not append an economic fact. The Fixture wallet does not pay outer EOA gas.
  const f = await deploy('InventoryV2Fixture',[1000n,2n,3n],10000n);
  const b = address(await read(f,'InventoryV2Fixture','base')),
    n = address(await read(f,'InventoryV2Fixture','nativeReserve')),
    token = address(await read(f,'InventoryV2Fixture','token'));
  const t = await backend.read(b,'InventoryV2OpenBase','terms',[]);
  // Backend read returns the decoded tuple directly for this struct result.
  const terms = t.baseChainId !== undefined ? t : t[0];
  const model = createInventoryModel({fixtureOnly:true,controller:f,totalBaseSupply:'1000',baseLot:'2',nativeLot:'3',
    routeId:lower(await read(b,'InventoryV2OpenBase','routeId')),termsHash:lower(await read(b,'InventoryV2OpenBase','termsHash')),
    base:{chainId:String(terms.baseChainId),domain:lower(terms.baseDomain),endpoint:b,
      verifierDomain:lower(terms.baseToNativeVerifierDomain),decimals:18},
    native:{chainId:String(terms.nativeChainId),domain:lower(terms.nativeDomain),endpoint:n,
      verifierDomain:lower(terms.nativeToBaseVerifierDomain),decimals:18},
    baseBalances:{[f]:'1000'},nativeBalances:{[f]:'10000'}});
  const emitted = new Map(), lockMessages = [];
  async function emissions(r) {
    const result=[];
    for (const [where,name,side] of [[b,'InventoryV2OpenBase','base'],[n,'InventoryV2OpenNative','native']]) {
      for (const p of matching(r,where,name,'MessageSent')) {
        const m=plainMessage(p.args.message), ref=side+':'+m.nonce;
        need(!emitted.has(ref) && lower(p.args.digest)===digest(m) && lower(p.args.sourceKey)===sourceKey(m),'independent wire commitment');
        need(m.sourceEndpoint===where,'actual source endpoint');
        need(lower(await read(where,name,'outbox',[m.nonce]))===digest(m),'actual outbox commitment');
        emitted.set(ref,m);result.push(m);if(m.kind===2)lockMessages.push(m);
      }
    }
    return result;
  }
  const pick=(messages,kind)=>{const a=messages.filter(m=>m.kind===kind);need(a.length===1,'one actual message kind '+kind);return a[0];};
  const ref=m=>(m.sourceEndpoint===b?'base:':'native:')+m.nonce;
  async function observed() {
    const reserve=uint(await read(n,'InventoryV2OpenNative','accountedReserve')),
      pending=uint(await read(n,'InventoryV2OpenNative','pendingFunds')),
      accounted=uint(await read(b,'InventoryV2OpenBase','accountedBase')),
      B=uint(await read(token,'InventoryV2TokenMock','balanceOf',[b])), R=uint(await backend.balance(n));
    let pn=0n;
    for(const m of lockMessages)if(await read(n,'InventoryV2OpenNative','paidLocks',[m.transferId])===false)pn+=BigInt(m.nativeAmount);
    const snapshot={totalSupply:String(await read(token,'InventoryV2TokenMock','totalSupply')),baseInventory:B.toString(),nativeBalance:R.toString(),
      freeBase:String(await read(b,'InventoryV2OpenBase','freeBase')),quarantinedBase:String(await read(b,'InventoryV2OpenBase','pendingNativeBase')),
      pendingNative:pn.toString(),pendingBase:String(await read(b,'InventoryV2OpenBase','reservedBase')),pendingFunds:pending.toString(),
      accountedReserve:reserve.toString(),donatedBase:(B-accounted).toString(),donatedNative:(R-reserve-pending).toString(),executionFees:'0',
      baseBalances:{[f]:String(await read(token,'InventoryV2TokenMock','balanceOf',[f]))},nativeBalances:{[f]:String(await backend.balance(f))}};
    model.assertObserved(snapshot);
    const expected=model.snapshot();
    need(await read(b,'InventoryV2OpenBase','backingReady')===expected.confirmed &&
      await read(b,'InventoryV2OpenBase','supplyDeclared')===expected.declared &&
      await read(n,'InventoryV2OpenNative','backingInitialized')===expected.bootstrapped,'actual one-shot bootstrap states');
    for(const [where,name,side]of [[b,'InventoryV2OpenBase','base'],[n,'InventoryV2OpenNative','native']]){
      need(await read(where,name,'enabled')===expected.gates[side] && await read(where,name,'supplyFrozen')===expected.frozen[side],'actual gate/freeze');
      const count=[...emitted.keys()].filter(k=>k.startsWith(side+':')).length;
      need(uint(await read(where,name,'nextNonce'))===BigInt(count+1),'strict contiguous outgoing nonce');
    }
    for(const [key,m]of emitted){need(JSON.stringify(expected.messages[key])===JSON.stringify(m),'model wire matches actual source');
      const dest=m.sourceEndpoint===b?n:b,name=m.sourceEndpoint===b?'InventoryV2OpenNative':'InventoryV2OpenBase';
      need(await read(dest,name,'consumed',[sourceKey(m)])===expected.consumed.includes((dest===b?'base:':'native:')+key),'actual exclusive consumption');}
    ++pairedChecks;return snapshot;
  }
  async function action(method,args,facts,ok=true) {
    const before = model.snapshot();
    const r=await send(f,'InventoryV2Fixture',method,args,ok);need(lower(r.from)!==f,'outer gas payer separate from modeled fixture');
    const ms=await emissions(r);
    if(ok)for(const fact of facts(ms))model.append(fact);
    else need(JSON.stringify(model.snapshot())===JSON.stringify(before),'refusal did not alter oracle facts');
    await observed();return ms;
  }
  await observed();
  await action('enable',[],()=>[{op:'setGate',actor:f,side:'base',enabled:true},{op:'setGate',actor:f,side:'native',enabled:true}]);
  let ms=await action('declare',[],a=>{const m=pick(a,0);return[{op:'declareSupply',actor:f,observedSupply:'1000',id:m.transferId,nonce:m.nonce}];});
  const boot=pick(ms,0);
  await action('finishBootstrap',[boot,1499],()=>[],false);
  await action('finishBootstrap',[boot,1500],a=>{const m=pick(a,1);return[
    {op:'bootstrapNative',actor:f,value:'1500',message:ref(boot),nonce:m.nonce},{op:'confirmBacking',message:ref(m)}];});
  ms=await action('lock',[100,f],a=>{const m=pick(a,2);return[{op:'lockBase',actor:f,recipient:f,baseAmount:'100',id:m.transferId,nonce:m.nonce}];});
  const locked=pick(ms,2);
  ms=await action('payLock',[locked],a=>[{op:'payNative',message:ref(locked),nonce:pick(a,3).nonce}]);
  const paid=pick(ms,3);await action('acknowledgeLock',[paid],()=>[{op:'ackNativePaid',message:ref(paid)}]);
  ms=await action('reserve',[40,f],a=>{const m=pick(a,4);return[{op:'reserveBase',actor:f,recipient:f,funder:f,baseAmount:'40',id:m.transferId,nonce:m.nonce}];});
  const reserved=pick(ms,4);
  ms=await action('requestCancel',[reserved.transferId],a=>[{op:'requestCancel',actor:f,id:reserved.transferId,nonce:pick(a,7).nonce}]);
  const cancelRequest=pick(ms,7);
  ms=await action('fund',[reserved],a=>[{op:'fundNative',actor:f,message:ref(reserved),value:'60',nonce:pick(a,5).nonce}]);
  const funded=pick(ms,5);
  await action('cancelUnfunded',[cancelRequest],()=>[],false);await action('settle',[funded],()=>[],false);
  ms=await action('cancelFunded',[funded],a=>[{op:'cancelFundedBase',actor:f,message:ref(funded),nonce:pick(a,9).nonce}]);
  const cancel=pick(ms,9);
  ms=await action('refund',[cancel],a=>[{op:'refundNative',message:ref(cancel),nonce:pick(a,10).nonce}]);
  const refund=pick(ms,10);await action('acknowledgeRefund',[refund],()=>[{op:'ackNativeRefunded',message:ref(refund)}]);
  ms=await action('reserve',[60,f],a=>{const m=pick(a,4);return[{op:'reserveBase',actor:f,recipient:f,funder:f,baseAmount:'60',id:m.transferId,nonce:m.nonce}];});
  const second=pick(ms,4);
  ms=await action('fund',[second],a=>[{op:'fundNative',actor:f,message:ref(second),value:'90',nonce:pick(a,5).nonce}]);
  const secondFund=pick(ms,5);
  ms=await action('settle',[secondFund],a=>[{op:'payBase',message:ref(secondFund),nonce:pick(a,6).nonce}]);
  const secondPaid=pick(ms,6);await action('acknowledgeBase',[secondPaid],()=>[{op:'ackBasePaid',message:ref(secondPaid)}]);
  await action('donate',[2,7],()=>[{op:'donateBase',actor:f,amount:'2'},{op:'donateNative',actor:f,amount:'7'}]);
  const finalFinancial=await observed();
  return {format:'nano-inventory-v2-private-synthetic-execution-v1',scenarioCompleted:true,cases:reports,
    pairedChecks,expectedRefusals,receipts,deploymentBounds,gasUsed:gas.toString(),receiptCalculatedOuterGasCost:gasCost.toString(),
    outerGasWalletDebitVerified:false,
    finalFinancial,sourceFinalityAuthenticated:false,economicTermsAdopted:false,realAssetsUsed:false,
    actualArtifactProvenanceQualified:false,processRecoveryQualified:false,productionQualified:false};
}
