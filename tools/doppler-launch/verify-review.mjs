// Offline encoding equality only. No wallet, RPC, signing or broadcast path.
import {constants} from 'node:fs';
import {open,writeFile,lstat,realpath} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {isDeepStrictEqual} from 'node:util';
import {pathToFileURL} from 'node:url';
import {preview} from './preview.mjs';

const need=(ok,why)=>{if(!ok)throw Error('Unsigned review: '+why);};
const exact=(o,keys)=>need(o&&typeof o==='object'&&!Array.isArray(o)&&
 Object.keys(o).sort().join('\0')===[...keys].sort().join('\0'),'exact artifact fields');
const jsonValue=v=>{
 if(typeof v==='bigint')return v.toString();
 if(v===null||typeof v==='string'||typeof v==='boolean')return v;
 if(typeof v==='number'){need(Number.isFinite(v),'finite JSON number');return v;}
 if(Array.isArray(v))return v.map(jsonValue);
 need(v&&typeof v==='object'&&Object.getPrototypeOf(v)===Object.prototype&&Object.getOwnPropertySymbols(v).length===0,'plain JSON value');
 return Object.fromEntries(Object.entries(v).map(([k,x])=>[k,jsonValue(x)]));
};

export async function verifyUnsignedReview(review,approvedConfiguration){
 // Both inputs belong to the caller; detach before the SDK's async salt work.
 const retained=structuredClone(review),configuration=structuredClone(approvedConfiguration);
 exact(retained,['schema','sdkVersion','status','productionReady','nativeAssetRelationship','config',
   'transaction','calldataHash','predicted','createParams','simulation']);
 need(retained.schema===1&&retained.sdkVersion==='1.0.43'&&retained.productionReady===false,'unsupported schema/SDK/readiness');
 need(['unsigned-preview','simulation-rejected'].includes(retained.status),'known preview status');
 need(retained.simulation&&typeof retained.simulation==='object'&&!Array.isArray(retained.simulation)&&
   typeof retained.simulation.performed==='boolean','retained simulation claim');
 need(isDeepStrictEqual(jsonValue(retained.config),jsonValue(configuration)),'configuration differs from approved terms');
 // The existing preview is deterministic offline and refuses any attempted RPC.
 const expected=JSON.parse(JSON.stringify(await preview(configuration),(_,v)=>typeof v==='bigint'?v.toString():v));
 for(const field of ['nativeAssetRelationship','transaction','calldataHash','predicted','createParams'])
   need(isDeepStrictEqual(jsonValue(retained[field]),jsonValue(expected[field])),'encoding mismatch: '+field);
 return {format:'nano-doppler-offline-review-verification-v1',reviewIntegrityVerified:true,
   sdkVersion:expected.sdkVersion,chainId:8453,calldataHash:expected.calldataHash,
   transaction:structuredClone(expected.transaction),predicted:structuredClone(expected.predicted),
   retainedStatus:retained.status,simulationVerified:false,nativeAssetRelationshipVerified:false,
   signed:false,broadcast:false,productionReady:false};
}

const F=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const row=s=>F.map(k=>String(s[k]));
async function pinnedJson(file,sha256){
 need(typeof sha256==='string'&&/^[0-9a-f]{64}$/.test(sha256),'externally approved file SHA required');
 need(await realpath(file)===file,'physical input path');
 const fd=await open(file,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{
  const before=await fd.stat({bigint:true}),identity=row(before);
  need(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=1048576n,'bounded regular JSON file');
  need(isDeepStrictEqual(identity,row(await lstat(file,{bigint:true}))),'input FD/path identity');
  const h=createHash('sha256'),chunks=[];let count=0;
  for(;;){const b=Buffer.alloc(65536),{bytesRead:n}=await fd.read(b,0,b.length,null);if(!n)break;
   count+=n;need(BigInt(count)<=before.size,'input grew');h.update(b.subarray(0,n));chunks.push(b.subarray(0,n));}
  need(BigInt(count)===before.size&&h.digest('hex')===sha256&&
   isDeepStrictEqual(identity,row(await fd.stat({bigint:true})))&&isDeepStrictEqual(identity,row(await lstat(file,{bigint:true})))&&
   await realpath(file)===file,'input SHA or identity changed');
  return JSON.parse(Buffer.concat(chunks).toString('utf8'));
 }finally{await fd.close();}
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 try{
  const [configFile,configSha,reviewFile,reviewSha,output,...extra]=process.argv.slice(2);
  need(configFile&&configSha&&reviewFile&&reviewSha&&output&&!extra.length,
   'Usage: node verify-review.mjs APPROVED_CONFIG CONFIG_SHA REVIEW REVIEW_SHA NEW_OUTPUT');
  const configuration=await pinnedJson(configFile,configSha),review=await pinnedJson(reviewFile,reviewSha);
  const result=await verifyUnsignedReview(review,configuration);
  await writeFile(output,JSON.stringify({...result,approvedConfigurationSha256:configSha,reviewSha256:reviewSha},null,2)+'\n',{flag:'wx',mode:0o600});
  console.log('Offline review encoding verified; simulation and asset relationship remain unverified');
 }catch(e){console.error(e.message);process.exitCode=1;}
}
