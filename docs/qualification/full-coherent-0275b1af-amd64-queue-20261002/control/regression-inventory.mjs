import {readFileSync,existsSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
import {fileURLToPath} from 'node:url';

const skipProperties=new Set(['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION']);
export function inspectRegistry(registry,selected,executableExists=existsSync){
 if(!Array.isArray(registry?.tests)||registry.tests.length===0)throw new Error('Empty CTest registry');
 const names=new Set();const labels={};const tests=[];
 for(const test of registry.tests){
  if(typeof test.name!=='string'||!test.name||names.has(test.name))throw new Error('Invalid or duplicate CTest name');
  names.add(test.name);
  if(!Array.isArray(test.command)||!test.command.length||test.command.some(x=>typeof x!=='string'))throw new Error('Missing CTest command: '+test.name);
  if(!Array.isArray(test.properties))throw new Error('Missing CTest properties: '+test.name);
  const properties={};
  for(const property of test.properties){
   if(typeof property.name!=='string'||Object.hasOwn(properties,property.name))throw new Error('Invalid or duplicate property: '+test.name);
   if(skipProperties.has(property.name))throw new Error('CTest skip property forbidden: '+test.name+'/'+property.name);
   properties[property.name]=property.value;
  }
  const timeout=properties.TIMEOUT??null;
  if(timeout!==null&&(!Number.isFinite(timeout)||timeout<=0))throw new Error('Invalid CTest timeout: '+test.name);
  const testLabels=properties.LABELS??[];
  if(!Array.isArray(testLabels)||testLabels.some(x=>typeof x!=='string'))throw new Error('Invalid CTest labels: '+test.name);
  for(const label of testLabels)labels[label]=(labels[label]??0)+1;
  tests.push({name:test.name,command:test.command,timeoutSeconds:timeout,labels:testLabels,executablePresent:executableExists(test.command[0])});
 }
 if(!Array.isArray(selected)||new Set(selected).size!==selected.length||selected.some(name=>!names.has(name)))throw new Error('Selection contains duplicate or foreign tests');
 const selection=new Set(selected);
 tests.sort((a,b)=>a.name.localeCompare(b.name,'en'));
 return {schemaVersion:1,qualification:'inventory-only',runtimeTestsExecuted:0,totalTests:tests.length,selectedTests:selected.length,completeSelection:selected.length===tests.length,unselectedTests:tests.filter(t=>!selection.has(t.name)).map(t=>t.name),missingExecutables:tests.filter(t=>!t.executablePresent).map(t=>t.name),withoutExplicitTimeout:tests.filter(t=>t.timeoutSeconds===null).map(t=>t.name),sumExplicitTimeoutSeconds:tests.reduce((n,t)=>n+(t.timeoutSeconds??0),0),uniqueExecutables:new Set(tests.map(t=>t.command[0])).size,labels,tests};
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 try{
  const [registryPath,selectionPath,option]=process.argv.slice(2);
  if(!registryPath||!selectionPath||(option&&option!=='--require-complete'))throw new Error('Usage: node regression-inventory.mjs REGISTRY.json SELECTION.json [--require-complete]');
  const registryBytes=readFileSync(registryPath);const selectionBytes=readFileSync(selectionPath);
  const selection=JSON.parse(selectionBytes);
  if(!Array.isArray(selection?.tests))throw new Error('Missing selection tests');
  const report=inspectRegistry(JSON.parse(registryBytes),selection.tests.map(t=>t.name));
  report.inputs={registrySha256:createHash('sha256').update(registryBytes).digest('hex'),selectionSha256:createHash('sha256').update(selectionBytes).digest('hex')};
  console.log(JSON.stringify(report,null,2));
  if(option==='--require-complete'&&(!report.completeSelection||report.missingExecutables.length||report.withoutExplicitTimeout.length))process.exitCode=1;
 }catch(error){console.error(error.message);process.exitCode=1;}
}
