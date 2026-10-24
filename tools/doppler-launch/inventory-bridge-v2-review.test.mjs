// SOURCE proposal only: these new tests have not been executed by the author.
// Synthetic in-memory proposal/config/review; no wallet, RPC, chain or key.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {readFile} from 'node:fs/promises';
import {preview} from './preview.mjs';
import {inventoryBridgeV2TermsHash,inventoryBridgeV2RouteId,inventoryBridgeV2TermsComponents,verifyInventoryBridgeV2Proposal} from './inventory-bridge-v2-review.mjs';

const encode=x=>Buffer.from(JSON.stringify(x,(_,v)=>typeof v==='bigint'?v.toString():v,2)+'\n');
const hash=x=>createHash('sha256').update(x).digest('hex');
const address=n=>'0x'+n.repeat(40),domain=n=>'0x'+n.repeat(64);
const sample=JSON.parse(await readFile(new URL('./example.synthetic.json',import.meta.url)));
let original;
async function fixture(){
 if(!original){
  const configuration={...structuredClone(sample),initialSupply:'1000',numTokensToSell:'400',yearlyMintRate:'0',nativeAssetRelationship:'SYNTHETIC DECLARATION ONLY: explicit atomic lots, entire preissued supply, no activated bridge'};
  const review=JSON.parse(encode(await preview(configuration)).toString());
  const terms={baseChainId:'8453',baseDomain:domain('1'),baseEndpoint:address('2'),baseToken:review.predicted.tokenAddress,nativeChainId:'31337',nativeDomain:domain('3'),nativeEndpoint:address('4'),nativeAsset:'0x'+'0'.repeat(40),totalBaseSupply:'1000',baseLot:'2',nativeLot:'7',baseToNativeVerifierDomain:domain('5'),nativeToBaseVerifierDomain:domain('6'),maxProofBytes:'4096'};
  const proposal={format:'nano-doppler-inventory-bridge-v2-proposal-v1',status:'PROPOSED_NOT_ADOPTED',sdkVersion:'1.0.43',configurationSha256:hash(encode(configuration)),reviewSha256:hash(encode(review)),launch:{chainId:8453,predictedToken:review.predicted.tokenAddress,calldataHash:review.calldataHash},terms,termsHash:inventoryBridgeV2TermsHash(terms),routeId:inventoryBridgeV2RouteId(terms),entireSupplyPolicy:{scope:'entire-preissued-supply',futureMinting:'unsupported-refuse',reserveNativeUnits:'3500',reserveRelease:'no-unproven-release'},allocations:{auctionSale:'400',migrationLiquidity:'300',vesting:'100',treasury:'100',unallocated:'100'},unsoldTreatment:{subsetOf:'auctionSale',maximumBaseUnits:'400',coverage:'included-until-authenticated-settlement'}};
  original={configuration,review,proposal};
 }
 return structuredClone(original);
}
function pack(f){
 const configurationBytes=encode(f.configuration),reviewBytes=encode(f.review);f.proposal.configurationSha256=hash(configurationBytes);f.proposal.reviewSha256=hash(reviewBytes);const proposalBytes=encode(f.proposal);
 return {configurationBytes,configurationSha256:hash(configurationBytes),reviewBytes,reviewSha256:hash(reviewBytes),proposalBytes,proposalSha256:hash(proposalBytes)};
}

test('offline sidecar joins external byte pins and non1:1 atomic whole-supply terms without activation claims',async()=>{
 const f=await fixture(),oldFetch=globalThis.fetch;globalThis.fetch=()=>{throw Error('Unexpected network request');};
 try{
  const result=await verifyInventoryBridgeV2Proposal(pack(f));assert.equal(result.proposalEncodingVerified,true);assert.equal(result.coverage.totalBaseUnits,'1000');assert.equal(result.coverage.wholeBaseLots,'500');assert.equal(result.coverage.nativeUnits,'3500');assert.equal(result.coverage.baseLot,'2');assert.equal(result.coverage.nativeLot,'7');
  assert.deepEqual(result.allocations,{auctionSale:'400',migrationLiquidity:'300',vesting:'100',treasury:'100',unallocated:'100'});assert(Object.isFrozen(inventoryBridgeV2TermsComponents));assert(inventoryBridgeV2TermsComponents.every(Object.isFrozen));
  for(const flag of ['economicTermsAdopted','nativeAssetRelationshipVerified','finalityVerified','activationVerified','simulationVerified','reserveAccountingVerified','endpointCodeVerified','supplyOnchainVerified','allocationExecutionVerified','signed','broadcast','productionReady','productionQualified'])assert.equal(result[flag],false);
 }finally{globalThis.fetch=oldFetch;}
});

test('byte mutation, missing external authority and ambiguous duplicate/noncanonical JSON fail before adoption',async()=>{
 for(const field of ['configurationBytes','reviewBytes','proposalBytes']){const input=pack(await fixture());input[field]=Buffer.concat([input[field],Buffer.from(' ')]);await assert.rejects(verifyInventoryBridgeV2Proposal(input),/SHA256/);}
 for(const field of ['configurationSha256','reviewSha256','proposalSha256']){const input=pack(await fixture());input[field]='0'.repeat(64);await assert.rejects(verifyInventoryBridgeV2Proposal(input),/SHA256/);}
 const input=pack(await fixture());input.proposalBytes=Buffer.from('{"format":"shadow","format":"nano-doppler-inventory-bridge-v2-proposal-v1"}\n');input.proposalSha256=hash(input.proposalBytes);await assert.rejects(verifyInventoryBridgeV2Proposal(input),/canonical JSON/);
 const absent=pack(await fixture());delete absent.proposalSha256;await assert.rejects(verifyInventoryBridgeV2Proposal(absent),/exact fields/);
});

