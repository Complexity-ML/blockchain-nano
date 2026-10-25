// Independent secure-MPT algorithm using ethers RLP/Keccak only; no Nano import.
// SOURCE proposal. Importing exports performs no file/RPC/process operation.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const need=(ok,why)=>{if(!ok)throw Error('Independent storage oracle: '+why);};
const bytes=s=>typeof s==='string'&&/^0x(?:[0-9a-f]{2})*$/.test(s);
const hash=s=>typeof s==='string'&&/^0x[0-9a-f]{64}$/.test(s);
const address=s=>typeof s==='string'&&/^0x[0-9a-f]{40}$/.test(s);
const word=s=>{need(typeof s==='string'&&/^0x[0-9a-f]{1,64}$/.test(s),'uint256 word');return BigInt(s);};
const quantity=s=>{need(typeof s==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(s)&&s.length<=66,'canonical uint256 quantity');return BigInt(s);};
const q=n=>'0x'+n.toString(16),data32=n=>'0x'+n.toString(16).padStart(64,'0');
const minimal=n=>{if(n===0n)return '0x';let h=n.toString(16);return '0x'+(h.length%2?'0':'')+h;};

export function createIndependentMptOracle(ethers,check=()=>{}){
 need(['encodeRlp','decodeRlp','keccak256','getBytes'].every(k=>typeof ethers[k]==='function'),'explicit ethers RLP/Keccak primitives');
 const length=s=>{need(bytes(s),'hex bytes');return (s.length-2)/2;},nibbles=s=>Array.from(ethers.getBytes(s)).flatMap(b=>[b>>4,b&15]);
 const enc=node=>{check();const raw=ethers.encodeRlp(node);check();return raw;},digest=raw=>{check();const h=ethers.keccak256(raw);check();return h;};
 const EMPTY=digest(enc('0x'));
 function compact(ns,leaf){const a=ns.length%2?[leaf?3:1,...ns]:[leaf?2:0,0,...ns];let h='';for(let i=0;i<a.length;i+=2)h+=(a[i]*16+a[i+1]).toString(16).padStart(2,'0');return '0x'+h;}
 function unpack(raw){need(bytes(raw)&&length(raw)>0,'compact path bytes');const a=nibbles(raw),flag=a[0];need(flag<=3&&(flag%2===1||a[1]===0),'compact path flags/padding');const ns=a.slice(flag%2?1:2);need(flag>=2||ns.length>0,'nonempty extension');return {path:ns,leaf:flag>=2};}
 function buildStorage(storage){
  need(storage!==null&&typeof storage==='object'&&!Array.isArray(storage)&&Object.keys(storage).length<=1024,'bounded Native storage map');const entries=[],seen=new Set(),nodes=new Map();let discarded=0;
  for(const [key,value]of Object.entries(storage)){check();const k=quantity(key),v=quantity(value),secure=digest(data32(k));need(!seen.has(secure),'unique secure storage key');seen.add(secure);if(v===0n){++discarded;continue;}entries.push({path:nibbles(secure),value:enc(minimal(v))});}
  entries.sort((a,b)=>{for(let i=0;i<64;i++)if(a.path[i]!==b.path[i])return a.path[i]-b.path[i];return 0;});
  function node(value){const raw=enc(value),h=digest(raw);nodes.set(h,{hash:h,rlp:raw,bytes:length(raw)});return {value,raw,hash:h};}
  const ref=n=>length(n.raw)<32?n.value:n.hash;
  function build(rows,depth){check();need(depth<=64&&rows.length>0,'bounded trie recursion');if(rows.length===1)return node([compact(rows[0].path.slice(depth),true),rows[0].value]);
   let common=0;while(depth+common<64&&rows.every(r=>r.path[depth+common]===rows[0].path[depth+common]))++common;
   if(common)return node([compact(rows[0].path.slice(depth,depth+common),false),ref(build(rows,depth+common))]);
   need(depth<64,'duplicate terminal key');const children=Array(17).fill('0x');for(let i=0;i<16;i++){const group=rows.filter(r=>r.path[depth]===i);if(group.length)children[i]=ref(build(group,depth+1));}return node(children);
  }
  if(!entries.length)return {root:EMPTY,entries:0,discardedZeroEntries:discarded,rootNodeRlp:enc('0x'),nodes:[]};const tree=build(entries,0);return {root:tree.hash,entries:entries.length,discardedZeroEntries:discarded,rootNodeRlp:tree.raw,nodes:[...nodes.values()]};
 }
 function verifyProof(rootHash,secureKey,proof){
  need(hash(rootHash)&&hash(secureKey)&&Array.isArray(proof)&&proof.length<=128,'bounded root/key/proof');let total=0;const map=new Map(),used=new Set(),visited=new Set();
  for(const raw of proof){check();need(bytes(raw)&&(total+=length(raw))<=1048576,'proof bytes');const decoded=ethers.decodeRlp(raw);need(enc(decoded)===raw&&Array.isArray(decoded),'canonical RLP proof node');const h=digest(raw);need(!map.has(h),'duplicate proof node');map.set(h,{raw,decoded});}
  const finish=(exists,value)=>{need(used.size===map.size,'unrelated/unconsumed proof node');return {exists,value,hashedNodesUsed:used.size,proofBytes:total};};
  if(rootHash===EMPTY){need(map.size===0,'empty trie proof');return finish(false,null);}
  const key=nibbles(secureKey);let remaining=key,reference=rootHash,steps=0;
  for(;;){check();need(++steps<=128,'proof traversal bound');let node;
   if(Array.isArray(reference)){const raw=enc(reference);need(length(raw)<32,'oversized inline trie child');const h=digest(raw);if(map.has(h)){need(map.get(h).raw===raw,'inline proof node identity');used.add(h);}node=reference;}
   else{need(hash(reference)&&!visited.has(reference),'hashed child or repeated proof reference');visited.add(reference);const row=map.get(reference);need(row,'referenced proof hash missing');need(digest(row.raw)===reference,'proof hash binding');used.add(reference);node=row.decoded;}
   need(Array.isArray(node),'trie node list');
   if(node.length===17){need(bytes(node[16]),'branch value');if(!remaining.length)return node[16]==='0x'?finish(false,null):finish(true,node[16]);const next=node[remaining[0]];remaining=remaining.slice(1);if(next==='0x')return finish(false,null);reference=next;continue;}
   need(node.length===2&&bytes(node[0]),'leaf/extension node');const hp=unpack(node[0]),matching=hp.path.every((n,i)=>remaining[i]===n);
   if(!matching)return finish(false,null);remaining=remaining.slice(hp.path.length);
   if(hp.leaf){need(bytes(node[1]),'leaf value bytes');return remaining.length?finish(false,null):finish(true,node[1]);}
   reference=node[1];
  }
 }
 function storageValue(result){if(!result.exists)return 0n;need(bytes(result.value),'storage leaf payload');const decoded=ethers.decodeRlp(result.value);need(bytes(decoded)&&length(decoded)<=32&&enc(decoded)===result.value&&(decoded==='0x'||!decoded.startsWith('0x00')),'canonical RLP integer storage value');const n=decoded==='0x'?0n:BigInt(decoded);need(n!==0n,'zero storage leaf must be absent');return n;}
 function verifyAccount(stateRoot,where,account){
  need(address(where)&&hash(account.storageHash)&&hash(account.codeHash),'account identity/root/hash');const proof=verifyProof(stateRoot,digest(where),account.accountProof),nonce=word(account.nonce),balance=word(account.balance);
  if(proof.exists)need(proof.value===enc([minimal(nonce),minimal(balance),account.storageHash,account.codeHash]),'authenticated account payload differs from reported fields');
  else need(nonce===0n&&balance===0n&&account.storageHash===EMPTY&&account.codeHash===digest('0x'),'absent account reports empty fields');return proof;
 }
 return {emptyRoot:EMPTY,buildStorage,verifyProof,verifyAccount,storageValue,secureStorageKey:k=>digest(data32(quantity(k)))};
}

