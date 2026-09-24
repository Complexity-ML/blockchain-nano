import {readFile} from 'node:fs/promises';

// Summarize individual RPC durations, never multi-request round durations.
// Missing/failed requests remain failures even if consensus kept progressing.
const campaigns=[];
for(const path of process.argv.slice(2)){
  const rows=(await readFile(path,'utf8')).split('\n').filter(line=>line.startsWith('{')).map(line=>JSON.parse(line));
  const stages=rows.filter(row=>row.stage==='rpc-cost-clients-finished');
  const finals=rows.filter(row=>row.scenario==='bft-rpc-cost');
  const issues=[];
  if(stages.length!==1)issues.push('Expected one client trace');
  if(finals.length!==1)issues.push('Expected one completed finality/reopen summary');
  const stage=stages[0], final=finals[0];
  const requests=(stage?.requests??[]).flatMap((requests,worker)=>requests.map(request=>({worker,...request})));
  if(stage?.controlError)issues.push(stage.controlError);
  for(const [worker,error] of (stage?.errors??[]).entries())if(error)issues.push(`worker ${worker}: ${error}`);
  if(final){
    if(final.workers!==16||final.roundsPerWorker!==24)issues.push('Unexpected workload');
    if(stage?.requests?.length!==16||stage?.samples?.length!==16||stage?.errors?.length!==16)issues.push('Missing workers');
    for(let worker=0;worker<16;worker++){
      const actual=stage?.requests?.[worker]??[];
      const expected=[];
      for(let round=0;round<24;round++){
        const mode=(worker+round)%4, tag=mode<2?'latest':'pending';
        expected.push([mode%2?'eth_estimateGas':'eth_call',tag]);
        if(mode%2)expected.push(['eth_call',tag]);
        expected.push(['eth_chainId',null]);
      }
      if(actual.length!==expected.length)issues.push(`worker ${worker}: missing/extra requests`);
      for(const [index,[method,tag]] of expected.entries()){
        const r=actual[index];
        if(!r||r.request!==index||r.method!==method||r.tag!==tag||r.success!==true||!Number.isFinite(r.durationMs)||r.durationMs<0)
          issues.push(`worker ${worker}: invalid request ${index}`);
      }
      const samples=stage?.samples?.[worker]??[];
      if(samples.length!==24||samples.some((sample,index)=>sample.round!==index))issues.push(`worker ${worker}: invalid rounds`);
    }
    if(final.reopenedPeers!==4||final.activeWorkersWhenCertified!==16)issues.push('Finality/reopen coverage differs');
  }
  const methods={};
  for(const method of ['eth_estimateGas','eth_call','eth_chainId']){
    const selected=requests.filter(request=>request.method===method);
    methods[method]={count:selected.length,failed:selected.filter(request=>!request.success).length,maxMs:selected.length?Math.max(...selected.map(request=>request.durationMs)):null};
  }
  campaigns.push({path,fork:stage?.fork,transport:final?.transport,requests:requests.length,methods,failedRequests:requests.filter(request=>!request.success),issues,verified:issues.length===0});
}
console.log(JSON.stringify({campaigns,verified:campaigns.length>0&&campaigns.every(c=>c.verified)},null,2));
if(!campaigns.length||campaigns.some(c=>!c.verified))process.exitCode=1;
