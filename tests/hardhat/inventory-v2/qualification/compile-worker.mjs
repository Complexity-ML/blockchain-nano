// Child compiler only; parent owns process-group timeout and immutable inputs.
// No import callback, download, network client, wallet or shell command.
import fs from 'node:fs';
import {createHash} from 'node:crypto';
import {createRequire} from 'node:module';
const need=(ok,why)=>{if(!ok)throw Error('Inventory compiler: '+why);};
const sha=b=>createHash('sha256').update(b).digest('hex');
function load(path,digest,limit){
 const fd=fs.openSync(path,fs.constants.O_RDONLY|fs.constants.O_NOFOLLOW);
 try{const a=fs.fstatSync(fd,{bigint:true});need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=BigInt(limit),'regular bounded input');
  const b=fs.readFileSync(fd);const z=fs.fstatSync(fd,{bigint:true});
  for(const k of ['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'])need(a[k]===z[k],'input changed');
  need(sha(b)===digest,'external SHA');return b;
 }finally{fs.closeSync(fd);}
}
try{
 need(process.argv.length===4&&/^[0-9a-f]{64}$/.test(process.argv[3]),'exact CLI');
 const bytes=load(process.argv[2],process.argv[3],8*1024*1024),job=JSON.parse(bytes);
 need(job.format==='nano-inventory-v2-compiler-job-v1','job format');
 const entry=load(job.solcEntry.path,job.solcEntry.sha256,32*1024*1024);
 need(entry.length===job.solcEntry.bytes,'entry byte length');
 const solc=createRequire(import.meta.url)(job.solcEntry.path);
 const version=solc.version();need(/^0\.8\.28\+commit\./.test(version),'exact solc release');
 // The parent constructs the fixed Standard JSON settings and full source map.
 need(job.input.language==='Solidity'&&job.input.settings.evmVersion==='shanghai'&&job.input.settings.viaIR===false,
  'fixed compiler profile');
 const raw=solc.compile(JSON.stringify(job.input));need(typeof raw==='string'&&Buffer.byteLength(raw)<=8*1024*1024,'bounded compiler result');
 const compiled=JSON.parse(raw),errors=Array.isArray(compiled.errors)?compiled.errors:[];
 const failed=errors.some(e=>e.severity==='error');
 process.stdout.write(JSON.stringify({format:'nano-inventory-v2-compiler-output-v1',compilerVersion:version,
  compilationSucceeded:!failed,inputSha256:sha(Buffer.from(JSON.stringify(job.input))),compiled})+'\n');
 process.exitCode=failed?1:0;
}catch(error){process.stderr.write('Inventory compiler failed: '+(error?.code??'SOURCE_OR_COMPILER_ERROR')+'\n');process.exitCode=1;}
