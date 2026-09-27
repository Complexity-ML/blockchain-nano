// Isolated real Docker network partition. All keys come from the public fixture exporter.
import {spawn} from 'node:child_process';
import {mkdir,readFile,writeFile,appendFile} from 'node:fs/promises';
import {resolve,join} from 'node:path';
import {randomBytes} from 'node:crypto';
import {setTimeout as wait} from 'node:timers/promises';
const [exporter,outArg,image,revision]=process.argv.slice(2);
if(!exporter||!outArg||!image||!/^([a-f0-9]{40})$/.test(revision??''))throw Error('Usage: docker-partition.mjs exporter NEW-output-directory image revision');
const out=resolve(outArg),fixture=join(out,'fixture'),campaign='nano-capella-partition-'+randomBytes(6).toString('hex'),network=campaign+'-net',names=[campaign+'-0',campaign+'-1'];
await mkdir(out);const label='nano.qualification=capella-partition';
function run(bin,args,timeout=30000){return new Promise((ok,no)=>{const child=spawn(bin,args,{stdio:['ignore','pipe','pipe']});let stdout='',stderr='',done=false;const timer=setTimeout(()=>child.kill('SIGKILL'),timeout);child.stdout.on('data',x=>{stdout+=x;if(stdout.length>16*1024*1024)child.kill('SIGKILL');});child.stderr.on('data',x=>{stderr+=x;});child.on('error',e=>{clearTimeout(timer);if(!done){done=true;no(e);}});child.on('close',code=>{clearTimeout(timer);if(done)return;done=true;code===0?ok(bin==='docker'&&args[0]==='logs'?stdout+stderr:stdout):no(Error(`${bin} ${args[0]} failed (${code}): ${stderr.slice(-2000)}`));});});}
const docker=args=>run('docker',args),record=async e=>{const line=JSON.stringify({time:new Date().toISOString(),...e});console.log(line);await appendFile(join(out,'events.jsonl'),line+'\n');};
const inspect=async name=>JSON.parse(await docker(['inspect','--format','{{json .}}',name]));
const roots=new Map(),epochs=new Map();let created=[],networkCreated=false,isolated=false,address,daemonPids=[],result='FAILED',failure;
function check(ok,message){if(!ok)throw Error(message);}
async function provenance(name){const s=await inspect(name);check(s.Config.Labels?.['nano.qualification']==='capella-partition'&&s.Config.Labels?.['org.opencontainers.image.revision']===revision,'Container provenance mismatch');return s;}
async function get(i,path){const text=await docker(['exec',names[i],'curl','--silent','--show-error','--max-time','5','-w','\n%{http_code}','-H','Accept: application/json','http://127.0.0.1:8545'+path]);const split=text.lastIndexOf('\n'),status=Number(text.slice(split+1));if(status!==200)throw Object.assign(Error('Beacon HTTP '+status+' for '+path),{httpStatus:status});return JSON.parse(text.slice(0,split));}
async function rpc(i,method){const text=await docker(['exec',names[i],'curl','--fail','--silent','--show-error','--max-time','5','-H','Content-Type: application/json','--data',JSON.stringify({jsonrpc:'2.0',id:1,method,params:[]}),'http://127.0.0.1:8545/']);const j=JSON.parse(text);check(!j.error,'RPC '+method+' failed');return j.result;}
async function sample(){const rows=[];for(let i=0;i<2;++i){const checkpoint=(await get(i,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized,head=(await get(i,'/eth/v1/beacon/headers/head')).data,epoch=BigInt(checkpoint.epoch);check(!epochs.has(i)||epoch>=epochs.get(i),'Finality regressed');check(!roots.has(String(epoch))||roots.get(String(epoch))===checkpoint.root,'Conflicting finalized roots across observations');epochs.set(i,epoch);roots.set(String(epoch),checkpoint.root);rows.push({node:i,checkpoint,headRoot:head.root,slot:head.header.message.slot,peers:await rpc(i,'net_peerCount')});}await record({event:'observation',rows});return rows;}
async function commonFinality(minimum,seconds){const deadline=Date.now()+seconds*1000;let stable=0;while(Date.now()<deadline){const rows=await sample(),a=rows[0].checkpoint,b=rows[1].checkpoint;let good=BigInt(a.epoch)>=minimum&&a.epoch===b.epoch&&a.root===b.root&&rows.every(x=>BigInt(x.peers)>0n);if(good){for(let i=0;i<2;++i){const h=await get(i,'/eth/v1/beacon/headers/finalized');good&&=h.finalized===true&&h.data.root===a.root;}}stable=good?stable+1:0;if(stable>=3)return rows;await wait(2000);}throw Error('Common finality deadline exceeded');}
try{
 const img=JSON.parse(await docker(['image','inspect','--format','{{json .}}',image]));check(img.Config.Labels?.['org.opencontainers.image.revision']===revision,'Image revision mismatch');
 await record({event:'image',id:img.Id,revision,labels:img.Config.Labels});
 await run(resolve(exporter),['--export-network',fixture],60000);
 const meta=JSON.parse(await readFile(join(fixture,'fixture.json'),'utf8'));check(meta.chainId===1337&&meta.nodes===2&&meta.syntheticValidators===64&&meta.fork==='Capella/Shanghai','Unexpected fixture');
 await docker(['network','create','--internal','--label',label,network]);networkCreated=true;
 for(let i=0;i<2;++i){
  if(i===1){const logs=await docker(['logs',names[0]]);const listen=logs.split('\n').map(x=>{try{return JSON.parse(x);}catch{return {};}}).find(x=>x.event==='listen');check(listen,'Missing peer listen address');const left=await provenance(names[0]);const ip=left.NetworkSettings.Networks[network].IPAddress;const cfg=JSON.parse(await readFile(join(fixture,'node-1.json'),'utf8'));cfg.bootnodes=[listen.address.replace('/ip4/0.0.0.0/','/ip4/'+ip+'/')];await writeFile(join(fixture,'node-1.json'),JSON.stringify(cfg));}
  const env=JSON.parse(await readFile(join(fixture,`env-${i}.json`),'utf8')),file=join(out,`fixture-${i}.env`);await writeFile(file,Object.entries(env).map(([k,v])=>k+'='+v).join('\n')+'\n');
  await docker(['create','--name',names[i],'--label',label,'--label','org.opencontainers.image.revision='+revision,'--network',network,'--env-file',file,'--mount','type=bind,src='+fixture+',dst=/config,readonly',image,'run','/config/genesis.json','/var/lib/nano/chain',`/config/node-${i}.json`]);created.push(names[i]);await docker(['start',names[i]]);
  const until=Date.now()+30000;let ready=false;while(Date.now()<until){const logs=await docker(['logs',names[i]]);const events=logs.split('\n').map(x=>{try{return JSON.parse(x);}catch{return {};}}),pid=events.find(x=>x.event==='p2p-process')?.pid;if(events.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)&&Number.isInteger(pid)&&pid>0){daemonPids[i]=pid;ready=true;break;}check((await provenance(names[i])).State.Running,'Node exited during startup');await wait(500);}check(ready,'Node startup deadline');
 }
 const readyDeadline=Date.now()+2*meta.slotsPerEpoch*meta.secondsPerSlot*1000;let headsReady=false;
 while(Date.now()<readyDeadline){try{for(let i=0;i<2;++i)await get(i,'/eth/v1/beacon/headers/head');headsReady=true;break;}catch(e){if(e.httpStatus!==404)throw e;}await wait(1000);}
 check(headsReady,'First signed head unavailable at startup');await record({event:'signed-heads-ready'});
 const before=await commonFinality(2n,240),cutoff=before.reduce((m,x)=>BigInt(x.slot)>m?BigInt(x.slot):m,0n)/BigInt(meta.slotsPerEpoch);
 const remote=await provenance(names[1]);check(Object.keys(remote.NetworkSettings.Networks).length===1,'Unexpected extra container network');address=remote.NetworkSettings.Networks[network].IPAddress;
 isolated=true;await docker(['network','disconnect',network,names[1]]);await record({event:'partition-start',cutoffEpoch:String(cutoff),address,durationSeconds:96});const cutEnd=Date.now()+96000;let sawZero=false;
 while(Date.now()<cutEnd){const state=await provenance(names[1]);check(state.State.Running&&!state.State.Paused&&Object.keys(state.NetworkSettings.Networks).length===0,'Partition ineffective');
  for(let i=0;i<2;++i){const stat=await docker(['exec',names[i],'cat',`/proc/${daemonPids[i]}/stat`]);const processState=stat.slice(stat.lastIndexOf(')')+2).split(' ')[0];check(!['T','t','Z','X'].includes(processState),'P2P daemon not actively running');}
  const rows=await sample();check(rows.every(x=>BigInt(x.checkpoint.epoch)<=cutoff),'Isolated half-stake finalized beyond cut');if(BigInt(rows[1].peers)===0n)sawZero=true;await wait(2000);
 }
 check(sawZero,'No zero-peer observation during partition');const after=await sample();check(after.every((x,i)=>BigInt(x.slot)>BigInt(before[i].slot))&&after[0].headRoot!==after[1].headRoot,'Isolated branches did not advance distinctly');
 await docker(['network','connect','--ip',address,network,names[1]]);isolated=false;const restored=await provenance(names[1]);check(restored.NetworkSettings.Networks[network]?.IPAddress===address,'Original address not restored');
 const minimum=after.reduce((m,x)=>BigInt(x.slot)>m?BigInt(x.slot):m,0n)/BigInt(meta.slotsPerEpoch);await record({event:'partition-restored',minimumFinalizedEpoch:String(minimum)});const final=await commonFinality(minimum,240);result='PASSED';await record({event:'partition-qualified',final});
}catch(e){failure=e;await record({event:'failure',message:e.message});}
finally{
 if(isolated){try{await provenance(names[1]);await docker(['network','connect','--ip',address,network,names[1]]);}catch(e){failure??=e;result='FAILED';await record({event:'restore-failed',message:e.message});}}
 for(const name of created){try{await provenance(name);await writeFile(join(out,name+'.log'),await docker(['logs',name]));await docker(['stop','-t','10',name]);await writeFile(join(out,name+'.log'),await docker(['logs',name]));await docker(['rm','-v',name]);}catch(e){failure??=e;result='FAILED';await record({event:'cleanup-failed',name,message:e.message});}}
 if(networkCreated){try{await docker(['network','rm',network]);}catch(e){failure??=e;result='FAILED';}}
 await writeFile(join(out,'summary.json'),JSON.stringify({result,revision,image,campaign,scope:'Docker packet partition; two running native Capella daemons, 32 validators each',failure:failure?.message??null},null,2)+'\n');
}
if(failure)throw failure;
