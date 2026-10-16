import test from 'node:test';
import assert from 'node:assert/strict';
import {configuration,evaluate,sample,run} from './finality-monitor.mjs';
const a='0x'+'a'.repeat(64),b='0x'+'b'.repeat(64),c={chainId:'0x539',genesisHash:a,configName:'nano-capella-reviewed',staleMs:100,timeoutMs:1000,maximumResponseBytes:4096,maximumEpochs:256,minimumPeers:1,nodes:[{name:'a',url:'http://127.0.0.1:8545/'},{name:'b',url:'http://127.0.0.1:9545/'}]};
const rows=(epoch='2',root=a)=>c.nodes.map(n=>({name:n.name,epoch,root,peers:1}));
test('common finality is observed, zero peers are unhealthy',()=>{assert.equal(evaluate(c,rows(),0).report.healthy,true);const r=rows();r[0].peers=0;assert.equal(evaluate(c,r,0).report.healthy,false);});
test('stall deadline persists across serialization and recovers on progress',()=>{const state=evaluate(c,rows(),0).state;assert.equal(evaluate(c,rows(),99,state).report.healthy,true);const stalled=evaluate(c,rows(),100,JSON.parse(JSON.stringify(state)));assert.equal(stalled.report.alerts.length,2);assert.equal(evaluate(c,rows('3',b),101,stalled.state).report.healthy,true);});
test('conflicting finality is latched across later healthy observations',()=>{const r=rows();r[1].root=b;const first=evaluate(c,r,0);assert.equal(first.report.healthy,false);assert.equal(evaluate(c,rows('3',b),1,first.state).report.healthy,false);});
test('regression cannot be erased by advancing the other observer',()=>{const state=evaluate(c,rows('3'),0).state;const r=rows('4',b);r[0]=rows('2')[0];assert.equal(evaluate(c,r,1,state).report.safetyFaults[0].code,'finality-regression');});
test('lagging observer keeps safety root history, retention trims only below minimum',()=>{const state=evaluate(c,rows('2'),0).state;const r=rows('3',b);r[0]=rows('2')[0];const next=evaluate(c,r,1,state);assert.deepEqual(Object.keys(next.state.roots),['2','3']);const final=evaluate(c,rows('3',b),2,next.state);assert.deepEqual(Object.keys(final.state.roots),['3']);});
test('unavailable observer and corrupt observation never earn healthy status',()=>{let r=rows();r[0]={name:'a',error:'down'};assert.equal(evaluate(c,r,0).report.healthy,false);r=rows();r[0].epoch='-1';assert.equal(evaluate(c,r,0).report.healthy,false);});
test('state cannot be reused with another profile or time reversal',()=>{const state=evaluate(c,rows(),10).state;assert.throws(()=>evaluate({...c,configName:'other'},rows(),11,state));assert.throws(()=>evaluate(c,rows(),9,state));});
test('reject public RPC, duplicate endpoint and duplicate samples',()=>{assert.throws(()=>configuration({...c,nodes:[{name:'a',url:'http://example.com/'}]}));assert.throws(()=>configuration({...c,nodes:[c.nodes[0],{name:'b',url:c.nodes[0].url}]}));assert.throws(()=>evaluate(c,[rows()[0],rows()[0]],0));});
const request=(change=()=>{})=>async(url,options)=>{
 let data;
 if(options.method==='POST'){const method=JSON.parse(options.body).method;const result=method==='eth_chainId'?'0x539':method==='net_peerCount'?'0x1':{hash:a};data={jsonrpc:'2.0',id:1,result};}
 else if(url.endsWith('/spec'))data={data:{CONFIG_NAME:c.configName}};
 else if(url.endsWith('/head'))data={data:{root:a,header:{message:{state_root:b}}}};
 else{assert.ok(url.includes('/states/'+b+'/'));data={data:{finalized:{epoch:'2',root:a}}};}
 change(data,url);return new Response(JSON.stringify(data),{status:200});
};
test('reader pins finality to immutable head state and verifies chain/profile',async()=>{const r=await sample(c,configuration(c).nodes[0],request());assert.equal(r.epoch,'2');assert.equal(r.error,undefined);});
test('RPC errors and profile mismatch are unavailable',async()=>{assert.ok((await sample(c,c.nodes[0],request((d,url)=>{if(url.endsWith('/spec'))d.data.CONFIG_NAME='wrong';}))).error);assert.ok((await sample(c,c.nodes[0],request(d=>{if(d.jsonrpc)d.error={code:-1};}))).error);});
test('response body resource budget enforced',async()=>{const r=await sample({...c,maximumResponseBytes:10},c.nodes[0],request());assert.match(r.error,/budget/);});

