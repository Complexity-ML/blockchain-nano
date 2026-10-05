import {readFile,writeFile,mkdir,cp} from 'node:fs/promises';
import {execFile} from 'node:child_process';import {promisify} from 'node:util';import assert from 'node:assert/strict';import {randomBytes} from 'node:crypto';
import {pinnedDepositRestart} from '/Users/boris/Dev/blockchain-nano/tests/capella/deposit-restart-pins.mjs';
const exec=promisify(execFile),source='/private/tmp/nano-deposit-resume-v2-20261005',out='/private/tmp/nano-startup-cold-activation-20261005',name='nano-startup-cold-activation-arm64-20261005',image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';
const json=async p=>JSON.parse(await readFile(p,'utf8'));
const docker=async args=>(await exec('docker',args,{timeout:30000,maxBuffer:16*1024*1024})).stdout;
let made=false,failure=null;
try{
 for(const pid of [97373,422]){let present=true;try{process.kill(pid,0);}catch(e){if(e.code==='ESRCH')present=false;else throw e;}assert(!present,'Source signer PID still present');}
 const receipt=await json('/private/tmp/nano-deposit-resume-v2-offline-audits-20261005/0-external-stop-receipt.json'),capsule=await json('/private/tmp/nano-cold-clock-diagnostic-evidence-20261005/capture.json');assert.deepEqual(capsule.restart,receipt.restart);
 for(const k of capsule.validatorKeys){const saved=receipt.validatorKeys.find(x=>BigInt(x.index)===BigInt(k.index));assert(saved);assert.deepEqual(k.restart,saved.restart);assert.equal(k.retired,false);}
 await mkdir(out);await cp(source+'/mac-data',out+'/data',{recursive:true,errorOnExist:true,force:false});await cp(source+'/fixture',out+'/config',{recursive:true,errorOnExist:true,force:false});
 const pending=(await json(source+'/fixture/deposit.json')).publicKey;
 let config=pinnedDepositRestart(await json(source+'/mac.json'),receipt,pending);
 config=JSON.parse(JSON.stringify(config).split(source+'/mac-data').join('/var/lib/nano').split(source+'/fixture').join('/config'));
 config.capella.network.enabled=false;config.capella.clock.startupCatchup={maximumDowntimeMs:'0x5265c00',maximumSteps:'0x10'};
 await writeFile(out+'/config/serve.json',JSON.stringify({capella:config.capella},null,2)+'\n');
 const env=await json(source+'/fixture/env-0.json');for(let i=0;i<32;i++)assert.equal(BigInt(env['NANO_TEST_CAPELLA_NETWORK_KEY_'+i]),BigInt(i+1));
 const token='0x'+randomBytes(32).toString('hex');await writeFile(out+'/env',Object.entries({...env,NANO_NETWORK_TOKEN:token}).map(([k,v])=>k+'='+v).join('\n')+'\n',{mode:0o600});
 await docker(['create','--name',name,'--network','none','--read-only','--cpus','1','--memory','2g','--env-file',out+'/env','--mount','type=volume,src=nano-clean-arm64-build-20261002,dst=/build,readonly','--mount',`type=bind,src=${out}/data,dst=/var/lib/nano`,'--mount',`type=bind,src=${out}/config,dst=/config,readonly`,'--entrypoint','/build/startup-catchup-6b7a437e-arm64-20261005/nano',image,'serve','/config/genesis.json','/var/lib/nano/chain','0','127.0.0.1','/config/serve.json']);made=true;await docker(['start',name]);
 // Bounded observation only: no transactions, flood, public RPC or peer relay.
 await new Promise(r=>setTimeout(r,20000));
 const state=JSON.parse(await docker(['inspect',name,'--format','{{json .State}}']));await writeFile(out+'/observed-state.json',JSON.stringify(state,null,2)+'\n');
}catch(e){failure=e.stack;process.exitCode=1;}
finally{
 if(made){const state=JSON.parse(await docker(['inspect',name,'--format','{{json .State}}']));if(state.Running)await docker(['stop','--time','20',name]);await writeFile(out+'/state.json',await docker(['inspect',name,'--format','{{json .State}}']));await writeFile(out+'/runtime.log',await docker(['logs',name]));}
 const log=made?await readFile(out+'/runtime.log','utf8'):'';const rows=log.split('\n').flatMap(l=>{try{return [JSON.parse(l)]}catch{return []}}),catchup=rows.find(r=>r.event==='capella-startup-catchup-completed'),stop=rows.find(r=>r.event==='capella-runtime-stopped');
 if(made)await writeFile(out+'/result.json',JSON.stringify({failure,container:name,sourceSnapshotPreserved:true,controlledCopy:true,clockCatchupCompleted:!!catchup,catchup:catchup??null,validatorHealthy:stop?.validator?.healthy??null,validatorFailure:stop?.validator?.failure??null,proposals:stop?.validator?.proposalsAdmitted??null,attestations:stop?.validator?.attestationsAdmitted??null,networkQualified:false,productionQualified:false},null,2)+'\n');
 console.log(JSON.stringify({failure,clockCatchupCompleted:!!catchup,validatorHealthy:stop?.validator?.healthy??null,validatorFailure:stop?.validator?.failure??null}));
}
