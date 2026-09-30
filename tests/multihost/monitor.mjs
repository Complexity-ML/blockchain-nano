import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,writeFile,mkdir,appendFile} from 'node:fs/promises';
import {join} from 'node:path';
import {createHash} from 'node:crypto';
import {setTimeout as delay} from 'node:timers/promises';
const exec=promisify(execFile);
export async function startFinalityMonitor({repo,output,prepared,local,remote,target,record}){
 const directory=join(output,'monitor');await mkdir(directory);
 const initial=JSON.parse(await readFile(join(prepared,'mac-native/initial.json'),'utf8'));
 const genesis=initial.ancestors.find(b=>BigInt(b.number)===0n)?.hash;
 if(!/^0x[0-9a-f]{64}$/.test(genesis??''))throw Error('Trusted fixture genesis missing');
 const profile=(await readFile(join(prepared,'mac-native/profile.json'),'utf8')).trim();
 if(JSON.stringify(JSON.parse(profile))!==profile)throw Error('Noncanonical trusted profile');
 const config={chainId:'1337',genesisHash:genesis,configName:'nano-capella-'+createHash('sha256').update(profile).digest('hex'),staleMs:96000,timeoutMs:5000,maximumResponseBytes:1048576,maximumEpochs:256,minimumPeers:1,nodes:[
  {name:'mac-native',kind:'http',url:'http://127.0.0.1:18545/'},
  {name:'mac-docker',kind:'docker',container:local,url:'http://127.0.0.1:8545/'},
  {name:'vps-docker',kind:'ssh-docker',container:remote,ssh:target,url:'http://127.0.0.1:8545/'}]};
 const configFile=join(directory,'config.json'),stateFile=join(directory,'state.json');
 await writeFile(configFile,JSON.stringify(config,null,2)+'\n');
 const invoke=async(file,state)=>{
  try{const r=await exec(process.execPath,[join(repo,'scripts/operations/finality-monitor.mjs'),file,state],{timeout:45000,maxBuffer:2*1024*1024});return {code:0,report:JSON.parse(r.stdout)};}
  catch(e){if(e.code!==2||!e.stdout)throw e;return {code:2,report:JSON.parse(e.stdout)};}
 };
 const negativeFile=join(directory,'wrong-genesis.json');
 await writeFile(negativeFile,JSON.stringify({...config,genesisHash:'0x'+'0'.repeat(64)},null,2)+'\n');
 const negative=await invoke(negativeFile,join(directory,'wrong-genesis-state.json'));
 if(negative.code!==2||negative.report.healthy||negative.report.samples.length!==3||negative.report.samples.some(s=>s.error!=='Chain/genesis mismatch'))throw Error('Live monitor accepted wrong genesis');
 await writeFile(join(directory,'wrong-genesis-result.json'),JSON.stringify(negative,null,2)+'\n');
 await record({event:'monitor-wrong-genesis-rejected',nodes:3});
 let stopping=false,failure,observations=0,first,last;
 const poll=async()=>{
  const r=await invoke(configFile,stateFile);await appendFile(join(directory,'samples.jsonl'),JSON.stringify(r)+'\n');
  if(r.code||!r.report.healthy||r.report.safetyFaults.length)throw Error('Live finality monitor unhealthy: '+JSON.stringify(r.report.alerts));
  ++observations;const epochs=r.report.samples.map(s=>BigInt(s.epoch));
  first??=epochs;last=epochs;
 };
 // Finite shadow observation; it never relaxes the existing stability gate.
 const deadline=Date.now()+360000;
 const task=(async()=>{while(!stopping){if(Date.now()>deadline||observations>=100)throw Error('Monitor finite budget exhausted');await poll();for(let i=0;i<30&&!stopping;i++)await delay(100);}})().catch(e=>{failure=e;});
 return {stop:async(qualify=false)=>{
  stopping=true;await task;if(failure)throw failure;
  if(qualify){await poll();if(observations<2||last.some((e,i)=>e<first[i]+3n))throw Error('Monitor did not observe three finalized advances on every node');
   const result={observations,firstEpochs:first.map(String),lastEpochs:last.map(String),wrongGenesisRejected:true,transport:'native HTTP, local Docker exec, remote SSH Docker exec',independentFixtureGenesis:true,executionBindingVerifiedBySeparateStabilityGate:true};
   await writeFile(join(directory,'result.json'),JSON.stringify(result,null,2)+'\n');await record({event:'monitor-passed',...result});}
 }};
}