test('missing persisted safety root is rejected, not silently re-established',()=>{const state=evaluate(c,rows(),0).state;delete state.roots['2'];assert.throws(()=>evaluate(c,rows(),1,state));});
test('invalid samples cannot report historical common finality as current',()=>{const state=evaluate(c,rows(),0).state;const r=rows();r[0].epoch='invalid';assert.equal(evaluate(c,r,1,state).report.commonFinality,false);});
import {mkdtemp,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
test('overlapping CLI invocations fail before reading or modifying monitor state',async()=>{const dir=await mkdtemp(join(tmpdir(),'nano-monitor-lock-'));try{const state=join(dir,'state.json');await writeFile(state+'.lock','');await assert.rejects(run(join(dir,'missing-config.json'),state),e=>e.code==='EEXIST');}finally{await rm(dir,{recursive:true,force:true});}});

test('lagging observer cannot make root history exceed operator resource policy',()=>{const bounded={...c,maximumEpochs:1};const state=evaluate(bounded,rows(),0).state;const next=evaluate(bounded,rows('3',b),1,state);assert.equal(next.report.healthy,false);assert.equal(Object.keys(next.state.roots).length,1);assert.equal(next.report.safetyFaults[0].code,'history-capacity');});
import {createServer} from 'node:http';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile);
test('CLI samples HTTP, persists safety fault and releases lock across five processes',async()=>{
 const dir=await mkdtemp(join(tmpdir(),'nano-monitor-cli-'));const servers=[];let epoch='2',conflict=false;
 try{
  for(let i=0;i<2;++i){
   const server=createServer(async(req,res)=>{
    let value;
    if(req.method==='POST'){let body='';for await(const part of req)body+=part;const method=JSON.parse(body).method;value={jsonrpc:'2.0',id:1,result:method==='eth_chainId'?'0x539':method==='net_peerCount'?'0x1':{hash:a}};}
    else if(req.url.endsWith('/spec'))value={data:{CONFIG_NAME:c.configName}};
    else if(req.url.endsWith('/head'))value={data:{root:a,header:{message:{state_root:b}}}};
    else value={data:{finalized:{epoch,root:conflict&&i===1?b:a}}};
    res.setHeader('Content-Type','application/json');res.end(JSON.stringify(value));
   });await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));servers.push(server);
  }
  const config=join(dir,'config.json'),state=join(dir,'state.json');await writeFile(config,JSON.stringify({...c,staleMs:1,nodes:servers.map((s,i)=>({name:i?'b':'a',url:'http://127.0.0.1:'+s.address().port+'/'}))}));
  const invoke=async()=>{try{const r=await exec(process.execPath,['scripts/operations/finality-monitor.mjs',config,state],{timeout:5000});return {code:0,report:JSON.parse(r.stdout)};}catch(e){return {code:e.code,report:JSON.parse(e.stdout)};}};
  assert.equal((await invoke()).code,0);
  const stalled=await invoke();assert.equal(stalled.code,2);assert.ok(stalled.report.alerts.some(a=>a.code==='finality-stalled'));
  epoch='3';assert.equal((await invoke()).code,0);
  conflict=true;const bad=await invoke();assert.equal(bad.code,2);assert.ok(bad.report.safetyFaults.some(a=>a.code==='conflicting-finality'));
  epoch='4';conflict=false;assert.equal((await invoke()).code,2);
 }finally{for(const s of servers)await new Promise(resolve=>s.close(resolve));await rm(dir,{recursive:true,force:true});}
});

