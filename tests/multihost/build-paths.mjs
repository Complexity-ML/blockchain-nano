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
export function remoteDockerArgument(argument,localImage,remoteImage,buildVolume,paths){
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
