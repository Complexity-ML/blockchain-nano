// Source proposal only: this suite has not been executed by its author.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createInventoryModel,quoteBaseToNative,quoteNativeToBase,UINT256_MAX,Kind} from './inventory-model.mjs';
const vectors=JSON.parse(await readFile(new URL('./trace-vectors.json',import.meta.url),'utf8'));
const clone=value=>structuredClone(value),A='0x'+'1'.repeat(40),B='0x'+'2'.repeat(40),TREASURY='0x'+'3'.repeat(40),CONTROLLER='0x'+'4'.repeat(40);
const id=n=>'0x'+BigInt(n).toString(16).padStart(64,'0');
function refuses(model,event,pattern){const before=model.snapshot(),facts=model.facts();assert.throws(()=>model.append(event),pattern);assert.deepEqual(model.snapshot(),before);assert.deepEqual(model.facts(),facts);}
function opened(){const m=createInventoryModel(clone(vectors.parameters));
 for(const side of ['base','native'])m.append({op:'setGate',actor:CONTROLLER,side,enabled:true});
 m.append({op:'declareSupply',actor:CONTROLLER,observedSupply:'1000',nonce:'1'});
 m.append({op:'bootstrapNative',actor:TREASURY,value:'3500',message:'base:1',nonce:'1'});
 m.append({op:'confirmBacking',message:'native:1'});return m;
}
function inventory(){const m=opened();m.append({op:'lockBase',id:id(1),actor:A,recipient:A,baseAmount:'200',nonce:'2'});m.append({op:'payNative',message:'base:2',nonce:'2'});m.append({op:'ackNativePaid',message:'native:2'});return m;}

test('exact rational lots, explicit 6/18 atomic units, remainders and uint256 bounds',()=>{
 assert.equal(quoteBaseToNative('1000','2','7'),'3500');assert.equal(quoteNativeToBase('3500','2','7'),'1000');
 assert.equal(quoteBaseToNative('1000000000','2000000','7000000000000000000'),'3500000000000000000000');
 assert.equal(quoteNativeToBase('3500000000000000000000','2000000','7000000000000000000'),'1000000000');
 for(const [amount,p,q] of [['1','2','7'],['0','2','7'],['2','0','7'],['02','2','7'],['1.0','1','7'],[(UINT256_MAX+1n).toString(),'1','1']])assert.throws(()=>quoteBaseToNative(amount,p,q));
 assert.throws(()=>quoteNativeToBase('1','2','7'),/native remainder/);
 assert.equal(quoteBaseToNative(UINT256_MAX.toString(),'3','2'),((UINT256_MAX/3n)*2n).toString());
 assert.throws(()=>quoteBaseToNative(UINT256_MAX.toString(),'1','2'),/uint256 result/);
 const bad=clone(vectors.parameters);bad.totalBaseSupply='1002';assert.throws(()=>createInventoryModel(bad),/entire preissued allocation/);
 const fractional=clone(vectors.parameters);fractional.totalBaseSupply='1001';assert.throws(()=>createInventoryModel(fractional),/base remainder/);
 // Decimal display metadata never silently rescales explicitly atomic lots.
 const changed=clone(vectors.parameters);changed.base.decimals=18;changed.native.decimals=6;assert.deepEqual(createInventoryModel(changed).snapshot().financial,createInventoryModel(vectors.parameters).snapshot().financial);
});

test('hand-calculated interleaving vector, every refusal atomic, full paired final checkpoint',()=>{
 const m=createInventoryModel(vectors.parameters);let accepted=0,rejected=0;
 for(const row of vectors.events){if(row.expectError){refuses(m,row.event,new RegExp(row.expectError));++rejected;}else{const got=m.append(row.event);++accepted;for(const [key,value] of Object.entries(row.expectedFinancial??{}))assert.deepEqual(got.financial[key],value,row.label+':'+key);}}
 assert(accepted>20&&rejected>5);assert.deepEqual(m.snapshot().financial,vectors.finalFinancial);
 assert.deepEqual(m.assertObserved(clone(vectors.finalFinancial)),{consistent:true,sourceFinalityAuthenticated:false,productionQualified:false});
 const removed=clone(vectors.finalFinancial);delete removed.pendingFunds;assert.throws(()=>m.assertObserved(removed),/paired observed checkpoint mismatch/);
 const corrupt=clone(vectors.finalFinancial);corrupt.totalSupply='1001';assert.throws(()=>m.assertObserved(corrupt),/paired observed checkpoint mismatch/);
 assert.equal(m.snapshot().physicalBaseSupplyConservationClaimed,true);
 const reserve=m.snapshot().financial.accountedReserve;m.append({op:'chargeGas',actor:A,amount:'7'});
 assert.equal(m.snapshot().financial.accountedReserve,reserve);assert.equal(m.snapshot().financial.executionFees,'7');assert.equal(m.snapshot().financial.nativeBalances[A],'1483');
});

test('all wire fields are bound and one source nonce cannot describe two payloads',()=>{
 const m=opened();m.append({op:'lockBase',id:id(1),actor:A,recipient:A,baseAmount:'20',nonce:'2'});
 const original=m.message('base','2');assert.equal(original.kind,Kind.LOCK_BASE);
 const mutations={kind:Kind.RESERVE_BASE,routeId:id(100),termsHash:id(101),sourceChainId:'8454',sourceDomain:id(102),sourceEndpoint:B,destinationChainId:'9001',destinationDomain:id(103),destinationEndpoint:A,nonce:'99',transferId:id(104),sender:B,recipient:B,baseAmount:'22',nativeAmount:'77',verifierDomain:id(105)};
 for(const [field,value] of Object.entries(mutations)){const claim=clone(original);claim[field]=value;refuses(m,{op:'payNative',message:'base:2',nonce:'2',claim},/contradictory source payload/);}
 refuses(m,{op:'payNative',message:'base:2',nonce:'2',proofAccepted:false},/synthetic proof refused/);
 refuses(m,{op:'payNative',message:'base:99',nonce:'2'},/actual source emission/);
 refuses(m,{op:'lockBase',id:id(2),actor:A,recipient:A,baseAmount:'2',nonce:'2'},/source nonce reused/);
 m.append({op:'payNative',message:'base:2',nonce:'2'});refuses(m,{op:'payNative',message:'base:2',nonce:'3'},/source already consumed/);
});

