// Compile every production Solidity source in the pinned OpenZeppelin release.
// Compilation is an inventory gate, never a claim of behavioral compatibility.
import fs from 'node:fs';
import crypto from 'node:crypto';
import {createRequire} from 'node:module';
const solc=createRequire(import.meta.url)('../.deps/solc/node_modules/solc');
if(!solc.version().startsWith('0.8.28+')) throw new Error('Expected solc 0.8.28');
const root='tests/contracts/openzeppelin-v5.0.2/contracts';
const sources={};
function walk(dir){for(const e of fs.readdirSync(dir,{withFileTypes:true}).sort((a,b)=>a.name.localeCompare(b.name))){const p=`${dir}/${e.name}`;if(e.isDirectory())walk(p);else if(p.endsWith('.sol'))sources[p]={content:fs.readFileSync(p,'utf8')};}}
walk(root);
const settings=JSON.parse(fs.readFileSync('contracts/NanoStaking.json','utf8')).settings;
settings.outputSelection['*']['']=['ast'];
const output=JSON.parse(solc.compile(JSON.stringify({language:'Solidity',sources,settings})));
const errors=(output.errors||[]).filter(e=>e.severity==='error');
if(errors.length)throw new Error(errors.map(e=>e.formattedMessage).join('\n'));
const components=[];
for(const [source,contracts] of Object.entries(output.contracts))for(const [name,c] of Object.entries(contracts)) components.push({source:source.slice(root.length+1),name,creationBytes:c.evm.bytecode.object.length/2,runtimeBytes:c.evm.deployedBytecode.object.length/2,behavioralCoverage:'pending'});
const report={upstream:'https://github.com/OpenZeppelin/openzeppelin-contracts/tree/v5.0.2',compiler:solc.version(),evmVersion:settings.evmVersion,settings,sourceCount:Object.keys(sources).length,sourceSha256:Object.fromEntries(Object.entries(sources).map(([p,s])=>[p.slice(root.length+1),crypto.createHash('sha256').update(s.content).digest('hex')])),components};
fs.writeFileSync('tests/fixtures/openzeppelin-catalog.json',JSON.stringify(report,null,2)+'\n');
console.log(`${report.sourceCount} sources, ${components.length} components compiled; behavioral coverage tracked separately.`);

const deployments=[];
for(const [source,contracts] of Object.entries(output.contracts))for(const [name,c] of Object.entries(contracts))if(output.sources[source].ast.nodes.some(n=>n.nodeType==='ContractDefinition'&&n.name===name&&n.contractKind==='contract'&&!n.abstract))deployments.push({source:source.slice(root.length+1),name,bytecode:'0x'+c.evm.bytecode.object,abi:c.abi,methodIdentifiers:c.evm.methodIdentifiers});
fs.writeFileSync('tests/fixtures/openzeppelin-deployments.json',JSON.stringify({compiler:solc.version(),settings,contracts:deployments},null,2)+'\n');
console.log(`${deployments.length} concrete contracts exported for signed deployment tests.`);
