import {createServer} from 'node:http';import {execFile} from 'node:child_process';import {promisify} from 'node:util';import {readFile,writeFile} from 'node:fs/promises';import assert from 'node:assert/strict';
const exec=promisify(execFile),root='/private/tmp/nano-slashing-helper-build-v3-20261005',binary='/tmp/nano-monotonic-runtime-native-build-20261005/nano-capella-slashing-fixture',proof=JSON.parse(await readFile(root+'/proof.json','utf8')),reports=[];
for(const mode of ['normal','local-failure','wrong-genesis','valid-rejected']){
 const submitted=[],forgotten=[];let statusReads=0;
 const server=createServer(async(req,res)=>{try{
  let text='';for await(const part of req)text+=part;const r=JSON.parse(text);let result;
  assert.equal(r.params[0],'synthetic-private-ipc-token');
  if(r.method==='nano_capellaStatus'){statusReads++;result={chainId:proof.chainId,genesisHash:mode==='wrong-genesis'?'0x'+'0'.repeat(64):proof.genesisHash,forkDigest:'0x12345678',consensusIdentity:'0x'+'a'.repeat(64),admissionSession:'fixture-session'};}
  else{assert.equal(r.params[1],'fixture-session');const index=Number(BigInt(r.params[2]));
   if(r.method==='nano_capellaSubmit'){
    const envelope=JSON.parse(r.params[3]);assert.equal(envelope.kind,'attester_slashing');const expected=index<=4?proof.invalid[index-1].ssz:index===5?proof.ssz:proof.reverseSsz;assert.equal(envelope.ssz,expected);submitted.push(index);result={state:'queued'};
   }else if(r.method==='nano_capellaJob')result={state:'completed',decision:mode==='local-failure'&&index===1?'local_failure':index<=4||mode==='valid-rejected'?'rejected':'accepted'};
   else if(r.method==='nano_capellaForget'){forgotten.push(index);result={state:'completed'};}else throw Error('unexpected RPC');
  }
  res.setHeader('Content-Type','application/json');res.end(JSON.stringify({jsonrpc:'2.0',id:r.id,result}));
 }catch(error){res.statusCode=500;res.end(error.message);}});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));let result;
 try{result=await exec(binary,[String(server.address().port),'unused-listener','unused-bootnodes'],{env:{PATH:process.env.PATH,NANO_TEST_SLASHING_FILE:root+'/proof.json',NANO_TEST_SLASHING_DAEMON:'/usr/bin/true',NANO_NETWORK_TOKEN:'synthetic-private-ipc-token'},timeout:20000});}
 catch(e){result={code:e.code,stdout:e.stdout,stderr:e.stderr};}
 finally{await new Promise(resolve=>server.close(resolve));}
 if(mode==='normal'){assert.equal(result.code,undefined);assert.deepEqual(submitted,[1,2,3,4,5,6]);assert.deepEqual(forgotten,submitted);assert.match(result.stdout,/private-slashing-bootstrap-passed/);}
 else{assert.equal(result.code,1);assert.ok(!result.stdout.includes('private-slashing-bootstrap-passed'));if(mode==='wrong-genesis')assert.equal(submitted.length,0);if(mode==='local-failure')assert.equal(submitted.length,1);if(mode==='valid-rejected')assert.equal(submitted.length,5);}
 reports.push({mode,passed:true,statusReads,submitted,forgotten,helperExit:result.code??0,stderr:result.stderr});
}
const before=await readFile(root+'/proof.json','utf8');let refused=false;try{await exec(binary,['--generate','/private/tmp/nano-exit-wan-current-20261005/fixture/genesis.json','/private/tmp/nano-exit-wan-current-20261005/mac.json',root+'/proof.json']);}catch(e){refused=e.code===1&&e.stderr.includes('fresh output required');}assert.ok(refused);assert.equal(await readFile(root+'/proof.json','utf8'),before);
await writeFile(root+'/bootstrap-check.json',JSON.stringify({reports,freshOutputRefusal:true,existingProofUnchanged:true,syntheticIpcServerOnly:true,realNetworkQualified:false,productionQualified:false},null,2)+'\n');console.log('Four bootstrap transport/decision cases and output preservation passed');
