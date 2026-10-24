// Independent append-only economic oracle. No Solidity/runtime/tool imports.
export const Kind=Object.freeze({BOOTSTRAP:0,ACK_BOOTSTRAP:1,LOCK_BASE:2,ACK_NATIVE_PAID:3,RESERVE_BASE:4,FUND_NATIVE:5,ACK_BASE_PAID:6,REQUEST_CANCEL:7,ACK_CANCEL_UNFUNDED:8,CANCEL_FUNDED:9,ACK_NATIVE_REFUNDED:10,FREEZE_SUPPLY:11});
export const UINT256_MAX=(1n<<256n)-1n;
const fail=code=>{throw Error('Inventory model: '+code);};
const need=(condition,code)=>{if(!condition)fail(code);};
const own=value=>structuredClone(value);
const canonical=value=>Array.isArray(value)?'['+value.map(canonical).join(',')+']':value&&typeof value==='object'?'{'+Object.keys(value).sort().map(k=>JSON.stringify(k)+':'+canonical(value[k])).join(',')+'}':JSON.stringify(value);
const address=value=>{need(typeof value==='string'&&/^0x[0-9a-f]{40}$/.test(value)&&!/^0x0{40}$/.test(value),'address');return value;};
const root=value=>{need(typeof value==='string'&&/^0x[0-9a-f]{64}$/.test(value),'root');return value;};
export function uint(value){need(typeof value==='string'&&/^(?:0|[1-9][0-9]*)$/.test(value),'uint encoding');const n=BigInt(value);need(n<=UINT256_MAX,'uint256');return n;}
const checked=n=>{need(n>=0n&&n<=UINT256_MAX,'uint256 result');return n;};
const positive=value=>{const n=uint(value);need(n>0n,'positive');return n;};
const sum=values=>values.reduce((n,value)=>n+value,0n);
const mapBalances=value=>{need(value&&typeof value==='object'&&!Array.isArray(value),'balance map');return new Map(Object.entries(value).map(([key,amount])=>[address(key),uint(amount)]));};
const balance=(map,key)=>map.get(key)??0n;
const move=(map,key,delta)=>map.set(key,checked(balance(map,key)+delta));
const asObject=map=>Object.fromEntries([...map].sort(([a],[b])=>a.localeCompare(b)).map(([key,value])=>[key,value.toString()]));
const ZERO='0x'+'0'.repeat(64);

export function quoteBaseToNative(amount,baseLot,nativeLot){const b=positive(amount),p=positive(baseLot),q=positive(nativeLot);need(b%p===0n,'base remainder');return checked((b/p)*q).toString();}
export function quoteNativeToBase(amount,baseLot,nativeLot){const n=positive(amount),p=positive(baseLot),q=positive(nativeLot);need(n%q===0n,'native remainder');return checked((n/q)*p).toString();}

