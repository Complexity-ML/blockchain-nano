// Image selection/integrity only; production readiness remains a separate gate.
const hash=s=>typeof s==='string'&&/^[0-9a-f]{64}$/.test(s);
export function selectCandidates(manifest,revision){
 const out={};
 for(const arch of ['arm64','amd64']){
  const s=manifest?.images?.[arch];
  if(s?.integrityVerified!==true||s.architecture!==arch||s.sourceRevision!==revision||!/^sha256:[0-9a-f]{64}$/.test(s.image??'')||!hash(s.executables?.nano)||!hash(s.executables?.['nano-p2p']))throw Error('Reviewed candidate image/revision/hashes required');
  out[arch]=s;
 }
 if(out.arm64.image===out.amd64.image)throw Error('Architecture images must be distinct');
 return out;
}
export function verifyPackagedImage(expected,info,text){
 if(info.Id!==expected.image||info.Architecture!==expected.architecture||info.Config?.Labels?.['org.opencontainers.image.revision']!==expected.sourceRevision||info.Config.User!=='10001:10001')throw Error('Packaged identity/platform/revision/user mismatch');
 const found={};
 for(const line of text.trim().split('\n')){const m=/^([0-9a-f]{64})\s+\/usr\/local\/bin\/(nano|nano-p2p)$/.exec(line);if(!m||found[m[2]])throw Error('Invalid packaged checksum output');found[m[2]]=m[1];}
 if(found.nano!==expected.executables.nano||found['nano-p2p']!==expected.executables['nano-p2p'])throw Error('Packaged executable hash mismatch');
 return found;
}
