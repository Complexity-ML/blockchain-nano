import {test} from 'node:test';
import assert from 'node:assert/strict';
import {decodeHttpResponse,httpStatusTrailer} from './http-response.mjs';
import {createServer} from 'node:http';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const response=(body,status)=>JSON.stringify(body)+'\nNANO_HTTP_STATUS:'+status;
test('Beacon 503 diagnostics remain failures and distinguish capacity, queue and state',()=>{
 for(const message of ['Beacon read capacity exceeded','Beacon read queue expired','Beacon state unavailable']){
  assert.throws(()=>decodeHttpResponse(response({code:503,message},503),'vps','/eth/v1/beacon/headers/head'),error=>error.status===503&&error.serverCode===503&&error.serverMessage===message);
 }
});
test('successful Beacon and JSON-RPC error bodies keep their original semantics',()=>{
 for(const body of [{data:{finalized:{epoch:'7'}}},{error:{code:-32001,message:'Unauthorized Capella request'}}])assert.deepEqual(decodeHttpResponse(response(body,200),'local','/'),body);
});
test('malformed transport output fails closed and non-JSON HTTP errors expose no raw body',()=>{
 for(const text of ['{}','{}\nNANO_HTTP_STATUS:000','broken\nNANO_HTTP_STATUS:200'])assert.throws(()=>decodeHttpResponse(text,'local','/'));
 assert.throws(()=>decodeHttpResponse('private diagnostic must not be echoed\nNANO_HTTP_STATUS:503','vps','/'),error=>error.status===503&&!error.message.includes('private diagnostic'));
});
test('server diagnostics are bounded and control characters removed',()=>{
 assert.throws(()=>decodeHttpResponse(response({code:503,message:'x\n'.repeat(400)},503),'vps','/'),error=>error.serverMessage.length===256&&!/[\r\n]/.test(error.serverMessage));
});
test('real curl preserves a 503 JSON body instead of discarding it',async()=>{
 const server=createServer((request,response)=>{response.writeHead(503,{'Content-Type':'application/json'});response.end(JSON.stringify({code:503,message:'Beacon read queue expired'}));});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 try{
  const {stdout}=await promisify(execFile)('curl',['-sS','--max-time','5','--write-out',httpStatusTrailer,`http://127.0.0.1:${server.address().port}/eth/v1/beacon/headers/head`]);
  assert.throws(()=>decodeHttpResponse(stdout,'vps','/eth/v1/beacon/headers/head'),error=>error.status===503&&error.serverMessage==='Beacon read queue expired');
 }finally{await new Promise(resolve=>server.close(resolve));}
});

test('Beacon cancellation phase and duration survive the fatal campaign error',()=>{
 assert.throws(()=>decodeHttpResponse(response({code:503,message:'Beacon read cancelled or execution budget expired',phase:'publication-pin',elapsedMs:2345},503),'vps','/eth/v1/beacon/states/head/finality_checkpoints'),error=>error.status===503&&error.phase==='publication-pin'&&error.elapsedMs===2345&&error.message.includes('[phase=publication-pin, elapsedMs=2345]'));
});
test('untrusted phase labels and malformed durations are not echoed',()=>{
 for(const [phase,elapsedMs] of [['private-secret',12],['publication-pin',-1],['publication-pin','private-secret'],['publication-pin',Number.MAX_SAFE_INTEGER+1]])assert.throws(()=>decodeHttpResponse(response({code:503,message:'Unavailable',phase,elapsedMs},503),'vps','/'),error=>!error.message.includes('private-secret')&&!error.message.includes('[phase='));
});
