// NONCANONICAL COUNTERFACTUAL ONLY. Never imported by the canonical builder.
// Adds one deliberate RLP(0) leaf at secure slot8 without changing input state.
import fs from 'node:fs/promises';
import {constants as F} from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const need=(ok,m)=>{if(!ok)throw Error('Zero-slot8 hypothesis: '+m);};
const sha=b=>createHash('sha256').update(b).digest('hex'),hash=s=>typeof s==='string'&&/^0x[0-9a-f]{64}$/.test(s);
const quantity=s=>{need(typeof s==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(s)&&s.length<=66,'canonical uint256');return BigInt(s);};
const word=n=>'0x'+n.toString(16).padStart(64,'0');
const minimal=n=>{if(n===0n)return '0x';const h=n.toString(16);return '0x'+(h.length%2?'0':'')+h;};
const began=performance.now(),check=()=>need(performance.now()-began<60000,'offline deadline');
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s));
async function readKnown(file,pin){
 const h=await fs.open(file,F.O_RDONLY|F.O_NOFOLLOW);try{const a=await h.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=16777216n,'bounded regular input');const b=await h.readFile();check();need(BigInt(b.length)===a.size&&sha(b)===pin&&JSON.stringify(nine(a))===JSON.stringify(nine(await h.stat({bigint:true})))&&JSON.stringify(nine(a))===JSON.stringify(nine(await fs.lstat(file,{bigint:true}))),'external SHA/full-nine/path binding');return b;}finally{await h.close();}
}
function nonCanonicalSlot8Tree(storage,ethers){
 need(storage!==null&&typeof storage==='object'&&!Array.isArray(storage)&&Object.keys(storage).length<=1024,'bounded map');need(!Object.hasOwn(storage,'0x8')||quantity(storage['0x8'])===0n,'slot8 is absent or zero');
 const nibbles=h=>Array.from(ethers.getBytes(h)).flatMap(b=>[b>>4,b&15]);
 const compact=(ns,leaf)=>{const v=ns.length%2?[leaf?3:1,...ns]:[leaf?2:0,0,...ns];let out='0x';for(let i=0;i<v.length;i+=2)out+=(v[i]*16+v[i+1]).toString(16).padStart(2,'0');return out;};
 const rows=[];for(const [key,value]of Object.entries(storage)){check();const k=quantity(key),v=quantity(value);if(v!==0n)rows.push({path:nibbles(ethers.keccak256(word(k))),payload:ethers.encodeRlp(minimal(v))});}
 // This is the ONLY deliberately noncanonical leaf. Do not use this tree as
 // Ethereum storage or pass its zero payload through canonical storageValue().
 const target=nibbles(ethers.keccak256(word(8n)));rows.push({path:target,payload:ethers.encodeRlp('0x')});
 const encode=(value,extra={})=>{check();const raw=ethers.encodeRlp(value);return {value,raw,hash:ethers.keccak256(raw),...extra};};
 const reference=node=>(node.raw.length-2)/2<32?node.value:node.hash;
 function build(items,depth){check();need(depth<=64&&items.length>0,'bounded trie');if(items.length===1)return encode([compact(items[0].path.slice(depth),true),items[0].payload],{leaf:true});
  let shared=0;while(depth+shared<64&&items.every(r=>r.path[depth+shared]===items[0].path[depth+shared]))++shared;
  if(shared){const child=build(items,depth+shared);return encode([compact(items[0].path.slice(depth,depth+shared),false),reference(child)],{extension:shared,child});}
  need(depth<64,'duplicate secure key');const value=Array(17).fill('0x'),children=new Map();for(let i=0;i<16;i++){const part=items.filter(r=>r.path[depth]===i);if(part.length){const child=build(part,depth+1);children.set(i,child);value[i]=reference(child);}}return encode(value,{children});
 }
 const tree=build(rows,0),proof=[];let cursor=tree,depth=0;for(let steps=0;;steps++){check();need(steps<128,'bounded derived proof');proof.push(cursor.raw);if(cursor.leaf)break;if(cursor.extension){depth+=cursor.extension;cursor=cursor.child;}else{cursor=cursor.children.get(target[depth++]);need(cursor,'slot8 path exists');}}
 return {root:tree.hash,rootNodeRlp:tree.raw,derivedSlot8Proof:proof,deliberateZeroLeafPayload:'0x80',nonCanonicalEntries:rows.length};
}

