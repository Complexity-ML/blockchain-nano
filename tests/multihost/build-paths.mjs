// Select existing read-only build artifacts; never copy or compile here.
export function selectBuildPaths(buildVolume,subdirectory,networkWithinBuild=false){
 if(typeof networkWithinBuild!=="boolean")throw Error("Invalid network build selector");
 if(networkWithinBuild&&!buildVolume)throw Error("Network build selector requires build-volume mode");
 if(subdirectory!==undefined&&!buildVolume)throw Error('Build subdirectory requires build-volume mode');
 if(subdirectory!==undefined&&(typeof subdirectory!=='string'||!/^[a-zA-Z0-9]/.test(subdirectory)||/[^a-zA-Z0-9_.-]/.test(subdirectory)))throw Error('Invalid build subdirectory');
 const directory=subdirectory===undefined?'/build':`/build/${subdirectory}`;
 return {directory,nano:`${directory}/nano`,revision:`${directory}/production-source-revision.txt`,rotationAudit:`${directory}/nano-capella-rotation-audit`,p2p:networkWithinBuild?`${directory}/network/nano-p2p`:'/build/network/nano-p2p'};
}
// Both architecture selectors must be explicit unless a common fallback exists.
export function selectHostBuildPaths(buildVolume,common,arm64,amd64,networkWithinBuild=false){
 if(common===undefined&&(arm64===undefined)!==(amd64===undefined))throw Error('Both architecture build selectors are required');
 return {arm64:selectBuildPaths(buildVolume,arm64===undefined?common:arm64,networkWithinBuild),amd64:selectBuildPaths(buildVolume,amd64===undefined?common:amd64,networkWithinBuild)};
}
export function selectRemoteSourceRepository(user,requested){
 const path=requested===undefined?`/home/${user}/blockchain-nano-git`:requested;
 if(typeof path!=='string'||!path.startsWith('/')||path==='/'||path.split('/').some(x=>x==='.'||x==='..')||/[^a-zA-Z0-9_./-]/.test(path)||path.includes('//'))throw Error('Invalid remote source repository');
 return path;
}
export function remoteDockerArgument(argument,localImage,remoteImage,buildVolume,paths,artifacts){
 if(buildVolume&&artifacts&&argument===artifacts.arm64.mount)return artifacts.amd64.mount;
 if(argument===localImage)return remoteImage;
 if(buildVolume)for(const key of ['nano','revision','rotationAudit','p2p'])if(argument===paths.arm64[key])return paths.amd64[key];
 return argument;
}

// Explicit native selections avoid silently using an old shared daemon.
export function selectNativeBuildPaths(repository,coreDirectory,networkDirectory){
 const check=path=>{
  if(typeof path!=='string'||!path.startsWith('/')||path==='/'||path.split('/').some(x=>x==='.'||x==='..')||/[^a-zA-Z0-9_./-]/.test(path)||path.includes('//'))throw Error('Invalid native build directory');
  return path;
 };
 const core=check(coreDirectory===undefined?`${repository}/build-release`:coreDirectory);
 const network=check(networkDirectory===undefined?`${repository}/build-network`:networkDirectory);
 return {nano:`${core}/nano`,p2p:`${network}/nano-p2p`,rotationAudit:`${core}/nano-capella-rotation-audit`,transitionTests:{bellatrix:`${core}/nano-bellatrix-transition-tests`,capella:`${core}/nano-capella-transition-tests`}};
}

// Explicit existing images/volumes must be complete and cannot escape Docker argv.
export function selectBuildArtifacts(buildVolume,armImage,amdImage,armVolume,amdVolume){
 if(typeof buildVolume!=='boolean')throw Error('Invalid build-volume mode');
 const values=[armImage,amdImage,armVolume,amdVolume];
 const explicit=values.some(v=>v!==undefined);
 if(explicit&&!buildVolume)throw Error('Explicit artifact selection requires build-volume mode');
 if(explicit&&values.some(v=>v===undefined))throw Error('Both images and volumes are required');
 const image=v=>{if(typeof v!=='string'||v.length!==71||!/^sha256:[a-f0-9]{64}$/.test(v))throw Error('Invalid immutable builder image');return v;};
 const volume=v=>{if(typeof v!=='string'||!v||!/^[a-zA-Z0-9]/.test(v)||/[^a-zA-Z0-9_.-]/.test(v))throw Error('Invalid existing build volume');return v;};
 const artifact=(i,v)=>({image:i,volume:v,mount:`type=volume,src=${v},dst=/build,readonly`});
 return explicit?{arm64:artifact(image(armImage),volume(armVolume)),amd64:artifact(image(amdImage),volume(amdVolume))}:{arm64:artifact('nano-build-dependencies:local','nano-linux-sync-build'),amd64:artifact('nano-build-dependencies:local','nano-linux-sync-build')};
}

// Majority isolation can reuse qualified programs in read-only build volumes.
// Legacy mutable builder tags remain ineligible for this qualification mode.
export function qualifiedRetainedArtifacts(buildVolume,artifacts){
 if(typeof buildVolume!=='boolean')throw Error('Invalid build-volume mode');
 if(!buildVolume)return false;
 return ['arm64','amd64'].every(arch=>{
  const a=artifacts?.[arch];
  return typeof a?.image==='string'&&/^sha256:[a-f0-9]{64}$/.test(a.image)
   &&typeof a.volume==='string'&&/^[a-zA-Z0-9][a-zA-Z0-9_.-]*$/.test(a.volume)
   &&a.mount===`type=volume,src=${a.volume},dst=/build,readonly`;
 });
}
