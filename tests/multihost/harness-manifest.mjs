import {readFile,realpath} from 'node:fs/promises';
import {resolve,relative,dirname,isAbsolute} from 'node:path';
import {createHash} from 'node:crypto';
// Static relative ES-module imports used by this repository; external commands
// are explicit roots. This is a source inventory, not executable attestation.
export const harnessRoots=['tests/multihost/run.mjs','tests/multihost/observe.mjs','tests/multihost/prepare.mjs','tests/multihost/backup-restore.sh','scripts/operations/finality-monitor.mjs','scripts/operations/monitor-and-deliver.mjs','docs/qualification/local-supervisor-linux-20261015/source/known-files.pl','docs/qualification/local-supervisor-linux-20261015/source/copy-bootstrap-pairs.pl'];
export async function hashHarness(repo,entries=harnessRoots){
 const root=await realpath(repo),base=resolve(root,'tests/multihost'),seen=new Set(),hashes={};
 async function visit(path){
  const absolute=await realpath(path),local=relative(root,absolute);
  if(isAbsolute(local)||local==='..'||local.startsWith('../'))throw Error('Harness source escapes repository');
  if(seen.has(absolute))return;seen.add(absolute);
  const bytes=await readFile(absolute);hashes[relative(base,absolute)]=createHash('sha256').update(bytes).digest('hex');
  if(!absolute.endsWith('.mjs'))return;
  const source=bytes.toString('utf8');
  for(const match of source.matchAll(/(?:\bfrom\s*|\bimport\s*)['"](\.{1,2}\/[^'"]+)['"]/g))await visit(resolve(dirname(absolute),match[1]));
 }
 for(const entry of entries)await visit(resolve(root,entry));
 return Object.fromEntries(Object.entries(hashes).sort(([a],[b])=>a.localeCompare(b)));
}
