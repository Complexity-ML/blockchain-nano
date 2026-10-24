// Explicit integration check against an existing local Docker image. No node,
// private mount, environment file, network, pull, build or volume is permitted.
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {randomUUID} from 'node:crypto';
import {mkdir,open,lstat,realpath} from 'node:fs/promises';
import {constants} from 'node:fs';
import {resolve,dirname,join} from 'node:path';
import {performance} from 'node:perf_hooks';
import {dockerRuntimeInspectTemplate,launchDockerRuntimeNoRestart,validateDockerRuntimeInspection,parseDockerCreatedCid} from './docker-runtime-restart-guard.mjs';

const need=(ok,label)=>{if(!ok)throw Error(label);};
const execute=promisify(execFile),owned=[],began=performance.now();
let directory,first=null,normal=null,rejectedAlways=false,closingAt=null;
const cleanupFailures=[];
const check=()=>need(performance.now()<(closingAt===null?began+90000:closingAt+30000),'integration cooperative deadline');
async function docker(args){
 check();
 const end=closingAt===null?began+90000:closingAt+30000;
 const timeout=Math.min(10000,Math.floor(end-performance.now()));need(timeout>0,'remaining integration command budget');
 const {stdout,stderr}=await execute('/usr/bin/sudo',['-n','/usr/bin/docker',...args],{env:{PATH:'/usr/bin:/bin',LANG:'C',LC_ALL:'C'},timeout,maxBuffer:65536});
 check();
 need(!stderr,'unexpected Docker diagnostic');return stdout;
}
async function save(name,value){
 need(directory&&/^[a-z0-9.-]+$/.test(name),'public evidence leaf');
 const fd=await open(join(directory,name),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);
 try{await fd.writeFile(JSON.stringify(value,null,2)+'\n');await fd.sync();}finally{await fd.close();}
}
function identity(v,b){
 need(v.Id===b.cid&&v.Name==='/'+b.name&&v.Image===b.image&&v.Config?.Labels?.['nano.fresh.campaign']===b.uuid&&v.Config.Labels['nano.qualification']==='multihost'&&v.Config.Labels['org.opencontainers.image.revision']===b.revision,'owned test identity');
}
async function inspection(b){const v=JSON.parse(await docker(['inspect',b.cid??b.name,'--format',dockerRuntimeInspectTemplate]));identity(v,b);return v;}
const argvFor=(b,restart)=>['create','--pull=never','--restart='+restart,'--no-healthcheck','--init','--name',b.name,'--label','nano.fresh.campaign='+b.uuid,'--label','nano.qualification=multihost','--label','org.opencontainers.image.revision='+b.revision,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','32m','--cpus','0.1','--pids-limit','16','--user','1000:1000','--entrypoint','/bin/sleep',b.image,'30'];
async function create(b,restart,args=argvFor(b,restart)){
 need(JSON.stringify(args)===JSON.stringify(argvFor(b,restart)),'exact no-key/no-mount sleep argv');
 await save(b.kind+'-creation-intent.json',{format:'nano-docker-policy-check-intent-v1',...b,command:'sleep 30',restart,network:'none',mounts:0,envFiles:0,privateKeys:0});
 owned.push(b); // Discovery by this exact name/UUID/image is retained if create fails.
 const text=await docker(args);
 b.cid=parseDockerCreatedCid(text);return text;
}
try{
 const [image,revision,output,...extra]=process.argv.slice(2);
 need(extra.length===0&&/^sha256:[0-9a-f]{64}$/.test(image??'')&&/^[0-9a-f]{40}$/.test(revision??'')&&output&&resolve(output)===output,'usage: integration.mjs immutable-image source-revision new-output-directory');
 need(await realpath(dirname(output))===dirname(output),'canonical evidence parent');
 const parent=await lstat(dirname(output));need(parent.isDirectory()&&parent.uid===process.getuid(),'owned evidence parent');
 await mkdir(output,{mode:0o700});directory=output;
 const imageMeta=JSON.parse(await docker(['image','inspect',image,'--format','{"Id":{{json .Id}},"Volumes":{{json (index .Config "Volumes")}},"Healthcheck":{{json (index .Config "Healthcheck")}}}']));
 need(imageMeta.Id===image,'existing immutable image');
 need(imageMeta.Volumes===null||(typeof imageMeta.Volumes==='object'&&!Array.isArray(imageMeta.Volumes)&&Object.keys(imageMeta.Volumes).length===0),'image anonymous volumes not allowed');
 need(imageMeta.Healthcheck===null||imageMeta.Healthcheck?.Test?.[0]==='NONE','image active healthcheck not allowed');
 const uuid=randomUUID(),base={image,revision,uuid};
 const good={...base,kind:'no-restart',name:'nano-policy-no-'+uuid};
 const createArgs=argvFor(good,'no');
 normal=await launchDockerRuntimeNoRestart({binding:{name:good.name,image,uuid,revision},createArgs,check,
  docker:async args=>args[0]==='create'?create(good,'no',args):docker(args),
  remember:b=>need(good.cid===b.cid,'same create CID'),
  publish:receipt=>save('no-restart-'+receipt.phase+'.json',receipt),
  launchGate:async start=>start(),onStartAttempt:()=>{}
 });
 need(normal.after.State.Running===true,'sleep process running after admitted start');
 const bad={...base,kind:'always-rejected',name:'nano-policy-always-'+uuid};
 await create(bad,'always');const raw=await docker(['inspect',bad.cid,'--format',dockerRuntimeInspectTemplate]);const projected=JSON.parse(raw);identity(projected,bad);
 need(projected.State.Status==='created'&&projected.State.Running===false&&projected.State.Pid===0&&projected.HostConfig.RestartPolicy.Name==='always','bad policy remains never started');
 try{validateDockerRuntimeInspection(raw,{name:bad.name,image,uuid,revision,cid:bad.cid},{phase:'before-start'});}catch(e){need(String(e).includes('restart policy must be no/0'),'specific bad-policy refusal');rejectedAlways=true;}
 need(rejectedAlways,'always policy must be refused before start');
 await save('always-rejected.json',{format:'nano-docker-policy-negative-v1',cid:bad.cid,name:bad.name,createdNotStarted:true,restartPolicy:projected.HostConfig.RestartPolicy,refused:true,productionQualified:false});
}catch(e){first={name:e?.name??'Error',message:String(e?.message??e).slice(0,512)};}
finally{
 closingAt=performance.now();
 // Stop/remove only this invocation's exact new UUID/name/image. No prune and
 // no old campaign, journal or volume is inspected or changed.
 for(const b of owned.reverse()){
  try{
   if(!b.cid){const text=(await docker(['ps','-a','--no-trunc','--filter','name=^/'+b.name+'$','--filter','label=nano.fresh.campaign='+b.uuid,'--format','{{.ID}}'])).trim();if(!text)continue;b.cid=parseDockerCreatedCid(text);}
   const v=await inspection(b);if(v.State.Running)await docker(['stop','--time','1',b.cid]);
   const stopped=await inspection(b);need(stopped.State.Running===false&&stopped.State.Restarting===false&&stopped.State.Pid===0,'own process stopped');
   await docker(['rm',b.cid]);need(!(await docker(['ps','-a','--no-trunc','--filter','name=^/'+b.name+'$','--format','{{.ID}}'])).trim(),'exact test container absent');
  }catch(e){const failure={name:e?.name??'Error',message:String(e?.message??e).slice(0,512)};cleanupFailures.push(failure);first??=failure;}
 }
}
const result={format:'nano-docker-runtime-policy-integration-v1',passed:first===null&&normal!==null&&rejectedAlways,firstFailure:first,cleanupFailures,elapsedMs:Math.ceil(performance.now()-began),normalCreateInspectStartInspect:normal!==null,alwaysRefusedBeforeStart:rejectedAlways,testContainerCreationsAttempted:owned.length,newTestContainers:owned.filter(b=>b.cid).length,allOwnContainersRemoved:cleanupFailures.length===0,imagePulls:0,imageBuilds:0,volumesCreatedOrRemoved:0,privateMounts:0,keysRead:0,chainRuntimeStarted:false,productionQualified:false};
if(directory)await save('result-public.json',result);
console.log(JSON.stringify(result));if(!result.passed)process.exitCode=1;