test('oversized remote epoch and unconfigured endpoint are rejected before trust',async()=>{const r=rows();r[0].epoch='9'.repeat(1000);assert.equal(evaluate(c,r,0).report.healthy,false);await assert.rejects(sample(c,{name:'a',url:'http://example.com/'},()=>{throw Error('Must not fetch');}),/Unconfigured/);});
import {transportRequest} from './monitor-transport.mjs';
test('same loopback port is distinct only for distinct containers/hosts',()=>{const nodes=[{name:'a',kind:'docker',container:'nano-a',url:c.nodes[0].url},{name:'b',kind:'ssh-docker',container:'nano-a',ssh:'ubuntu@145.239.199.166',url:c.nodes[0].url}];assert.equal(configuration({...c,nodes}).nodes.length,2);assert.throws(()=>configuration({...c,nodes:[nodes[0],{...nodes[0],name:'b'}]}));});
test('container and SSH identities cannot inject commands',()=>{for(const container of ['x;evil','$(evil)','-option'])assert.throws(()=>configuration({...c,nodes:[{...c.nodes[0],kind:'docker',container}]}));assert.throws(()=>configuration({...c,nodes:[{...c.nodes[0],kind:'ssh-docker',container:'nano-a',ssh:'x;evil'}]}));});
test('Docker reader uses argv and validates HTTP status/body budget',async()=>{const n={name:'a',kind:'docker',container:'nano-a',url:'http://127.0.0.1:8545'};let calls=0;const adapter=transportRequest(n,c,async(bin,args,limits)=>{++calls;assert.equal(bin,'docker');assert.deepEqual(args.slice(0,3),['exec','nano-a','curl']);assert.equal(limits.maxBuffer,c.maximumResponseBytes+64);return {stdout:'{"ok":true}\n200'};});assert.deepEqual(await (await adapter(n.url+'/path')).json(),{ok:true});assert.equal(calls,1);await assert.rejects(adapter('http://example.com/'));});
test('SSH reader quotes each remote argument including POST data',async()=>{const n={name:'a',kind:'ssh-docker',container:'nano-a',ssh:'ubuntu@145.239.199.166',url:'http://127.0.0.1:8545'};const adapter=transportRequest(n,c,async(bin,args)=>{assert.equal(bin,'ssh');assert.ok(args.includes(n.ssh));assert.ok(args.at(-1).includes("'sudo' '-n' 'docker' 'exec' 'nano-a'"));assert.ok(args.at(-1).includes("'\\''"));return {stdout:'{}\n200'};});await adapter(n.url+'/',{method:'POST',body:JSON.stringify({text:"can't"})});});


// Injected adapters only: no HTTP server, Docker, SSH or alert delivery.
test('HTTP monitor rejects cross-origin redirects at every read phase',async t=>{
 for(let phase=1;phase<=6;++phase)await t.test('read '+phase,async()=>{
  let reads=0,externalReads=0;const direct=request();
  const observed=await sample(c,c.nodes[0],async(url,options)=>{
   if(++reads===phase){
    // Model fetch's redirect policy; never perform the redirected request.
    if(options.redirect==='error')throw new TypeError('Synthetic redirect refused');
    ++externalReads;
   }
   return direct(url,options);
  });
  assert.ok(observed.error,'a redirect must not yield an accepted monitor sample');
  assert.equal(externalReads,0,'the request policy must prevent redirect following');
  assert.equal(reads,phase,'sampling must stop at the refused read');
 });
 await t.test('valid pinned observations stay accepted',async()=>{
  let reads=0;const direct=request();
  const observed=await sample(c,c.nodes[0],async(url,options)=>{++reads;return direct(url,options);});
  assert.deepEqual(observed,{name:'a',epoch:'2',root:a,headRoot:a,stateRoot:b,peers:1});
  assert.equal(reads,6);
 });
});

test('monitor rejects an already-redirected response even from an injected adapter',async t=>{
 for(let phase=1;phase<=6;++phase)await t.test('read '+phase,async()=>{
  let reads=0;const direct=request();
  const observed=await sample(c,c.nodes[0],async(url,options)=>{
   const response=await direct(url,options);
   if(++reads===phase)Object.defineProperty(response,'redirected',{value:true});
   return response;
  });
  assert.match(observed.error??'',/Redirected monitor response rejected/);
  assert.equal(reads,phase);
 });
});

test('monitor rejects result and falsy error in the same RPC envelope',async t=>{
 for(const error of [null,false,0,''])await t.test('error '+JSON.stringify(error),async()=>{
  let reads=0;const direct=request(data=>{if(data.jsonrpc)data.error=error;});
  const observed=await sample(c,c.nodes[0],async(url,options)=>{++reads;return direct(url,options);});
  assert.match(observed.error??'',/RPC response rejected/);
  assert.equal(reads,1,'malformed chain reply must stop before later reads');
 });
});
