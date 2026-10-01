import {execFileSync} from 'node:child_process';
import {readFileSync,appendFileSync} from 'node:fs';
import {pathToFileURL} from 'node:url';
const sha=value=>typeof value==='string'&&/^[0-9a-f]{40}$/.test(value);
export function coreRequired(paths){
 if(!Array.isArray(paths)||paths.some(p=>typeof p!=='string'||!p||p.startsWith('/')||p.includes('\0')||p.split('/').includes('..')))throw Error('Invalid change paths');
 return paths.some(p=>/^(src|include|network|cmake|third_party|genesis)\//.test(p)||/^tests\/(fixtures\/|.*\.(cpp|hpp|h)$)/.test(p)||['CMakeLists.txt','Dockerfile','dependencies.lock.json','scripts/ci/core.sh'].includes(p)||p.startsWith('.github/workflows/'));
}
export function networkRequired(paths){
 coreRequired(paths); // Apply the same path validation before classifying.
 return paths.some(p=>/^(src|include|network|cmake)\//.test(p)||p==='third_party/json.hpp'||['scripts/build-libp2p.sh','scripts/ci/network.sh','scripts/ci/check-network.cmake','scripts/ci/changes.mjs','dependencies.lock.json','Dockerfile'].includes(p)||p.startsWith('.github/workflows/'));
}
export function selectChanges(event,eventName,head,git=args=>execFileSync('git',args,{encoding:'utf8',maxBuffer:16*1024*1024})){
 if(!sha(head)||git(['rev-parse','HEAD']).trim()!==head)throw Error('Unbound checkout revision');
 if(eventName==='workflow_dispatch')return {core:event.inputs?.core===true||event.inputs?.core==='true',network:event.inputs?.network===true||event.inputs?.network==='true',paths:[]};
 let base;
 if(eventName==='push'){base=event.before;if(!sha(base))throw Error('Invalid push baseline');}
 else if(eventName==='pull_request'){base=event.pull_request?.base?.sha;if(!sha(base))throw Error('Invalid PR baseline');base=git(['merge-base',base,head]).trim();if(!sha(base))throw Error('Invalid merge base');}
 else throw Error('Unsupported change event');
 const bytes=base==='0'.repeat(40)?git(['ls-files','-z']):git(['diff','--name-only','--no-renames','-z',base,head,'--']);
 const paths=bytes.split('\0').filter(Boolean);return {core:coreRequired(paths),network:networkRequired(paths),paths};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 try{const result=selectChanges(JSON.parse(readFileSync(process.env.GITHUB_EVENT_PATH,'utf8')),process.env.GITHUB_EVENT_NAME,process.env.GITHUB_SHA);appendFileSync(process.env.GITHUB_OUTPUT,'core='+result.core+'\nnetwork='+result.network+'\n');console.log(JSON.stringify({core:result.core,network:result.network,changedFiles:result.paths.length}));}catch(e){console.error(e.message);process.exitCode=1;}
}
