#!/usr/bin/env node
// Repartition PUBLIC exporter fixtures only. Never use a production genesis.
import {readFile,writeFile,mkdir,copyFile} from 'node:fs/promises';
import {join,resolve} from 'node:path';
const [input,output,topologyFile]=process.argv.slice(2);
if(!input||!output||!topologyFile)throw Error('Usage: prepare.mjs exported-fixtures new-output topology.json');
const read=async p=>JSON.parse(await readFile(p,'utf8'));
const topology=await read(topologyFile), genesis=await read(join(input,'genesis.json'));
const nodes=await Promise.all([0,1].map(i=>read(join(input,`node-${i}.json`))));
const envs=await Promise.all([0,1].map(i=>read(join(input,`env-${i}.json`))));
const fork=nodes.every(n=>n.capella&&!n.bellatrix)?'capella':nodes.every(n=>n.bellatrix&&!n.capella)?'bellatrix':null;
if(!fork)throw Error('Mixed or unsupported consensus fixtures');
const snapshot=fork==='capella'?'initial.json':'snapshot.json';
const keys=nodes.flatMap(n=>n[fork].validator.keys), environment=Object.assign({},...envs);
// Exporter uses chain 1337 and scalar index+1 for every PUBLIC synthetic key.
const chain=genesis.config?.chainId;
if(BigInt(chain)!==1337n||keys.length!==64)throw Error('Not the expected isolated exporter fixture');
const indices=new Set(),names=new Set();
for(const k of keys){
 const index=BigInt(k.index),name=fork==='capella'?`NANO_TEST_CAPELLA_NETWORK_KEY_${index}`:`NANO_KURTOSIS_FIXTURE_${index}`;
 if(index<0n||index>=64n||indices.has(String(index))||k.keyEnv!==name||BigInt(environment[name])!==index+1n)throw Error('Unexpected or duplicate fixture key');
 indices.add(String(index));
}
if(!Array.isArray(topology)||topology.length<2||topology.length>keys.length)throw Error('Each test instance needs a nonempty key partition');
for(const t of topology){
 if(typeof t.host!=='string'||!t.host||!/^[a-z0-9-]+$/.test(t.name)||names.has(t.name)||!['native','docker'].includes(t.mode))throw Error('Invalid topology identity');
 names.add(t.name);
 if(!Number.isInteger(t.rpcPort)||t.rpcPort<1024||t.rpcPort>65535||!Number.isInteger(t.p2pPort)||t.p2pPort<1024||t.p2pPort>65535)throw Error('Invalid port');
 if(t.mode==='native'&&(!t.p2pBinary?.startsWith('/')||!t.runtimeDirectory?.startsWith('/')))throw Error('Native absolute paths required');
}
await mkdir(output); // Refuse to overwrite an earlier test or its journals.
const manifest={fork,physicalHosts:new Set(topology.map(t=>t.host)).size,syntheticValidators:keys.length,networkQualified:false,nodes:[]};
for(let i=0;i<topology.length;i++){
 const t=topology[i],directory=join(output,t.name);await mkdir(directory);
 const selected=keys.filter((_,index)=>index%topology.length===i);
 const node=structuredClone(nodes[0]),config=t.mode==='docker'?'/config':t.runtimeDirectory;
 const data=t.mode==='docker'?'/var/lib/nano':join(t.runtimeDirectory,'data');
 node.rpc_host='127.0.0.1';node.rpc_port=t.rpcPort;
 node.listen_addr=`/ip4/0.0.0.0/tcp/${t.p2pPort}`;
 node.p2p_binary=t.mode==='docker'?'/usr/local/bin/nano-p2p':t.p2pBinary;
 node.bootnodes=[]; // Filled from the actual remote peer ID at startup.
 node.p2p_identity=join(data,'peer.key');
 node[fork].profile.file=join(config,'profile.json');node[fork].initial.file=join(config,snapshot);
 node[fork].recoveryWitness=join(data,'core-witness');
 node[fork].validator.keys=selected.map(k=>({...k,journal:join(data,`key-${BigInt(k.index)}`),recoveryWitness:join(data,`key-${BigInt(k.index)}-witness`)}));
 for(const name of ['genesis.json','profile.json',snapshot])await copyFile(join(input,name),join(directory,name));
 await writeFile(join(directory,'node.json'),JSON.stringify(node,null,2)+'\n');
 await writeFile(join(directory,'fixture.env'),selected.map(k=>`${k.keyEnv}=${environment[k.keyEnv]}`).join('\n')+'\n',{mode:0o600});
 manifest.nodes.push({name:t.name,mode:t.mode,indices:selected.map(k=>String(BigInt(k.index))),configuration:resolve(directory)});
}
await writeFile(join(output,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify(manifest));
