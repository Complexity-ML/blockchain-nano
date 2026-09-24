import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
const reports=[];
for(const path of process.argv.slice(2)){
 const report={path,verified:false};
 try{
  const rows=(await readFile(path,'utf8')).split('\n').filter(line=>line.startsWith('{')).map(line=>JSON.parse(line));
  const stages=rows.filter(row=>row.stage==='rpc-overload-clients-finished'),finals=rows.filter(row=>row.scenario==='bft-rpc-overload');
  assert.equal(stages.length,1);assert.equal(finals.length,1,'Missing completed recovery/finality/reopen result');
  const stage=stages[0],final=finals[0];report.fork=stage.fork;const defaults=final.defaultEstimates===true;assert.equal(stage.defaultEstimates===true,defaults);
  assert.equal(final.fork,stage.fork);assert.equal(final.workers,48);assert.equal(final.roundsPerWorker,12);
  assert.equal(stage.controlError,'');assert.equal(stage.errors.length,48);assert(stage.errors.every(error=>error===''));
  assert.equal(stage.requests.length,48);assert.equal(stage.samples.length,48);
  assert(stage.activeWorkersWhenCertified>0&&stage.activeWorkersWhenCertified<=48);
  assert.equal(final.activeWorkersWhenCertified,stage.activeWorkersWhenCertified);
  let capacity=0,expiry=0,successfulEstimates=0,deferred=0,requests=0,identities=0,maximumMs=0,defaultAttempts=0,defaultSuccesses=0;
  for(let worker=0;worker<48;worker++){
   const traces=stage.requests[worker],samples=stage.samples[worker];assert.equal(samples.length,12);let next=0;
   function take(method,tag,permitOverload){
    const row=traces[next];assert(row,`Missing request ${worker}/${next}`);
    assert.equal(row.request,next++);assert.equal(row.method,method);assert.equal(row.tag,tag);
    assert(Number.isFinite(row.durationMs)&&row.durationMs>=0);assert(Number.isFinite(row.startedMs)&&row.startedMs>=0);
    maximumMs=Math.max(maximumMs,row.durationMs);requests++;
    if(row.success!==true){
     assert.equal(row.success,false);assert(permitOverload,'Control request failed');assert.equal(row.rpcCode,-32005);
     if(row.error==='RPC simulation capacity exceeded')capacity++;
     else {assert.equal(row.error,'RPC simulation queue wait exceeded');expiry++;}
    }
    return row.success;
   }
   for(let round=0;round<12;round++){
    const mode=(worker+round)%4,method=mode%2?'eth_estimateGas':'eth_call',omitted=defaults&&mode===1,tag=omitted?'pending':mode<2?'latest':'pending';
    const primary=take(method,omitted?null:tag,true);if(omitted){defaultAttempts++;if(primary)defaultSuccesses++;}let overloaded=!primary;
    if(primary&&mode%2){successfulEstimates++;const verified=take('eth_call',tag,true);if(!verified){overloaded=true;deferred++;}}
    take('eth_chainId',null,false);identities++;
    const sample=samples[round];assert.equal(sample.round,round);assert.equal(sample.node,Math.floor(worker/12));
    assert.equal(sample.omittedBlock===true,omitted);assert.equal(sample.method,method);assert.equal(sample.tag,tag);assert.equal(sample.overloaded,overloaded);
    if(!primary)assert.equal(sample.response,null);
    else if(mode%2)assert(BigInt(sample.response)>23000000n&&BigInt(sample.response)<=30000000n);
    else assert.equal(sample.response,'0x'+'0'.repeat(62)+'2a');
   }
   assert.equal(next,traces.length,'Extra requests');
  }
  assert(capacity>0,'No admission overload exercised');assert(successfulEstimates>0);
  assert.equal(final.overloadRejections,capacity);assert.equal(final.queueExpirations,expiry);assert.equal(final.deferredQuotesVerified,deferred);
  assert.equal(final.attemptedEstimateRounds,288);assert.equal(final.maximumExpensiveCallAttempts,576);assert.equal(final.identityReads,576);
  assert.equal(identities,576);const recoveryTags=defaults?['latest','pending','default']:['latest','pending'];assert.equal(final.recoveryProbes.length,4*recoveryTags.length);if(defaults){assert.equal(defaultAttempts,144);assert(defaultSuccesses>0,'No omitted-block estimate succeeded');}
  assert.deepEqual(final.recoveryProbes.map(row=>`${row.node}/${row.tag}`).sort(),[0,1,2,3].flatMap(node=>recoveryTags.map(tag=>`${node}/${tag}`)).sort());
  for(const row of final.recoveryProbes)assert(BigInt(row.estimate)>23000000n&&BigInt(row.estimate)<=30000000n);
  assert(stage.progress.length>=5);assert(BigInt(stage.progress.at(-1).height)>=BigInt(stage.progress[0].height)+5n);
  assert.equal(final.reopenedPeers,4);assert.equal(final.certificates.length,2);assert.equal(final.loadedValidators.length,4);
  const validators=new Set(final.loadedValidators.map(address=>address.toLowerCase()));assert.equal(validators.size,4);
  for(const certificate of final.certificates){const signers=new Set(certificate.signers.map(address=>address.toLowerCase()));assert(signers.size>=3);for(const signer of signers)assert(validators.has(signer));}
  Object.assign(report,{requests,identityReads:identities,attemptedEstimates:288,successfulEstimates,capacityRejections:capacity,queueExpirations:expiry,deferredQuotesVerified:deferred,recoveryProbes:final.recoveryProbes.length,defaultAttempts,defaultSuccesses,reopenedPeers:4,maximumRequestMs:maximumMs,activeWorkersWhenCertified:stage.activeWorkersWhenCertified,heightBefore:stage.progress[0].height,heightAfter:stage.progress.at(-1).height,verified:true});
 }catch(error){report.issue=error.message;}
 reports.push(report);
}
const verified=reports.length>0&&reports.every(row=>row.verified);console.log(JSON.stringify({reports,verified},null,2));if(!verified)process.exitCode=1;
