// Pure projection/parser and injected coordinator. No Docker/process/FS API.
const need=(ok,why)=>{if(!ok)throw Error('Docker runtime restart guard: '+why);};
const exact=(v,keys,label)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join('\0')===[...keys].sort().join('\0'),label+' exact fields');
const cidPattern=/^[0-9a-f]{64}$/;
const labelKeys=['nano.fresh.campaign','nano.qualification','org.opencontainers.image.revision'];
// Never request Config.Env, Mounts or the full container object. Only these
// three already-public labels and the fields consumed below leave the engine.
export const dockerRuntimeInspectTemplate='{"Id":{{json .Id}},"Name":{{json .Name}},"Image":{{json .Image}},"Config":{"Labels":{"nano.fresh.campaign":{{json (index .Config.Labels "nano.fresh.campaign")}},"nano.qualification":{{json (index .Config.Labels "nano.qualification")}},"org.opencontainers.image.revision":{{json (index .Config.Labels "org.opencontainers.image.revision")}}}},"State":{"Running":{{json .State.Running}},"Restarting":{{json .State.Restarting}},"Paused":{{json .State.Paused}},"Dead":{{json .State.Dead}},"Pid":{{json .State.Pid}},"Status":{{json .State.Status}}},"HostConfig":{"RestartPolicy":{"Name":{{json .HostConfig.RestartPolicy.Name}},"MaximumRetryCount":{{json .HostConfig.RestartPolicy.MaximumRetryCount}}}}}';
export const dockerRuntimeNetworkInspectTemplate=dockerRuntimeInspectTemplate.slice(0,-1)+',"NetworkSettings":{"Networks":{{json .NetworkSettings.Networks}}}}';