export function analyzePairedStorage(rows,stateRoot,ethers,check=()=>{}){
 need(Array.isArray(rows)&&rows.length===14&&hash(stateRoot),'fourteen paired accounts and externally retained Anvil state root');const oracle=createIndependentMptOracle(ethers,check),accounts=[],addresses=new Set();let slots=0;
 for(const row of rows){check();need(address(row.address)&&!addresses.has(row.address),'unique account address');addresses.add(row.address);const n=row.native,a=row.anvil;need(n&&a&&hash(n.storageRoot)&&hash(a.storageHash),'actual optional storage fields');
  const rebuilt=oracle.buildStorage(n.storage),accountProof=oracle.verifyAccount(stateRoot,row.address,a);need(ethers.keccak256(a.code)===a.codeHash&&ethers.keccak256(n.code)===n.codeHash,'code/hash binding');
  const keys=Object.keys(n.storage),proofs=new Map();need(Array.isArray(a.storageProof)&&a.storageProof.length===keys.length&&Array.isArray(row.slots)&&row.slots.length===keys.length,'one proof/direct read per known key');
  for(const p of a.storageProof){const key=q(word(p.key));need(!proofs.has(key),'duplicate requested proof key');proofs.set(key,p);}
  const direct=new Map();for(const s of row.slots){const key=q(quantity(s.key));need(!direct.has(key)&&bytes(s.anvil)&&s.anvil.length===66,'unique direct storage read');direct.set(key,s);}
  const verified=[];for(const key of keys){check();const p=proofs.get(key),d=direct.get(key);need(p&&d,'proof/direct read bound to actual requested key');const path=oracle.verifyProof(a.storageHash,oracle.secureStorageKey(key),p.proof),actual=oracle.storageValue(path),native=quantity(n.storage[key]);need(actual===word(p.value)&&actual===BigInt(d.anvil)&&native===quantity(d.native),'proof leaf/RPC/direct read identity');
   verified.push({key,native:q(native),reference:q(actual),valueMatches:native===actual,member:path.exists,proofNodes:path.hashedNodesUsed});++slots;}
  const equalsNative=rebuilt.root===n.storageRoot,equalsReference=rebuilt.root===a.storageHash;
  accounts.push({address:row.address,knownEntries:keys.length,nonzeroEntries:rebuilt.entries,zeroEntriesDiscarded:rebuilt.discardedZeroEntries,nativeReportedRoot:n.storageRoot,referenceAuthenticatedRoot:a.storageHash,independentRoot:rebuilt.root,
   independentEqualsNative:equalsNative,independentEqualsReference:equalsReference,accountProofVerified:true,storageProofsVerified:true,allKnownValuesMatch:verified.every(s=>s.valueMatches),slots:verified,rootNodeRlp:rebuilt.rootNodeRlp,constructedNodes:rebuilt.nodes,
   interpretation:equalsReference&&!equalsNative?'Native reported storage root differs from independently reconstructed reference-committed map':equalsNative&&!equalsReference?'Known map matches Native root; reference root differs: extra or different unenumerated entries remain possible':equalsNative&&equalsReference?'Known map root matches both commitments':'Independent known-map root differs from both reported commitments'});
 }
 check();return {format:'nano-independent-storage-mpt-diagnostic-v1',passed:false,analysisCompleted:true,referenceStateRoot:stateRoot,accountsVerified:accounts.length,knownSlotsVerified:slots,accounts,referenceStorageEnumerationPerformed:false,rootMismatchSuppressed:false,coreModified:false,productionQualified:false};
}

