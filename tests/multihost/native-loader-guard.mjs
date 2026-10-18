// Versioned, injected-I/O guard. No inspector, exporter, environment or FS API.
import {posix} from 'node:path';

export const c8CampaignFormat='nano-fresh-capella-three-owner-campaign-c8-v2';
const legacyFormat='nano-fresh-capella-three-owner-campaign-v1';
const fields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
const leaf='libnano-test-sync-fault.dylib',installName='@rpath/'+leaf;
const need=(ok,why)=>{if(!ok)throw Error('Native exporter loader: '+why);};
const same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
function exact(value,keys,label){
 need(value!==null&&typeof value==='object'&&!Array.isArray(value)&&Object.keys(value).sort().join('\0')===[...keys].sort().join('\0'),label+' exact fields');
}
function path(value){
 need(typeof value==='string'&&value.length>1&&value.length<4096&&value.startsWith('/')&&!value.endsWith('/')&&posix.normalize(value)===value&&!/[\0\r\n,\\]/.test(value),'canonical absolute path');return value;
}
function artifact(value,label,maximum=1073741824){
 exact(value,['path','sha256','bytes'],label);path(value.path);
 need(typeof value.sha256==='string'&&/^[0-9a-f]{64}$/.test(value.sha256)&&Number.isSafeInteger(value.bytes)&&value.bytes>0&&value.bytes<=maximum,label+' known SHA/bytes');return value;
}
function material(value,pin,owner,executable){
 need(Array.isArray(value)&&value.length===9,'original material has nine strings');
 for(let i=0;i<value.length;i++){
  const item=value[i];need(typeof item==='string'&&item.length<=21&&(i<7?/^(0|[1-9][0-9]*)$/:/^(0|-?[1-9][0-9]*)$/).test(item),'original '+fields[i]+' canonical decimal string');
  const n=BigInt(item);need(i<7?n<=18446744073709551615n:n>=-9223372036854775808n&&n<=9223372036854775807n,'original '+fields[i]+' integer range');
 }
 const mode=BigInt(value[2]);
 need((mode&61440n)===32768n&&value[5]==='1'&&value[6]===String(pin.bytes),'original unique regular file/size');
 need(value[3]===String(owner.uid)&&value[4]===String(owner.gid),'original native owner');
 need(!executable||(mode&73n)!==0n,'original exporter executable');return value;
}
function ownedArtifact(value,label,owner,executable){
 exact(value,['path','sha256','bytes','originalMaterial'],label);
 const pin=artifact({path:value.path,sha256:value.sha256,bytes:value.bytes},label);
 material(value.originalMaterial,pin,owner,executable);return value;
}
function parse(bytes,maximum,label){
 need(Buffer.isBuffer(bytes)&&bytes.length>0&&bytes.length<=maximum,label+' bounded bytes');
 try{return JSON.parse(bytes.toString('utf8'));}catch{throw Error('Native exporter loader: '+label+' JSON');}
}
const pinOf=value=>({path:value.path,sha256:value.sha256,bytes:value.bytes});

// The old campaign remains byte-only for its old overlay. Selection of C8 v2
// is explicit in the externally approved campaign, never inferred from a file.
export function selectNativeExporterLoaderGuard(spec,overlayBytes,{readArtifact}={}){
 if(spec.format===legacyFormat)return null;
 need(spec.format===c8CampaignFormat,'explicit supported campaign generation');
 need(typeof readArtifact==='function','injected held artifact reader');
 const owner=spec.owners?.['mac-native'];
 need(owner&&owner.uid===501&&owner.gid===20,'Mac native owner');
 const envelope=structuredClone(parse(overlayBytes,16777216,'C8 overlay'));
 exact(envelope,['format','sourceBaseRevision','overlayManifest','nativeExporterLoaderClosure'],'C8 overlay');
 need(envelope.format==='nano-root-fresh-capella-c8-overlay-v1'&&envelope.sourceBaseRevision===spec.sourceBaseRevision,'C8 overlay/source generation');
 artifact(envelope.overlayManifest,'original overlay',16777216);
 const closure=envelope.nativeExporterLoaderClosure;
 exact(closure,['format','exporter','shim','structuralReceipt'],'native closure');
 need(closure.format==='nano-native-exporter-loader-closure-v1','native closure generation');
 ownedArtifact(closure.exporter,'exporter',owner,true);ownedArtifact(closure.shim,'shim',owner,false);
 need(['path','sha256','bytes'].every(k=>closure.exporter[k]===owner.programs.exporter[k]),'closure binds selected exporter path/SHA/bytes');
 need(posix.basename(closure.exporter.path)==='nano-capella-transition-tests','exact exporter leaf');
 need(closure.shim.path===posix.join(posix.dirname(closure.exporter.path),leaf),'exact colocated shim path');
 artifact(closure.structuralReceipt,'Root structural receipt',1048576);
 const paths=[spec.overlayManifest.path,envelope.overlayManifest.path,closure.structuralReceipt.path,closure.exporter.path,closure.shim.path];
 need(new Set(paths).size===paths.length,'distinct manifest/receipt/software files');
 const within=(a,b)=>a===b||a.startsWith(b+'/');
 for(const file of paths)for(const protectedPath of [spec.fixture.seedFile,spec.fixture.ledgerDirectory])need(!within(file,protectedPath)&&!within(protectedPath,file),'loader authority disjoint from fixture');
 const expectedReceipt={
  format:'nano-root-native-exporter-loader-relation-v1',issuer:'ROOT',
  exporter:pinOf(closure.exporter),shim:pinOf(closure.shim),architecture:'arm64',
  loadCommand:'LC_LOAD_DYLIB',dependencyInstallName:installName,shimInstallName:installName,
  firstRpath:'@loader_path',inspectionPassed:true,globalDyldClosureQualified:false,productionQualified:false
 };
 async function verify(){
  // The original overlay stays independently held; it is never overwritten.
  await readArtifact(envelope.overlayManifest,{body:true});
  const receipt=parse(await readArtifact(closure.structuralReceipt,{body:true}),1048576,'Root structural receipt');
  exact(receipt,Object.keys(expectedReceipt),'Root structural receipt');
  exact(receipt.exporter,['path','sha256','bytes'],'receipt exporter');exact(receipt.shim,['path','sha256','bytes'],'receipt shim');
  // Reorder only for comparison, without accepting unknown keys or coercions.
  for(const key of Object.keys(expectedReceipt)){
   const expected=expectedReceipt[key],actual=receipt[key];
   need(typeof expected==='object'?Object.keys(expected).every(k=>actual[k]===expected[k]):actual===expected,'Root structural relation '+key);
  }
  for(const value of [closure.exporter,closure.shim]){
   const row=await readArtifact(pinOf(value),{body:false});
   exact(row,['path','sha256','bytes','material'],'live software observation');
   need(row.path===value.path&&row.sha256===value.sha256&&row.bytes===value.bytes,'live software known bytes');
   need(same(row.material,value.originalMaterial),'live software original full-nine authority');
  }
 }
 async function aroundExporter(launch){
  need(typeof launch==='function','launch callback');let primary,failed=false,result;
  try{await verify();result=await launch();}catch(error){primary=error;failed=true;}
  try{await verify();}catch(error){
   if(!failed){primary=error;failed=true;}
   else if(primary!==null&&typeof primary==='object')try{Object.defineProperty(primary,'nativeLoaderSecondaryFailure',{value:String(error),configurable:true});}catch{}
  }
  if(failed)throw primary;return result;
 }
 return Object.freeze({verify,aroundExporter});
}
