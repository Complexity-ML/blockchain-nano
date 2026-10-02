// Select existing read-only build artifacts; never copy or compile here.
export function selectBuildPaths(buildVolume,subdirectory){
 if(subdirectory!==undefined&&!buildVolume)throw Error('Build subdirectory requires build-volume mode');
 if(subdirectory!==undefined&&(typeof subdirectory!=='string'||!/^[a-zA-Z0-9]/.test(subdirectory)||/[^a-zA-Z0-9_.-]/.test(subdirectory)))throw Error('Invalid build subdirectory');
 const directory=subdirectory===undefined?'/build':`/build/${subdirectory}`;
 return {directory,nano:`${directory}/nano`,revision:`${directory}/production-source-revision.txt`,rotationAudit:`${directory}/nano-capella-rotation-audit`,p2p:'/build/network/nano-p2p'};
}
