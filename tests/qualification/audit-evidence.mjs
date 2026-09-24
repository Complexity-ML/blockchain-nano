import {createHash} from 'node:crypto';
import {createReadStream} from 'node:fs';
import {readFile,readdir,realpath} from 'node:fs/promises';
import {resolve,relative,dirname,isAbsolute,sep} from 'node:path';
import {fileURLToPath} from 'node:url';

export const pairs = {
  transitionBlockInitialResults:'transitionBlockCorrectedResults',
  pragueCalldataHistoricalResults:'pragueStateResults',pragueSpecificHistoricalResults:'pragueStateResults',
  cancunBaselineResults:'cancunStateResults', cancunPartialResults:'cancunStateResults',
  cancunBlobResults:'cancunStateResults', cancunKzgResults:'cancunStateResults',
  pragueInitialResults:'pragueStateResults', pragueBlsResults:'pragueStateResults',
  pragueAuthorizationInitialResults:'pragueStateResults', transactionInitialResults:'transactionResults',
  blockExecutionInitialResults:'blockWithdrawalResults', blockRejectionResults:'blockWithdrawalResults',
  blockParentHeaderResults:'blockWithdrawalResults', cancunBlockInitialResults:'cancunBlockCorrectedResults',
  blobHeaderInitialResults:'blobHeaderCorrectedResults', blobHeaderAdapterErrors:'blobHeaderCorrectedResults',
  pragueBlockInitialResults:'pragueBlockCorrectedResults', osakaBlockInitialResults:'osakaBlockCorrectedResults'
};
export async function digest(path){
  const hash=createHash('sha256');for await(const chunk of createReadStream(path))hash.update(chunk);return hash.digest('hex');
}
export function parseReport(text){
  const rows=[],issues=[];let summary;
  const lines=text.split('\n');if(lines.at(-1)==='')lines.pop();
  for(let i=0;i<lines.length;i++){
    let row;try{row=JSON.parse(lines[i]);}catch{issues.push(`invalid JSON at line ${i+1}`);continue;}
    if(row.summary){if(summary)issues.push('duplicate summary');summary=row.summary;if(i!==lines.length-1)issues.push('summary is not final');}
    else if(typeof row.case==='string'&&typeof row.status==='string')rows.push(row);
    else issues.push(`unrecognized record at line ${i+1}`);
  }
  if(!summary)issues.push('missing summary');
  const index=new Map(),counts={matched:0,mismatched:0,unsupported:0,errors:0};
  for(const row of rows){
    const fork=row.fork??row.case.match(/(?:\[|-)fork_([A-Za-z0-9]+)(?=-|\])/)?.[1]??'';
    const key=JSON.stringify([row.case,fork,row.variant??0]);
    if(index.has(key))issues.push(`duplicate case identity: ${key}`);index.set(key,row);
    if(row.status==='matched'){
      const bad=message=>issues.push(`inconsistent matched record ${key}: ${message}`);
      for(const field of ['postStateMatches','lastBlockHashMatches','receiptMatches','gasUsedMatches','transactionHashMatches'])if(row[field]===false)bad(field);
      for(const field of ['stateRoot','logsHash'])if(row[field]&&row[field].actual!==row[field].expected)bad(field);
      if(row.gasUsed!==undefined&&row.expectedGasUsed!==undefined){try{if(BigInt(row.gasUsed)!==BigInt(row.expectedGasUsed))bad('gasUsed');}catch{bad('invalid gas quantity');}}
      const rejections=[...(row.rejections??[]),...(row.validation?[row.validation]:[])];
      for(const rejection of rejections){
        if(typeof rejection.actual!=='string'||!rejection.actual||typeof rejection.expected!=='string'||!rejection.expected.split('|').includes(rejection.actual))bad('wrong rejection reason');
        if(rejection.stateUnchanged!==true||rejection.transactionExecuted===true||rejection.blockAccepted===true||rejection.matched===false)bad('rejection accepted or changed state');
      }
      counts.matched++;
    }
    else if(row.status==='mismatch')counts.mismatched++;
    else if(row.status.startsWith('unsupported'))counts.unsupported++;
    else if(row.status==='execution-or-adapter-error')counts.errors++;
    else issues.push(`unrecognized status: ${row.status}`);
  }
  if(summary)for(const [key,count] of Object.entries(counts))if(summary[key]!==count)issues.push(`summary ${key}: declared ${summary[key]}, observed ${count}`);
  return {rows,index,counts,summary,issues};
}
export function compareCases(before,after){
  let failures=0,matchedAfter=0;const unresolved=[];
  for(const [key,row] of before.index){
    if(row.status==='matched')continue;failures++;
    if(after.index.get(key)?.status==='matched')matchedAfter++;
    else unresolved.push({identity:JSON.parse(key),before:row.status,after:after.index.get(key)?.status??'missing'});
  }
  return {failures,matchedAfter,unresolved,reportIssues:[...before.issues,...after.issues],
    coverageVerified:before.issues.length===0&&after.issues.length===0&&unresolved.length===0};
}
export async function verifyArtifactIndex(root, descriptor){
  const result={path:descriptor?.path,status:'invalid-index',checked:0,issues:[]};
  const inside=(base,path)=>{const rel=relative(base,path);return rel!==''&&!rel.startsWith('..'+sep)&&rel!=='..'&&!isAbsolute(rel);};
  let indexPath,campaign,index;
  try{
    if(typeof descriptor?.path!=='string'||isAbsolute(descriptor.path)||!inside(resolve(root),resolve(root,descriptor.path)))throw Error('index path outside repository');
    if(!/^[0-9a-f]{64}$/.test(descriptor.sha256??''))throw Error('missing or invalid index hash');
    indexPath=resolve(root,descriptor.path);campaign=dirname(indexPath);
    const realRoot=await realpath(root);if(!inside(realRoot,await realpath(indexPath)))throw Error('index symlink outside repository');
    if(await digest(indexPath)!==descriptor.sha256){result.status='hash-mismatch';return result;}
    index=JSON.parse(await readFile(indexPath,'utf8'));if(!Array.isArray(index)||!index.length)throw Error('empty or invalid artifact index');
  }catch(error){result.issues.push(error.message);return result;}
  const listed=new Set();
  for(const entry of index){
    try{
      if(typeof entry?.path!=='string'||isAbsolute(entry.path))throw Error('invalid artifact path');
      const path=resolve(root,entry.path);
      if(!inside(campaign,path)||path===indexPath)throw Error('artifact outside campaign or index includes itself');
      if(listed.has(path))throw Error('duplicate artifact');listed.add(path);
      if(!/^[0-9a-f]{64}$/.test(entry.sha256??''))throw Error('missing or invalid artifact hash');
      if(!inside(await realpath(campaign),await realpath(path)))throw Error('artifact symlink outside campaign');
      if(await digest(path)!==entry.sha256)throw Error('artifact hash mismatch');
      result.checked++;
    }catch(error){result.issues.push({path:entry?.path??null,error:error.message});}
  }
  async function inventory(dir){
    for(const entry of await readdir(dir,{withFileTypes:true})){
      const path=resolve(dir,entry.name);if(path===indexPath)continue;
      if(entry.isDirectory())await inventory(path);
      else if(entry.isSymbolicLink())result.issues.push({path:relative(root,path),error:'symlink in artifact inventory'});
      else if(!listed.has(path))result.issues.push({path:relative(root,path),error:'unindexed artifact'});
    }
  }
  try{await inventory(campaign);}catch(error){result.issues.push(error.message);}
  result.status=result.issues.length?'invalid-artifacts':'verified';return result;
}
export async function audit(root){
  const manifestPath=resolve(root,'tests/fixtures/ethereum-official.json');
  const manifest=JSON.parse(await readFile(manifestPath,'utf8'));
  const dir=resolve(root,'build-cmake/qualification'),inventory=[];
  for(const entry of (await readdir(dir,{withFileTypes:true})).sort((a,b)=>a.name.localeCompare(b.name))){
    if(entry.isFile()&&/\.(jsonl|log)$/.test(entry.name)){
      const path=resolve(dir,entry.name);inventory.push({path:relative(root,path),sha256:await digest(path)});
    }
  }
  const references=[],indexDescriptors=[];
  function visit(value,path=[]){
    if(!value||typeof value!=='object')return;
    if(path.at(-1)==='artifactIndex')indexDescriptors.push({entry:path.join('.'),descriptor:value});
    if(path.at(-1)==='artifactIndices'&&Array.isArray(value))value.forEach((descriptor,i)=>indexDescriptors.push({entry:[...path,i].join('.'),descriptor}));
    if(!Array.isArray(value)&&(typeof value.report==='string'||typeof value.reportSha256==='string')){
      const named=typeof value.report==='string'?inventory.find(f=>f.path===value.report):undefined;
      const candidates=value.report?named?[named]:[]:inventory.filter(f=>f.sha256===value.reportSha256);
      const status=!value.reportSha256?'missing-expected-hash':!candidates.length?'missing-report':candidates.some(f=>f.sha256!==value.reportSha256)?'hash-mismatch':'verified';
      references.push({entry:path.join('.'),status,expectedSha256:value.reportSha256??null,candidates});
    }
    for(const [key,child] of Object.entries(value))visit(child,[...path,key]);
  }
  visit(manifest);
  const artifactIndices=[];for(const {entry,descriptor} of indexDescriptors)artifactIndices.push({entry,...await verifyArtifactIndex(root,descriptor)});
  const comparisons=[];const cache=new Map();
  async function report(ref){const path=ref.candidates[0].path;if(!cache.has(path))cache.set(path,parseReport(await readFile(resolve(root,path),'utf8')));return cache.get(path);}
  for(const [before,after] of Object.entries(pairs)){
    const a=references.find(r=>r.entry===before),b=references.find(r=>r.entry===after);
    if(a?.status!=='verified'||b?.status!=='verified'){
      comparisons.push({before,after,coverageVerified:false,reason:'missing or unverifiable evidence',beforeEvidence:a?.status??'no-report-reference',afterEvidence:b?.status??'no-report-reference'});continue;
    }
    comparisons.push({before,after,...compareCases(await report(a),await report(b))});
  }
  const referenced=new Set(references.flatMap(r=>r.candidates.map(f=>f.path)));
  const unreferencedReports=[];
  for(const item of inventory.filter(f=>!referenced.has(f.path))){
    const text=await readFile(resolve(root,item.path),'utf8');
    if(item.path.endsWith('.jsonl')){
      const parsed=parseReport(text);unreferencedReports.push({...item,counts:parsed.counts,issues:parsed.issues,complete:parsed.issues.length===0});
    }else{
      const signals=text.split('\n').filter(line=>/\bFAILED\b|\*\*\*Timeout|No rule to make target|fatal error:|error:|SIGSEGV|Segmentation fault/.test(line));
      unreferencedReports.push({...item,failureSignalCount:signals.length,failureSignals:signals.slice(0,10),note:'Text signals require manual review; their absence does not prove success'});
    }
  }
  return {scope:'Historical report/index/artifact integrity and case-identity coverage only; not a rerun, code-cause audit, fixture-content verification, or production approval',
    manifestSha256:await digest(manifestPath),references,artifactIndices,comparisons,unreferencedReports,
    pending:['Cause and code-review evidence for each failure','Current-binary replay and build provenance','Older reports without references','BFT/storage/network/RPC/Solidity/CI failure inventory','Anvil Shanghai admission remains divergent'],
    complete:false};
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const result=await audit(resolve(process.argv[2]??dirname(fileURLToPath(import.meta.url))+'/../..'));
  process.stdout.write(JSON.stringify(result,null,2)+'\n');
  process.exitCode=result.artifactIndices.some(i=>i.status!=='verified')||result.references.some(r=>r.status!=='verified')||result.comparisons.some(c=>!c.coverageVerified)||result.unreferencedReports.some(r=>r.failureSignalCount||r.counts?.errors||r.counts?.mismatched||r.counts?.unsupported||r.issues?.length)?1:0;
}