export function createInventoryModel(input){
 const p=own(input);need(p.fixtureOnly===true,'fixture only');root(p.routeId);root(p.termsHash);address(p.controller);
 for(const side of ['base','native']){const d=p[side];need(d&&typeof d==='object','domain');positive(d.chainId);root(d.domain);root(d.verifierDomain);address(d.endpoint);need(Number.isInteger(d.decimals)&&d.decimals>=0&&d.decimals<=255,'decimals');}
 need(canonical([p.base.chainId,p.base.domain,p.base.endpoint])!==canonical([p.native.chainId,p.native.domain,p.native.endpoint]),'distinct domains');
 const T=positive(p.totalBaseSupply),lot=positive(p.baseLot),nativeLot=positive(p.nativeLot),backing=BigInt(quoteBaseToNative(T.toString(),p.baseLot,p.nativeLot));
 const originalBase=mapBalances(p.baseBalances),originalNative=mapBalances(p.nativeBalances);
 need(sum([...originalBase.values()])===T,'entire preissued allocation');
 need(!originalBase.has(p.base.endpoint)&&!originalNative.has(p.native.endpoint),'no initial inventory import');
 const nativeTotal=sum([...originalNative.values()]);need(nativeTotal<=UINT256_MAX,'native fixture total');
 let facts=[];

 function derive(journal){
  const base=new Map(originalBase),native=new Map(originalNative),messages=new Map(),consumed=new Set(),returns=new Map(),reservations=new Map();
  let B=0n,R=0n,donatedBase=0n,donatedNative=0n,fees=0n,declared=false,bootstrapped=false,confirmed=false,accounted=0n;
  const gates={base:false,native:false},frozen={base:false,native:false};let externalSupplyObservation=null;
  const quote=b=>BigInt(quoteBaseToNative(b.toString(),p.baseLot,p.nativeLot));
  const enabled=side=>need(gates[side]&&confirmed,'closed route');
  const risk=side=>{enabled(side);need(!frozen[side],'supply frozen');};
  const lookup=(map,id)=>{root(id);const value=map.get(id);need(value,'known transfer');return value;};
  const ref=(side,nonce)=>{need(['base','native'].includes(side),'message origin');positive(nonce);return side+':'+nonce;};
  function emit(side,nonce,kind,id,sender,recipient,b,n){
   root(id);address(sender);address(recipient);checked(b);checked(n);
   const key=ref(side,nonce);need(!messages.has(key),'source nonce reused');
   const source=p[side],destination=p[side==='base'?'native':'base'];
   const message={kind,routeId:p.routeId,termsHash:p.termsHash,sourceChainId:source.chainId,sourceDomain:source.domain,sourceEndpoint:source.endpoint,destinationChainId:destination.chainId,destinationDomain:destination.domain,destinationEndpoint:destination.endpoint,nonce,transferId:id,sender,recipient,baseAmount:b.toString(),nativeAmount:n.toString(),verifierDomain:source.verifierDomain};
   messages.set(key,message);return key;
  }
  function consume(event,destination,kind){
   need(event.proofAccepted!==false,'synthetic proof refused');
   const message=messages.get(event.message);need(message,'actual source emission');
   need(message.kind===kind&&message.destinationEndpoint===p[destination].endpoint,'message kind or destination');
   if(event.claim!==undefined)need(canonical(event.claim)===canonical(message),'contradictory source payload');
   const key=destination+':'+event.message;need(!consumed.has(key),'source already consumed');consumed.add(key);return message;
  }
  function totals(){
   const Q=sum([...returns.values()].filter(x=>!x.acked).map(x=>x.b));
   const PN=sum([...returns.values()].filter(x=>!x.paid).map(x=>x.n));
   const PB=sum([...reservations.values()].filter(x=>['Reserved','CancelRequested'].includes(x.baseState)).map(x=>x.b));
   const pending=sum([...reservations.values()].filter(x=>x.funded&&!x.nativePaid&&!x.refunded).map(x=>x.n));
   const free=B-donatedBase-Q-PB;
   const F=sum([...reservations.values()].filter(x=>x.funded&&x.baseState!=='Paid'&&!x.refunded).map(x=>x.n));
   need(free>=0n,'inventory overcommitted');
   need(B+sum([...base.values()])===T,'base asset conservation');
   need(R+sum([...native.values()])+fees===nativeTotal,'native asset conservation');
   need(R===accounted+pending+donatedNative,'pending funds cannot fund reserve');
   if(bootstrapped){
    // Cross multiplication remains exact even when a donation is not a lot.
    const margin=lot*R-nativeLot*(T-B)-lot*(PN+F);
    need(margin===nativeLot*donatedBase+lot*donatedNative,'global backing reconciliation');
   }
   for(const value of [B,R,PN,PB,Q,pending,free,accounted,donatedBase,donatedNative])checked(value);
   return {Q,PN,PB,pending,free,F};
  }
  for(const event of journal){
   need(event&&typeof event==='object'&&typeof event.op==='string','event');
   switch(event.op){
    case 'setGate':need(event.actor===p.controller&&['base','native'].includes(event.side)&&typeof event.enabled==='boolean','fixture controller');gates[event.side]=event.enabled;break;
    case 'declareSupply':
     need(gates.base&&!frozen.base,'closed fixture gate');address(event.actor);need(!declared&&!bootstrapped,'one-shot declaration');
     need(event.observedSupply===T.toString(),'full fixed supply');declared=true;
     emit('base',event.nonce,Kind.BOOTSTRAP,event.id??('0x'+'0'.repeat(62)+'f0'),event.actor,p.native.endpoint,T,backing);break;
    case 'bootstrapNative':{
     need(gates.native&&!frozen.native,'closed fixture gate');need(!bootstrapped,'one-shot backing');const m=consume(event,'native',Kind.BOOTSTRAP);
     need(declared&&m.baseAmount===T.toString()&&m.nativeAmount===backing.toString(),'entire preissued backing');
     need(positive(event.value)===backing,'exact bootstrap value');address(event.actor);
     move(native,event.actor,-backing);R=checked(R+backing);accounted=backing;bootstrapped=true;
     emit('native',event.nonce,Kind.ACK_BOOTSTRAP,m.transferId,m.sender,m.recipient,T,backing);break;}
    case 'confirmBacking':need(gates.base,'closed fixture gate');consume(event,'base',Kind.ACK_BOOTSTRAP);need(bootstrapped&&!confirmed,'one-shot confirmation');confirmed=true;break;
    case 'lockBase':{
     risk('base');root(event.id);need(event.id!==ZERO&&!returns.has(event.id)&&!reservations.has(event.id),'unique transfer');
     address(event.actor);address(event.recipient);need(event.recipient!==p.native.endpoint,'native endpoint is not recipient');const b=positive(event.baseAmount),n=quote(b);
     move(base,event.actor,-b);B=checked(B+b);
     const row={b,n,sender:event.actor,recipient:event.recipient,paid:false,acked:false};returns.set(event.id,row);
     row.lock=emit('base',event.nonce,Kind.LOCK_BASE,event.id,row.sender,row.recipient,b,n);break;}
    case 'payNative':{
     risk('native');const m=consume(event,'native',Kind.LOCK_BASE),r=lookup(returns,m.transferId);
     need(!r.paid&&accounted>=r.n,'unpaid accounted native reserve');
     need(!['revert','reenter-revert'].includes(event.receiver),'native receiver rejected');
     // A caught nested reentry commits only this one outer economic payment.
     need(event.receiver===undefined||['accept','reenter-caught'].includes(event.receiver),'receiver behavior');
     R=checked(R-r.n);accounted=checked(accounted-r.n);move(native,r.recipient,r.n);r.paid=true;
     r.pay=emit('native',event.nonce,Kind.ACK_NATIVE_PAID,m.transferId,r.sender,r.recipient,r.b,r.n);break;}
    case 'ackNativePaid':{
     enabled('base');const m=consume(event,'base',Kind.ACK_NATIVE_PAID),r=lookup(returns,m.transferId);
     need(r.paid&&!r.acked,'one paid lock ack');r.acked=true;break;}
    case 'reserveBase':{
     risk('base');root(event.id);need(event.id!==ZERO&&!returns.has(event.id)&&!reservations.has(event.id),'unique transfer');
     address(event.actor);address(event.recipient);need(event.recipient===event.actor,'recipient creates own reservation');const funder=address(event.funder??event.actor);need(funder!==p.native.endpoint&&event.actor!==p.base.endpoint,'endpoint cannot reserve for itself');const b=positive(event.baseAmount),n=quote(b);need(totals().free>=b,'reserve before funding');
     const r={b,n,sender:funder,recipient:event.recipient,baseState:'Reserved',funded:false,nativePaid:false,nativeCancelled:false,refunded:false,refundAck:false};reservations.set(event.id,r);
     r.reserve=emit('base',event.nonce,Kind.RESERVE_BASE,event.id,r.sender,r.recipient,b,n);break;}
    case 'fundNative':{
     risk('native');const m=consume(event,'native',Kind.RESERVE_BASE),r=lookup(reservations,m.transferId);
     need(!r.funded&&!r.nativeCancelled,'fund once before native cancellation');
     need(event.actor===r.sender&&positive(event.value)===r.n,'exact reserved funder and value');
     move(native,r.sender,-r.n);R=checked(R+r.n);r.funded=true;
     r.fund=emit('native',event.nonce,Kind.FUND_NATIVE,m.transferId,r.sender,r.recipient,r.b,r.n);break;}
    case 'payBase':{
     risk('base');const m=consume(event,'base',Kind.FUND_NATIVE),r=lookup(reservations,m.transferId);
     need(r.funded&&r.baseState==='Reserved','cancel request excludes base payout');
     B=checked(B-r.b);move(base,r.recipient,r.b);r.baseState='Paid';
     r.pay=emit('base',event.nonce,Kind.ACK_BASE_PAID,m.transferId,r.sender,r.recipient,r.b,r.n);break;}
    case 'ackBasePaid':{
     enabled('native');const m=consume(event,'native',Kind.ACK_BASE_PAID),r=lookup(reservations,m.transferId);
     need(r.funded&&r.baseState==='Paid'&&!r.nativePaid&&!r.refunded,'irreversible base payout');
     r.nativePaid=true;accounted=checked(accounted+r.n);break;}
    case 'requestCancel':{
     enabled('base');const r=lookup(reservations,event.id);need(event.actor===r.recipient&&r.baseState==='Reserved','creator requests once before payout');r.baseState='CancelRequested';
     r.request=emit('base',event.nonce,Kind.REQUEST_CANCEL,event.id,r.sender,r.recipient,r.b,r.n);break;}
    case 'ackCancelUnfunded':{
     enabled('native');const m=consume(event,'native',Kind.REQUEST_CANCEL),r=lookup(reservations,m.transferId);
     need(r.baseState==='CancelRequested'&&!r.funded&&!r.nativeCancelled,'native confirms no funds');r.nativeCancelled=true;
     r.cancelAck=emit('native',event.nonce,Kind.ACK_CANCEL_UNFUNDED,m.transferId,r.sender,r.recipient,r.b,r.n);break;}
    case 'cancelUnfundedBase':{
     enabled('base');const m=consume(event,'base',Kind.ACK_CANCEL_UNFUNDED),r=lookup(reservations,m.transferId);
     need(r.baseState==='CancelRequested'&&r.nativeCancelled&&!r.funded,'cancel ack precedes capacity release');r.baseState='CancelledUnfunded';break;}
    case 'cancelFundedBase':{
     enabled('base');const m=consume(event,'base',Kind.FUND_NATIVE),r=lookup(reservations,m.transferId);
     need(event.actor===r.recipient&&r.funded&&['Reserved','CancelRequested'].includes(r.baseState),'creator cancels before payout');r.baseState='CancelledFunded';
     r.cancel=emit('base',event.nonce,Kind.CANCEL_FUNDED,m.transferId,r.sender,r.recipient,r.b,r.n);break;}
    case 'refundNative':{
     enabled('native');const m=consume(event,'native',Kind.CANCEL_FUNDED),r=lookup(reservations,m.transferId);
     need(r.funded&&r.baseState==='CancelledFunded'&&!r.nativePaid&&!r.refunded,'refund excludes payout');
     need(!['revert','reenter-revert'].includes(event.receiver),'native receiver rejected');
     need(event.receiver===undefined||['accept','reenter-caught'].includes(event.receiver),'receiver behavior');
     R=checked(R-r.n);move(native,r.sender,r.n);r.refunded=true;
     r.refund=emit('native',event.nonce,Kind.ACK_NATIVE_REFUNDED,m.transferId,r.sender,r.recipient,r.b,r.n);break;}
    case 'ackNativeRefunded':{
     enabled('base');const m=consume(event,'base',Kind.ACK_NATIVE_REFUNDED),r=lookup(reservations,m.transferId);
     need(r.baseState==='CancelledFunded'&&r.refunded&&!r.refundAck,'refund trace ack');r.refundAck=true;break;}
    case 'donateBase':{address(event.actor);const amount=positive(event.amount);move(base,event.actor,-amount);B=checked(B+amount);donatedBase=checked(donatedBase+amount);break;}
    case 'donateNative':{address(event.actor);const amount=positive(event.amount);move(native,event.actor,-amount);R=checked(R+amount);donatedNative=checked(donatedNative+amount);break;}
    case 'chargeGas':{address(event.actor);const amount=uint(event.amount);move(native,event.actor,-amount);fees=checked(fees+amount);break;}
    case 'signalSupplyMismatch':{
     need(gates.base&&!frozen.base,'freeze once');address(event.actor);const observed=uint(event.observedSupply);need(observed!==T,'supply actually differs');
     frozen.base=true;externalSupplyObservation=event.observedSupply;
     emit('base',event.nonce,Kind.FREEZE_SUPPLY,event.id??('0x'+'0'.repeat(62)+'ee'),event.actor,p.native.endpoint,observed,0n);break;}
    case 'acceptSupplyFreeze':{
     need(gates.native&&!frozen.native,'freeze once');const m=consume(event,'native',Kind.FREEZE_SUPPLY);
     need(m.baseAmount!==T.toString()&&m.nativeAmount==='0','mismatch freeze evidence');frozen.native=true;break;}
    default:fail('unknown economic event');
   }
   totals();
  }
  const t=totals(),financial={totalSupply:T.toString(),baseInventory:B.toString(),nativeBalance:R.toString(),freeBase:t.free.toString(),quarantinedBase:t.Q.toString(),pendingNative:t.PN.toString(),pendingBase:t.PB.toString(),pendingFunds:t.pending.toString(),accountedReserve:accounted.toString(),donatedBase:donatedBase.toString(),donatedNative:donatedNative.toString(),executionFees:fees.toString(),baseBalances:asObject(base),nativeBalances:asObject(native)};
  return {financial,declared,bootstrapped,confirmed,gates,frozen,externalSupplyObservation,physicalBaseSupplyConservationClaimed:externalSupplyObservation===null,returns:Object.fromEntries([...returns].map(([id,r])=>[id,{paid:r.paid,acked:r.acked}])),reservations:Object.fromEntries([...reservations].map(([id,r])=>[id,{baseState:r.baseState,funded:r.funded,nativePaid:r.nativePaid,nativeCancelled:r.nativeCancelled,refunded:r.refunded,refundAck:r.refundAck}])),messages:Object.fromEntries(messages),consumed:[...consumed].sort(),traceLength:journal.length,sourceFinalityAuthenticated:false,productionQualified:false};
 }
 return Object.freeze({
  append(event){const next=[...facts,own(event)],projected=derive(next);facts=next;return own(projected);},
  snapshot(){return own(derive(facts));},
  message(side,nonce){const value=derive(facts).messages[side+':'+nonce];need(value,'known message');return own(value);},
  facts(){return own(facts);},
  assertObserved(financial){need(canonical(financial)===canonical(derive(facts).financial),'paired observed checkpoint mismatch');return {consistent:true,sourceFinalityAuthenticated:false,productionQualified:false};}
 });
}