if(import.meta.url.startsWith('file:')&&process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const started=performance.now(),check=()=>need(performance.now()-started<60000,'finite offline analysis budget'),nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));let input,output,failed=null,report;
 try{need(process.argv.length===7,'CLI pairedJSON inputSHA nodeModules AnvilStateRoot newOutput');const [file,pin,modules,stateRoot,out]=process.argv.slice(2);need([file,modules,out].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&/^[0-9a-f]{64}$/.test(pin)&&hash(stateRoot),'absolute external pinned inputs');
  input=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);const before=await input.stat({bigint:true});need(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=16777216n,'bounded regular pinned JSON');const raw=await input.readFile();check();need(BigInt(raw.length)===before.size&&createHash('sha256').update(raw).digest('hex')===pin,'external JSON SHA/length');
  const ethers=await import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href);check();need(ethers.version==='6.16.0','existing ethers6.16.0');report=analyzePairedStorage(JSON.parse(raw),stateRoot,ethers,check);report.input={sha256:pin,bytes:raw.length};
  need(JSON.stringify(nine(before))===JSON.stringify(nine(await input.stat({bigint:true})))&&JSON.stringify(nine(before))===JSON.stringify(nine(await fs.lstat(file,{bigint:true}))),'input FD/path full-nine fence');await input.close();input=null;check();
  const encoded=Buffer.from(JSON.stringify(report,null,2)+'\n');need(encoded.length<=16777216,'output bound');output=await fs.open(out,F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);await output.writeFile(encoded);await output.sync();await output.close();output=null;check();process.stdout.write(JSON.stringify({format:report.format,passed:false,analysisCompleted:true,accountsVerified:report.accountsVerified,knownSlotsVerified:report.knownSlotsVerified,actualTerminalExitRequired:true,productionQualified:false})+'\n');
 }catch(e){failed=e;process.stderr.write(String(e?.message??e)+'\n');}
 finally{if(output)try{await output.close();}catch(e){failed??=e;}if(input)try{await input.close();}catch(e){failed??=e;}if(performance.now()-started>=60000)failed??=Error('offline final deadline');process.exitCode=failed?1:0;}
}