function authority(value,withCid){
 exact(value,['name','image','uuid','revision',...(withCid?['cid']:[])],'binding');
 need(typeof value.name==='string'&&/^[A-Za-z0-9][A-Za-z0-9_.-]{0,127}$/.test(value.name),'exact container name');
 need(typeof value.image==='string'&&/^sha256:[0-9a-f]{64}$/.test(value.image),'immutable image ID');
 need(typeof value.uuid==='string'&&/^[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}$/.test(value.uuid),'operator UUID');
 need(typeof value.revision==='string'&&/^[0-9a-f]{40}$/.test(value.revision),'source revision');
 if(withCid)need(typeof value.cid==='string'&&cidPattern.test(value.cid),'full returned CID');
 return Object.freeze({...value});
}
export function parseDockerCreatedCid(text){
 need(typeof text==='string'&&/^[0-9a-f]{64}\n?$/.test(text),'one full created CID');return text.endsWith('\n')?text.slice(0,-1):text;
}
function identityProjection(text,expected){
 need(typeof text==='string'&&text.length>0&&text.length<=65536,'bounded projected inspection');
 let value;try{value=JSON.parse(text);}catch{throw Error('Docker runtime restart guard: projected inspection JSON');}
 exact(value,['Id','Name','Image','Config','State','HostConfig'],'projection');exact(value.Config,['Labels'],'projected config');exact(value.Config.Labels,labelKeys,'projected labels');
 need(value.Id===expected.cid&&value.Name==='/'+expected.name&&value.Image===expected.image,'CID/name/immutable image binding');
 const labels=value.Config.Labels;need(labels['nano.fresh.campaign']===expected.uuid&&labels['nano.qualification']==='multihost'&&labels['org.opencontainers.image.revision']===expected.revision,'exact operator labels');
 return value;
}
// Dedicated cleanup identity gate: policy/state drift must not prevent stopping
// this exact owned container. It never admits a runtime or authorizes a start.
export function validateDockerRuntimeIdentityForCleanup(text,binding){
 const value=identityProjection(text,authority(binding,true));return Object.freeze({Id:value.Id,Name:value.Name,Image:value.Image,Config:Object.freeze({Labels:Object.freeze({...value.Config.Labels})})});
}
export function validateDockerRuntimeInspection(text,binding,{phase='either'}={}){
 const expected=authority(binding,true);need(['before-start','after-start','either'].includes(phase),'inspection phase');
 const value=identityProjection(text,expected),labels=value.Config.Labels;
 exact(value.State,['Running','Restarting','Paused','Dead','Pid','Status'],'projected state');exact(value.HostConfig,['RestartPolicy'],'projected host config');exact(value.HostConfig.RestartPolicy,['Name','MaximumRetryCount'],'restart policy');
 const policy=value.HostConfig.RestartPolicy;need(policy.Name==='no'&&Number.isSafeInteger(policy.MaximumRetryCount)&&policy.MaximumRetryCount===0,'restart policy must be no/0');
 const state=value.State;need(typeof state.Running==='boolean'&&state.Restarting===false&&state.Paused===false&&state.Dead===false&&Number.isSafeInteger(state.Pid)&&state.Pid>=0,'non-restarting live/stopped state');
 const stopped=state.Running===false&&state.Pid===0&&['created','exited'].includes(state.Status),running=state.Running===true&&state.Pid>0&&state.Status==='running';
 need(phase==='before-start'?stopped&&state.Status==='created':phase==='after-start'?running:stopped||running,'phase-consistent process state');
 return Object.freeze({Id:value.Id,Name:value.Name,Image:value.Image,Config:Object.freeze({Labels:Object.freeze({...labels})}),State:Object.freeze({...state}),HostConfig:Object.freeze({RestartPolicy:Object.freeze({...policy})})});
}
function receipt(value,phase){
 return Object.freeze({format:'nano-docker-runtime-restart-policy-v1',phase,cid:value.Id,name:value.Name.slice(1),image:value.Image,operatorUuid:value.Config.Labels['nano.fresh.campaign'],sourceBaseRevision:value.Config.Labels['org.opencontainers.image.revision'],state:value.State,restartPolicy:value.HostConfig.RestartPolicy,projectedFieldsOnly:true,restartPolicyObserved:true,automaticRestartAllowed:false,observationIsNotContinuousPolicyGuarantee:true,productionQualified:false});
}
export async function launchDockerRuntimeNoRestart({binding,createArgs,docker,remember,publish,launchGate,onStartAttempt,check}){
 const expected=authority(binding,false);need([docker,remember,publish,launchGate,onStartAttempt,check].every(x=>typeof x==='function'),'complete injected capabilities');
 need(Array.isArray(createArgs)&&createArgs.length>3&&createArgs.every(x=>typeof x==='string'&&x.length>0&&x.length<=131072&&!x.includes('\0')),'bounded create argv');
 const argv=[...createArgs],imageAt=argv.indexOf(expected.image);need(argv[0]==='create'&&imageAt>0,'explicit create and selected image');
 const options=argv.slice(1,imageAt),restart=options.filter(x=>x==='--restart'||x.startsWith('--restart='));
 need(restart.length===1&&restart[0]==='--restart=no'&&!options.includes('--rm')&&!options.includes('-d'),'one explicit no-restart retained create');
 need(options.filter(x=>x==='--name').length===1&&options[options.indexOf('--name')+1]===expected.name,'selected create name');
 check();const cid=parseDockerCreatedCid(await docker(argv)),bound=Object.freeze({...expected,cid});
 const remembered=remember(bound);need(!remembered||typeof remembered.then!=='function','synchronous CID ownership admission');
 check();
 const inspect=async phase=>validateDockerRuntimeInspection(await docker(['inspect',cid,'--format',dockerRuntimeInspectTemplate]),bound,{phase});
 const before=await inspect('before-start');check();await publish(receipt(before,'before-start'));check();
 let attempts=0,started=false,startPromise=null,gateOpen=true,first,failed=false;
 const retain=error=>{if(!failed){first=error;failed=true;}else if(first!==error&&first!==null&&typeof first==='object'){try{Object.defineProperty(first,'dockerRuntimeStartSecondaryFailure',{value:'Owned explicit Docker start also failed while joining',configurable:true});}catch{}}};
 const start=()=>{
  need(gateOpen,'start callback is no longer admitted');++attempts;need(attempts===1,'one explicit start attempt');
  startPromise=(async()=>{check();const intent=onStartAttempt(bound);need(!intent||typeof intent.then!=='function','synchronous start intent');check();need(parseDockerCreatedCid(await docker(['start',cid]))===cid,'start acknowledged selected CID');started=true;check();})();
  // A gate may throw or forget await. Retain ownership of its promise here and
  // join it before reporting either success or the gate's original failure.
  startPromise.catch(()=>{});return startPromise;
 };
 try{await launchGate(start);}catch(error){retain(error);}finally{gateOpen=false;}
 if(startPromise)try{await startPromise;}catch(error){retain(error);}
 if(!failed)try{need(attempts===1&&started,'launch gate must invoke exactly one explicit start');}catch(error){retain(error);}
 if(failed)throw first;
 const after=await inspect('after-start');check();await publish(receipt(after,'after-start'));check();
 return Object.freeze({binding:bound,before,after});
}
