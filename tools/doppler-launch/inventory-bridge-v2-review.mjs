// Offline proposal/encoding join only. No filesystem, wallet, RPC or verifier.
// This new sidecar deliberately does not change preview/verify-review schema1.
import {createHash} from 'node:crypto';
import {isDeepStrictEqual} from 'node:util';
import {encodeAbiParameters,getAddress,keccak256,stringToHex,zeroAddress} from 'viem';
import {verifyUnsignedReview} from './verify-review.mjs';

const U256=(1n<<256n)-1n,BASE=8453;
const fail=why=>{throw Error('Inventory bridge v2 proposal: '+why);};
const need=(ok,why)=>{if(!ok)fail(why);};
const exact=(v,keys,label)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join('\0')===[...keys].sort().join('\0'),label+' exact fields');
const sha=b=>createHash('sha256').update(b).digest('hex');
const amount=(v,label,positive=false)=>{need(typeof v==='string'&&/^(0|[1-9][0-9]*)$/.test(v),label+' canonical atomic decimal');const n=BigInt(v);need(n<=U256&&(!positive||n>0n),label+' uint256 range');return n;};
const addr=(v,label)=>{need(typeof v==='string'&&/^0x[0-9a-fA-F]{40}$/.test(v),label+' address');let a;try{a=getAddress(v);}catch{fail(label+' checksum');}need(a!==zeroAddress,label+' nonzero address');return a;};
const domain=(v,label)=>need(typeof v==='string'&&/^0x[0-9a-f]{64}$/.test(v)&&v!=='0x'+'0'.repeat(64),label+' nonzero bytes32');
const pinned=(bytes,digest,label)=>{
 need(Buffer.isBuffer(bytes)&&bytes.length>0&&bytes.length<=1048576,label+' bounded bytes');need(typeof digest==='string'&&/^[0-9a-f]{64}$/.test(digest),label+' external SHA256');
 const retained=Buffer.from(bytes);need(sha(retained)===digest,label+' SHA256 mismatch');let value;try{value=JSON.parse(retained.toString('utf8'));}catch{fail(label+' JSON');}
 need(retained.equals(Buffer.from(JSON.stringify(value,null,2)+'\n')),label+' canonical JSON bytes');return value;
};

// Keep this explicit tuple synchronized with the separate Solidity prototype.
// Units are ATOMIC lots; descriptive decimals never enter the conversion.
export const inventoryBridgeV2TermsType='NanoInventoryTermsV2(uint256 baseChainId,bytes32 baseDomain,address baseEndpoint,address baseToken,uint256 nativeChainId,bytes32 nativeDomain,address nativeEndpoint,address nativeAsset,uint256 totalBaseSupply,uint256 baseLot,uint256 nativeLot,bytes32 baseToNativeVerifierDomain,bytes32 nativeToBaseVerifierDomain,uint256 maxProofBytes)';
export const inventoryBridgeV2RouteType='NanoInventoryRouteV2(uint256 baseChainId,bytes32 baseDomain,address baseEndpoint,address baseToken,uint256 nativeChainId,bytes32 nativeDomain,address nativeEndpoint,address nativeAsset)';
export const inventoryBridgeV2TermsComponents=Object.freeze([
 {name:'baseChainId',type:'uint256'},{name:'baseDomain',type:'bytes32'},{name:'baseEndpoint',type:'address'},{name:'baseToken',type:'address'},
 {name:'nativeChainId',type:'uint256'},{name:'nativeDomain',type:'bytes32'},{name:'nativeEndpoint',type:'address'},{name:'nativeAsset',type:'address'},
 {name:'totalBaseSupply',type:'uint256'},{name:'baseLot',type:'uint256'},{name:'nativeLot',type:'uint256'},
 {name:'baseToNativeVerifierDomain',type:'bytes32'},{name:'nativeToBaseVerifierDomain',type:'bytes32'},{name:'maxProofBytes',type:'uint256'}
].map(Object.freeze));
function checkedTerms(value){
 const terms=structuredClone(value);exact(terms,inventoryBridgeV2TermsComponents.map(x=>x.name),'terms');
 for(const key of ['baseToken','baseEndpoint','nativeEndpoint'])addr(terms[key],key);
 need(terms.nativeAsset===zeroAddress,'native asset is native currency address(0), not a wrapped ERC20');
 for(const key of ['totalBaseSupply','baseLot','nativeLot','baseChainId','nativeChainId','maxProofBytes'])amount(terms[key],key,true);
 domain(terms.baseToNativeVerifierDomain,'Base-to-Native verifier domain');domain(terms.nativeToBaseVerifierDomain,'Native-to-Base verifier domain');
 need(terms.baseToNativeVerifierDomain!==terms.nativeToBaseVerifierDomain,'distinct verifier domains');need(BigInt(terms.maxProofBytes)<=1048576n,'prototype maximum proof bytes');
 domain(terms.baseDomain,'base domain');domain(terms.nativeDomain,'native domain');need(terms.baseDomain!==terms.nativeDomain,'distinct chain domains');need(BigInt(terms.baseChainId)===BigInt(BASE),'Base chain8453');
 const converted={...terms};for(const {name,type}of inventoryBridgeV2TermsComponents)if(type==='uint256')converted[name]=BigInt(terms[name]);
 return converted;
}
export function inventoryBridgeV2TermsHash(value){
 return keccak256(encodeAbiParameters([{type:'bytes32'},{type:'tuple',components:inventoryBridgeV2TermsComponents}],[keccak256(stringToHex(inventoryBridgeV2TermsType)),checkedTerms(value)]));
}
export function inventoryBridgeV2RouteId(value){
 const terms=checkedTerms(value),components=inventoryBridgeV2TermsComponents.slice(0,8);
 return keccak256(encodeAbiParameters([{type:'bytes32'},...components],[keccak256(stringToHex(inventoryBridgeV2RouteType)),...components.map(({name})=>terms[name])]));
}

