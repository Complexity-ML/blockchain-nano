import {spawn} from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
// Independent causal check against the unmodified binary: the new C++ test
// deliberately references the new trace API and cannot link an old library.
const binary=process.env.NANO_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&out&&path.isAbsolute(binary)&&path.isAbsolute(out));fs.mkdirSync(out,{mode:0o700});
const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-access-list-missing-'));
const address='0x00000000000000000000000000000000000000a1';
fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify({config:{chainId:1337,evmFork:'Shanghai'},alloc:{[address]:{code:'0x6000545000'}}}));
const child=spawn(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1'],{stdio:['ignore','pipe','pipe']});
let transcript='',failure=null,response=null,closed=false;const log=fs.createWriteStream(path.join(out,'baseline.log'),{flags:'wx',mode:0o600});
child.once('close',()=>{closed=true;log.end();});for(const pipe of [child.stdout,child.stderr])pipe.on('data',data=>{transcript+=data.toString();if(transcript.length>1048576){child.kill('SIGKILL');return;}log.write(data);});
try{
 const url=await new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('baseline startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('baseline exited during startup'));});child.stdout.on('data',()=>{const m=transcript.match(/listening on 127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});
 const raw=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method:'eth_createAccessList',params:[{to:address},'latest']}),signal:AbortSignal.timeout(5000)});assert(raw.ok);response=await raw.json();assert.equal(response?.error?.code,-32601);assert(!('result' in response));
}catch(e){failure=e.message;}finally{
 if(child.pid&&!closed&&child.exitCode===null&&child.signalCode===null){const done=new Promise(resolve=>child.once('close',resolve));child.kill('SIGTERM');const kill=setTimeout(()=>child.kill('SIGKILL'),3000);let deadline;try{await Promise.race([done,new Promise((_,reject)=>{deadline=setTimeout(()=>reject(Error('baseline cleanup timeout')),5000);})]);}catch(e){failure??=e.message;}finally{clearTimeout(kill);clearTimeout(deadline);}}
 if(closed||!child.pid)fs.rmSync(temp,{recursive:true,force:true});
 fs.writeFileSync(path.join(out,'result.json'),JSON.stringify({format:'nano-public-access-list-method-missing-causal-v1',status:failure?'REFUSED':'EXPECTED_BASELINE_METHOD_MISSING',binarySha256:crypto.createHash('sha256').update(fs.readFileSync(binary)).digest('hex'),actualRequestCount:response?1:0,response,failure,childClosed:closed||!child.pid,privateInputsRead:false,newSignatures:0,productionQualified:false},null,2)+'\n');
}
assert.equal(failure,null,failure??'');console.log('Baseline eth_createAccessList returns method-not-found (-32601)');
