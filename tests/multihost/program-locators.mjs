// Pure path/pin semantics only. Actual canonical path, FD, owner, nlink,
// architecture and byte checks remain the launcher's independent preflight.
import {posix} from 'node:path';

const leaves=Object.freeze({coreArchive:'libnano_core.a',nano:'nano',p2p:'nano-p2p',exporter:'nano-capella-transition-tests',bootstrapImport:'nano-capella-isolated-bootstrap-import',bootstrapAudit:'nano-capella-isolated-bootstrap-audit',stoppedCapture:'nano-capella-isolated-stopped-capture',rotationAudit:'nano-capella-rotation-audit',lifecycleTool:'nano-capella-deposit-audit',lifecycleSubmit:'nano-capella-lifecycle-submit'});
const owners=new Set(['mac-native','mac-docker','vps-docker']);
// These are runtime/control and operating-system namespaces, never new
// explicit host software locations. An unchanged legacy root is not remapped.
const protectedNamespaces=['/outputs','/network-build','/core','/config','/proof','/export','/work','/control','/data','/witness','/known','/bootstrap','/rotation-audit','/action-output','/lifecycle-owner','/lifecycle-results','/lifecycle-remote','/staged','/audit-inputs','/stop.json','/rotation-proof.json','/request.json','/var/lib','/run','/tmp','/private/tmp','/private/var','/proc','/sys','/dev','/etc','/bin','/sbin','/usr','/lib','/lib64','/System','/Library','/Applications','/opt/homebrew'];
const within=(value,parent)=>value===parent||value.startsWith(parent+'/');
const overlap=(a,b)=>within(a,b)||within(b,a);
const need=(value,why)=>{if(!value)throw Error('Fresh software locator: '+why);};
function absolute(value){
 need(typeof value==='string'&&value.length>1&&value.length<4096&&value.startsWith('/')&&!value.endsWith('/')&&posix.normalize(value)===value&&!/[\0\r\n,\\]/.test(value),'normalized absolute physical locator');return value;
}
function artifact(value,role){
 need(value&&typeof value==='object'&&!Array.isArray(value)&&Object.keys(value).sort().join(',')==='bytes,path,sha256','exact '+role+' Artifact');
 absolute(value.path);need(/^[0-9a-f]{64}$/.test(value.sha256)&&Number.isSafeInteger(value.bytes)&&value.bytes>0&&value.bytes<=1073741824,'known '+role+' pin');
 need(posix.basename(value.path)===leaves[role],role+' exact software leaf');return value;
}

export function freshSoftwareLocators(name,owner,{protectedPaths=[]}={}){
 need(owners.has(name)&&owner&&typeof owner==='object'&&!Array.isArray(owner),'known owner');
 const core=absolute(owner.coreBuildRoot),network=absolute(owner.networkBuildRoot),native=name==='mac-native';
 need(!overlap(core,network),'disjoint legacy software roots');
 need(Array.isArray(protectedPaths)&&protectedPaths.every(p=>absolute(p)),'explicit protected physical roots');
 need(owner.programs&&typeof owner.programs==='object'&&!Array.isArray(owner.programs)&&Object.keys(owner.programs).length>0,'selected program roles');
 const selected=[['coreArchive',owner.coreArchive],...Object.entries(owner.programs)];
 need(selected.every(([role])=>Object.hasOwn(leaves,role)),'known software roles only');
 const rows=selected.map(([role,value])=>{
  const pin=artifact(value,role),isP2p=role==='p2p',legacyRuntime=(native?(isP2p?network:core):isP2p?'/network-build':'/outputs')+'/'+leaves[role],legacy=pin.path===legacyRuntime;
  // Keep the separately admitted P2P root and its dependency closure unchanged.
  need(!isP2p||legacy,'P2P remains in its admitted network root');
  if(!legacy){
   need(!protectedNamespaces.some(p=>overlap(pin.path,p)),'explicit guest destination collides with runtime or system namespace');
   need(!overlap(pin.path,network)&&!within(core,pin.path),'explicit software file overlaps an admitted code root');
  }
  const hostPath=native||!legacy?pin.path:(isP2p?network:core)+'/'+leaves[role];
  need(!protectedPaths.some(p=>overlap(hostPath,p)),'software file overlaps protected physical path');
  return Object.freeze({role,runtimePath:pin.path,hostPath,sha256:pin.sha256,bytes:pin.bytes,legacy});
 });
 for(let i=0;i<rows.length;++i)for(let j=i+1;j<rows.length;++j){
  need(!overlap(rows[i].runtimePath,rows[j].runtimePath),'software guest destinations collide or overlap');
  need(!overlap(rows[i].hostPath,rows[j].hostPath),'software physical sources collide or overlap');
 }
 return Object.freeze(rows);
}

export function freshSoftwareFileMounts(name,owner,options){
 const rows=freshSoftwareLocators(name,owner,options);
 if(name==='mac-native')return [];
 // Every extra mount is a known regular software FILE at its exact selected
// runtime locator. Existing /outputs and /network-build directory mounts stay.
 return rows.filter(row=>!row.legacy).flatMap(row=>['--mount',`type=bind,src=${row.hostPath},dst=${row.runtimePath},readonly`]);
}
