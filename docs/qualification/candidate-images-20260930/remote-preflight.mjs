// Read-only qualification adapter: artifacts are read locally, Docker probes run
// on the authorized VPS. No Node installation on VPS or chain mounts required.
import {verifyCandidate} from '../../../scripts/operations/check-candidate.mjs';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile} from 'node:fs/promises';
const exec=promisify(execFile),base=new URL('./',import.meta.url);
const quote=s=>"'"+s.replaceAll("'","'\\''")+"'";
const run=async args=>(await exec('ssh',['-o','BatchMode=yes','ubuntu@145.239.199.166', ['sudo','-n','docker',...args].map(quote).join(' ')],{timeout:30000,maxBuffer:1048576})).stdout;
const sandbox=['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1'];
const manifest=JSON.parse(await readFile(new URL('amd64-reviewed.json',base),'utf8'));
const result=await verifyCandidate(manifest,{
 read:async name=>readFile(new URL(name,base)),
 daemon:async()=>{const [os,a]=(await run(['info','--format','{{.OSType}} {{.Architecture}}'])).trim().split(/\s+/);return {os,architecture:({x86_64:'amd64',aarch64:'arm64'})[a]??a};},
 inspect:async id=>JSON.parse(await run(['image','inspect',id,'--format','{{json .}}'])),
 digests:async id=>{const s=await run([...sandbox,'--entrypoint','/usr/bin/sha256sum',id,'/usr/local/bin/nano','/usr/local/bin/nano-p2p']);const out={};for(const line of s.trim().split('\n')){const m=/^([0-9a-f]{64})\s+\/usr\/local\/bin\/(nano|nano-p2p)$/.exec(line);if(!m||out[m[2]])throw Error('Invalid checksum');out[m[2]]=m[1];}return out;},
 libraries:async id=>run([...sandbox,'--entrypoint','/usr/bin/ldd',id,'/usr/local/bin/nano','/usr/local/bin/nano-p2p'])
});
console.log(JSON.stringify({...result,transport:'SSH read-only Docker probes; artifact hashes checked on Mac'}));