export async function verifyInventoryBridgeV2Proposal(input){
 exact(input,['configurationBytes','configurationSha256','reviewBytes','reviewSha256','proposalBytes','proposalSha256'],'inputs');
 // Copy and check all caller-owned bytes before the first SDK await.
 const config=pinned(input.configurationBytes,input.configurationSha256,'configuration'),review=pinned(input.reviewBytes,input.reviewSha256,'review'),proposal=pinned(input.proposalBytes,input.proposalSha256,'proposal');
 const pins={configurationSha256:input.configurationSha256,reviewSha256:input.reviewSha256,proposalSha256:input.proposalSha256};
 exact(proposal,['format','status','sdkVersion','configurationSha256','reviewSha256','launch','terms','termsHash','routeId','entireSupplyPolicy','allocations','unsoldTreatment'],'proposal');
 need(proposal.format==='nano-doppler-inventory-bridge-v2-proposal-v1'&&proposal.status==='PROPOSED_NOT_ADOPTED'&&proposal.sdkVersion==='1.0.43','proposal generation/SDK/not adopted');
 need(proposal.configurationSha256===pins.configurationSha256&&proposal.reviewSha256===pins.reviewSha256,'proposal binds externally pinned config and review');
 exact(proposal.launch,['chainId','predictedToken','calldataHash'],'launch');need(proposal.launch.chainId===BASE&&config.chainId===BASE,'exact Base launch chain');
 need(config.yearlyMintRate==='0','future minting/inflation is unsupported; fixed whole supply required');
 const termsHash=inventoryBridgeV2TermsHash(proposal.terms),routeId=inventoryBridgeV2RouteId(proposal.terms);need(proposal.termsHash===termsHash&&proposal.routeId===routeId,'terms and route ABI commitments');
 const terms=proposal.terms,supply=amount(config.initialSupply,'Doppler initialSupply',true),sale=amount(config.numTokensToSell,'Doppler sale',true),baseLot=amount(terms.baseLot,'baseLot',true),nativeLot=amount(terms.nativeLot,'nativeLot',true);
 need(amount(terms.totalBaseSupply,'terms supply',true)===supply,'entire preissued supply, not just sold tokens');need(supply%baseLot===0n,'whole initial supply must contain exact base lots');
 const lots=supply/baseLot,nativeCoverage=lots*nativeLot;need(nativeCoverage<=U256,'native inventory uint256 coverage');
 exact(proposal.entireSupplyPolicy,['scope','futureMinting','reserveNativeUnits','reserveRelease'],'entire-supply policy');
 need(proposal.entireSupplyPolicy.scope==='entire-preissued-supply'&&proposal.entireSupplyPolicy.futureMinting==='unsupported-refuse'&&proposal.entireSupplyPolicy.reserveRelease==='no-unproven-release'&&amount(proposal.entireSupplyPolicy.reserveNativeUnits,'native coverage',true)===nativeCoverage,'fixed entire-supply reserve policy');
 const categories=['auctionSale','migrationLiquidity','vesting','treasury','unallocated'];exact(proposal.allocations,categories,'declared allocations');const allocations=categories.map(k=>amount(proposal.allocations[k],k));
 need(allocations.reduce((a,b)=>a+b,0n)===supply&&allocations[0]===sale,'all preissued allocations included once; sale matches calldata configuration');
 exact(proposal.unsoldTreatment,['subsetOf','maximumBaseUnits','coverage'],'unsold treatment');need(proposal.unsoldTreatment.subsetOf==='auctionSale'&&amount(proposal.unsoldTreatment.maximumBaseUnits,'maximum unsold')===sale&&proposal.unsoldTreatment.coverage==='included-until-authenticated-settlement','unsold tokens remain covered without double counting');
 const verified=await verifyUnsignedReview(review,config);
 need(verified.sdkVersion==='1.0.43'&&verified.chainId===BASE&&verified.reviewIntegrityVerified===true,'existing offline SDK encoding qualification');
 need(addr(proposal.launch.predictedToken,'launch token')===addr(verified.predicted.tokenAddress,'SDK token')&&addr(terms.baseToken,'terms token')===addr(verified.predicted.tokenAddress,'SDK token'),'terms and proposal bind predicted Base token');
 need(proposal.launch.calldataHash===verified.calldataHash&&isDeepStrictEqual(verified.transaction,review.transaction),'proposal binds reviewed calldata/chain/from/to/value');
 return {format:'nano-doppler-inventory-bridge-v2-offline-join-v1',proposalEncodingVerified:true,unsignedReviewIntegrityVerified:true,sdkVersion:'1.0.43',...pins,retainedReviewStatus:verified.retainedStatus,chainId:BASE,termsHash,routeId,terms:structuredClone(terms),launch:structuredClone(proposal.launch),
  coverage:{scope:'entire-preissued-base-supply-including-out-of-sale',totalBaseUnits:supply.toString(),wholeBaseLots:lots.toString(),nativeUnits:nativeCoverage.toString(),baseLot:baseLot.toString(),nativeLot:nativeLot.toString(),declaredOnly:true},allocations:structuredClone(proposal.allocations),unsoldTreatment:structuredClone(proposal.unsoldTreatment),
  economicTermsAdopted:false,nativeAssetRelationshipVerified:false,finalityVerified:false,activationVerified:false,simulationVerified:false,reserveAccountingVerified:false,endpointCodeVerified:false,supplyOnchainVerified:false,allocationExecutionVerified:false,signed:false,broadcast:false,productionReady:false,productionQualified:false};
}