let failed=null,output;
try{
 need(process.argv.length===8,'CLI pairedJSON inputSHA modules canonicalOracleFile AnvilStateRoot newOutput');const [input,pin,modules,oracleFile,stateRoot,out]=process.argv.slice(2);
 need([input,modules,oracleFile,out].every(p=>path.isAbsolute(p)&&path.resolve(p)===p)&&/^[0-9a-f]{64}$/.test(pin)&&hash(stateRoot),'absolute paths/external pins');
 const raw=await readKnown(input,pin),original=await readKnown(oracleFile,'3f03c819377ab36a496143f83006119b23690d25611d8c53e7ed551b64e710ab');
 const ethers=await import(pathToFileURL(path.join(modules,'ethers/lib.esm/index.js')).href);check();need(ethers.version==='6.16.0','retained ethers6.16.0');
 const {createIndependentMptOracle}=await import('data:text/javascript;base64,'+original.toString('base64')),canonical=createIndependentMptOracle(ethers,check),rows=JSON.parse(raw);need(Array.isArray(rows)&&rows.length===14,'same fourteen public paired accounts');
 const results=[];for(const row of rows){check();const before=sha(Buffer.from(JSON.stringify(row.native.storage))),base=canonical.buildStorage(row.native.storage);if(base.root===row.anvil.storageHash)continue;
  need(base.root===row.native.storageRoot,'independent canonical root must still match Native before hypothesis');canonical.verifyAccount(stateRoot,row.address,row.anvil);
  const hypothetical=nonCanonicalSlot8Tree(row.native.storage,ethers),local=canonical.verifyProof(hypothetical.root,canonical.secureStorageKey('0x8'),hypothetical.derivedSlot8Proof);need(local.exists&&local.value==='0x80','derived noncanonical zero leaf');
  const matched=hypothetical.root===row.anvil.storageHash;let authenticated=false;if(matched){const atReference=canonical.verifyProof(row.anvil.storageHash,canonical.secureStorageKey('0x8'),hypothetical.derivedSlot8Proof);need(atReference.exists&&atReference.value==='0x80','same proof authenticated against reference root');authenticated=true;}
  need(before===sha(Buffer.from(JSON.stringify(row.native.storage))),'Native input map never mutated');results.push({address:row.address,canonicalRoot:base.root,nativeReportedRoot:row.native.storageRoot,referenceAuthenticatedRoot:row.anvil.storageHash,
   nonCanonicalZeroSlot8LeafRoot:hypothetical.root,rootMatchesReference:matched,referenceZeroLeafAuthenticated:authenticated,zeroSlot:'0x8',deliberateLeafPayload:'0x80',derivedProof:hypothetical.derivedSlot8Proof,rootNodeRlp:hypothetical.rootNodeRlp,nativeInputMapUnchanged:true,nonCanonicalHypothesisOnly:true});
 }
 need(results.length===2,'exactly the two retained canonical/reference mismatches');check();const report={format:'nano-noncanonical-zero-slot8-hypothesis-v1',passed:false,analysisCompleted:true,nonCanonicalHypothesisOnly:true,input:{sha256:pin,bytes:raw.length},canonicalOracleSha256:'3f03c819377ab36a496143f83006119b23690d25611d8c53e7ed551b64e710ab',referenceStateRoot:stateRoot,results,canonicalBuilderModified:false,canonicalZeroFilteringPreserved:true,stateOrInputsModified:false,referenceFinalityAuthenticated:false,productionQualified:false};
 const bytes=Buffer.from(JSON.stringify(report,null,2)+'\n');need(bytes.length<=16777216,'output bound');output=await fs.open(out,F.O_WRONLY|F.O_CREAT|F.O_EXCL|F.O_NOFOLLOW,0o600);await output.writeFile(bytes);await output.sync();await output.close();output=null;check();process.stdout.write(JSON.stringify({format:report.format,passed:false,analysisCompleted:true,nonCanonicalHypothesisOnly:true,accountsCompared:results.length,referenceZeroLeafAuthenticated:results.map(r=>r.referenceZeroLeafAuthenticated),actualTerminalExitRequired:true,productionQualified:false})+'\n');
}catch(e){failed=e;process.stderr.write(String(e?.message??e)+'\n');}
finally{if(output)try{await output.close();}catch(e){failed??=e;}if(performance.now()-began>=60000)failed??=Error('offline final deadline');process.exitCode=failed?1:0;}