test('all preissued allocations including vesting treasury migration and unsold scope stay covered exactly once',async()=>{
 for(const edit of [
  f=>delete f.proposal.allocations.vesting,
  f=>f.proposal.allocations.treasury='0',
  f=>f.proposal.allocations.unsold='400',
  f=>{f.proposal.allocations.auctionSale='500';f.proposal.allocations.unallocated='0';},
  f=>f.proposal.unsoldTreatment.maximumBaseUnits='100',
  f=>f.proposal.unsoldTreatment.coverage='release-without-proof',
  f=>f.proposal.entireSupplyPolicy.scope='only-auction-sale',
  f=>f.proposal.entireSupplyPolicy.reserveNativeUnits='1400',
  f=>{f.proposal.terms.totalBaseSupply='400';f.proposal.termsHash=inventoryBridgeV2TermsHash(f.proposal.terms);f.proposal.entireSupplyPolicy.reserveNativeUnits='1400';}
 ]){const f=await fixture();edit(f);await assert.rejects(verifyInventoryBridgeV2Proposal(pack(f)));}
});

test('explicit lot conversions refuse dust zero overflow inflation and implicit ratios',async()=>{
 for(const edit of [
  f=>{f.proposal.terms.baseLot='3';f.proposal.termsHash=inventoryBridgeV2TermsHash(f.proposal.terms);},
  f=>f.proposal.terms.baseLot='0',
  f=>delete f.proposal.terms.nativeLot,
  f=>{f.proposal.terms.nativeLot=((1n<<256n)-1n).toString();f.proposal.termsHash=inventoryBridgeV2TermsHash(f.proposal.terms);},
  f=>f.configuration.yearlyMintRate='1',
  f=>f.proposal.entireSupplyPolicy.futureMinting='automatically-expand-reserve'
 ]){const f=await fixture();edit(f);await assert.rejects(verifyInventoryBridgeV2Proposal(pack(f)));}
});

test('self-consistent external sidecar cannot replace SDK predicted token calldata chain or reviewed terms',async()=>{
 for(const edit of [
  f=>f.proposal.launch.predictedToken=address('5'),
  f=>{f.proposal.terms.baseToken=address('5');f.proposal.termsHash=inventoryBridgeV2TermsHash(f.proposal.terms);f.proposal.routeId=inventoryBridgeV2RouteId(f.proposal.terms);},
  f=>f.proposal.launch.calldataHash=domain('5'),
  f=>f.proposal.launch.chainId=1,
  f=>f.proposal.terms.baseChainId='1',
  f=>f.proposal.terms.nativeDomain=f.proposal.terms.baseDomain,
  f=>f.proposal.terms.nativeAsset=address('8'),
  f=>f.proposal.terms.baseToNativeVerifierDomain=domain('0'),
  f=>f.proposal.terms.baseToNativeVerifierDomain=f.proposal.terms.nativeToBaseVerifierDomain,
  f=>f.proposal.terms.maxProofBytes='0',
  f=>f.proposal.terms.maxProofBytes='1048577',
  f=>f.proposal.routeId=domain('8'),
  f=>f.review.transaction.data+='00',
  f=>f.review.predicted.tokenAddress=address('5'),
  f=>f.review.transaction.chainId=1,
  f=>f.review.productionReady=true,
  f=>f.proposal.status='ADOPTED',
  f=>f.proposal.sdkVersion='unreviewed'
 ]){const f=await fixture();edit(f);await assert.rejects(verifyInventoryBridgeV2Proposal(pack(f)));}
 const f=await fixture(),input=pack(f),p=JSON.parse(input.proposalBytes);p.configurationSha256='f'.repeat(64);input.proposalBytes=encode(p);input.proposalSha256=hash(input.proposalBytes);await assert.rejects(verifyInventoryBridgeV2Proposal(input),/externally pinned/);
});

test('sidecar inputs detach before SDK await and retained simulation cannot promote real claims',async()=>{
 const f=await fixture();f.review.simulation={performed:true,success:true,referenceHashPinned:true};const input=pack(f),retained=structuredClone(input),pending=verifyInventoryBridgeV2Proposal(input);
 input.configurationBytes.fill(0);input.reviewBytes.fill(0);input.proposalBytes.fill(0);input.proposalSha256='0'.repeat(64);
 const result=await pending;assert.equal(result.proposalSha256,retained.proposalSha256);assert.equal(result.simulationVerified,false);assert.equal(result.nativeAssetRelationshipVerified,false);assert.equal(result.productionReady,false);
 const rejected=await fixture();rejected.review.status='simulation-rejected';rejected.review.simulation={performed:true,success:false,error:'retained synthetic rejection'};const preserved=await verifyInventoryBridgeV2Proposal(pack(rejected));assert.equal(preserved.retainedReviewStatus,'simulation-rejected');assert.equal(preserved.simulationVerified,false);assert.equal(preserved.productionReady,false);
});
