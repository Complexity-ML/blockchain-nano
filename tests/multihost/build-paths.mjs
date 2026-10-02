// Select existing read-only build artifacts; never copy or compile here.
export function selectBuildPaths(buildVolume,subdirectory){
 if(subdirectory!==undefined&&!buildVolume)throw Error('Build subdirectory requires build-volume mode');
 if(subdirectory!==undefined&&(typeof subdirectory!=='string'||!/^[a-zA-Z0-9]/.test(subdirectory)||/[^a-zA-Z0-9_.-]/.test(subdirectory)))throw Error('Invalid build subdirectory');
 const directory=subdirectory===undefined?'/build':`/build/${subdirectory}`;
 return {directory,nano:`${directory}/nano`,revision:`${directory}/production-source-revision.txt`,rotationAudit:`${directory}/nano-capella-rotation-audit`,p2p:'/build/network/nano-p2p'};
}
// Both architecture selectors must be explicit unless a common fallback exists.
export function selectHostBuildPaths(buildVolume,common,arm64,amd64){
 if(common===undefined&&(arm64===undefined)!==(amd64===undefined))throw Error('Both architecture build selectors are required');
 return {arm64:selectBuildPaths(buildVolume,arm64===undefined?common:arm64),amd64:selectBuildPaths(buildVolume,amd64===undefined?common:amd64)};
}
export function selectRemoteSourceRepository(user,requested){
 const path=requested===undefined?`/home/${user}/blockchain-nano-git`:requested;
 if(typeof path!=='string'||!path.startsWith('/')||path==='/'||path.split('/').some(x=>x==='.'||x==='..')||/[^a-zA-Z0-9_./-]/.test(path)||path.includes('//'))throw Error('Invalid remote source repository');
 return path;
}
export function remoteDockerArgument(argument,localImage,remoteImage,buildVolume,paths){
 if(argument===localImage)return remoteImage;
 if(buildVolume)for(const key of ['nano','revision','rotationAudit'])if(argument===paths.arm64[key])return paths.amd64[key];
 return argument;
}
