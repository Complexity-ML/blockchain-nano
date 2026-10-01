import {readFileSync} from 'node:fs';
import {inspectRegistry} from './regression-inventory.mjs';
const [registryPath,manifestPath]=process.argv.slice(2);
try{
 const registry=JSON.parse(readFileSync(registryPath));
 const manifest=JSON.parse(readFileSync(manifestPath));
 if(!manifest||Array.isArray(manifest)||typeof manifest!=='object'||!Object.keys(manifest).length)throw Error('Empty executable manifest');
 for(const [path,hash] of Object.entries(manifest))if(!path.startsWith('/')||typeof hash!=='string'||!/^[a-f0-9]{64}$/.test(hash))throw Error('Invalid executable manifest entry');
 const report=inspectRegistry(registry,registry.tests.map(t=>t.name),p=>Object.hasOwn(manifest,p));
 console.log(JSON.stringify(report,null,2));
 if(!report.completeSelection||report.missingExecutables.length||report.withoutExplicitTimeout.length)process.exitCode=1;
}catch(e){console.error(e.message);process.exitCode=1;}