test('two independent returns are delivered and acknowledged out of order without early inventory credit',()=>{
 for(const b of ['2','20','100']){
  const m=opened(),n=quoteBaseToNative(b,'2','7');
  m.append({op:'lockBase',id:id(10),actor:A,recipient:A,baseAmount:b,nonce:'2'});
  m.append({op:'lockBase',id:id(11),actor:B,recipient:B,baseAmount:b,nonce:'3'});
  assert.equal(m.snapshot().financial.freeBase,'0');
  m.append({op:'payNative',message:'base:3',nonce:'2'});assert.equal(m.snapshot().financial.pendingNative,n);
  m.append({op:'ackNativePaid',message:'native:2'});assert.equal(m.snapshot().financial.freeBase,b);
  m.append({op:'payNative',message:'base:2',nonce:'3'});m.append({op:'ackNativePaid',message:'native:3'});
  assert.equal(m.snapshot().financial.freeBase,(2n*BigInt(b)).toString());assert.equal(m.snapshot().financial.pendingNative,'0');
 }
});

test('refundable funds cannot be spent as collateral before irreversible Base payout ack',()=>{
 const m=inventory();
 m.append({op:'reserveBase',id:id(2),actor:A,recipient:A,baseAmount:'200',nonce:'3'});
 m.append({op:'fundNative',message:'base:3',actor:A,value:'700',nonce:'3'});
 m.append({op:'payBase',message:'native:3',nonce:'4'});
 assert.equal(m.snapshot().financial.pendingFunds,'700');assert.equal(m.snapshot().financial.accountedReserve,'2800');
 m.append({op:'lockBase',id:id(3),actor:A,recipient:A,baseAmount:'600',nonce:'5'});
 m.append({op:'payNative',message:'base:5',nonce:'4'});
 m.append({op:'lockBase',id:id(4),actor:B,recipient:B,baseAmount:'400',nonce:'6'});
 assert.equal(m.snapshot().financial.nativeBalance,'1400');assert.equal(m.snapshot().financial.accountedReserve,'700');
 refuses(m,{op:'payNative',message:'base:6',nonce:'5'},/unpaid accounted native reserve/);
 m.append({op:'ackBasePaid',message:'base:4'});m.append({op:'payNative',message:'base:6',nonce:'5'});
 assert.equal(m.snapshot().financial.nativeBalance,'0');assert.equal(m.snapshot().financial.pendingFunds,'0');
});

test('recipient creates/cancels reservation, distinct funder is bound, payout and refund cannot both occur',()=>{
 const m=inventory();
 refuses(m,{op:'reserveBase',id:id(2),actor:A,recipient:B,baseAmount:'20',nonce:'3'},/recipient creates own reservation/);
 m.append({op:'reserveBase',id:id(2),actor:A,recipient:A,funder:B,baseAmount:'20',nonce:'3'});
 refuses(m,{op:'fundNative',message:'base:3',actor:A,value:'70',nonce:'3'},/exact reserved funder/);
 m.append({op:'fundNative',message:'base:3',actor:B,value:'70',nonce:'3'});
 refuses(m,{op:'requestCancel',id:id(2),actor:B,nonce:'4'},/creator requests/);
 m.append({op:'payBase',message:'native:3',nonce:'4'});
 refuses(m,{op:'requestCancel',id:id(2),actor:A,nonce:'5'},/creator requests/);
 refuses(m,{op:'cancelFundedBase',message:'native:3',actor:A,nonce:'5'},/source already consumed/);
 m.append({op:'ackBasePaid',message:'base:4'});
 assert.equal(m.snapshot().reservations[id(2)].nativePaid,true);assert.equal(m.snapshot().reservations[id(2)].refunded,false);
});

test('freeze evidence stops new risk, keeps already-created refund rights and makes no postcorruption supply claim',()=>{
 const m=inventory();m.append({op:'reserveBase',id:id(2),actor:A,recipient:A,baseAmount:'20',nonce:'3'});
 m.append({op:'fundNative',message:'base:3',actor:A,value:'70',nonce:'3'});
 m.append({op:'signalSupplyMismatch',actor:A,observedSupply:'1001',nonce:'4'});
 m.append({op:'acceptSupplyFreeze',message:'base:4'});
 refuses(m,{op:'payBase',message:'native:3',nonce:'5'},/supply frozen/);
 refuses(m,{op:'lockBase',id:id(3),actor:A,recipient:A,baseAmount:'2',nonce:'5'},/supply frozen/);
 m.append({op:'cancelFundedBase',message:'native:3',actor:A,nonce:'5'});
 m.append({op:'refundNative',message:'base:5',nonce:'4'});m.append({op:'ackNativeRefunded',message:'native:4'});
 assert.equal(m.snapshot().financial.pendingFunds,'0');assert.equal(m.snapshot().physicalBaseSupplyConservationClaimed,false);
 assert.equal(m.snapshot().externalSupplyObservation,'1001');assert.equal(m.snapshot().productionQualified,false);
});
