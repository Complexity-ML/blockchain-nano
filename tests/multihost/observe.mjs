#!/usr/bin/env node
// Read-only observation through local/SSH container exec; never expose RPC.
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,mkdir,appendFile,writeFile} from 'node:fs/promises';
import {join} from 'node:path';
import {observeStableFinality} from '../assertoor/stability.mjs';
const exec=promisify(execFile),[configFile,output]=process.argv.slice(2);
if(!configFile||!output)throw Error('Usage: observe.mjs observers.json fresh-output');
const config=JSON.parse(await readFile(configFile,'utf8'));
if(!Array.isArray(config.nodes)||config.nodes.length<2||!/^0x[0-9a-f]{64}$/.test(config.genesisHash)||!/^[0-9a-f]{40}$/.test(config.revision))throw Error('Expected genesis hash, revision and nodes');
const names=new Set(),transports=new Set(),peerIds=new Set();
const quote=s=>"'"+s.replaceAll("'","'\\''")+"'";
const command=async(node,args)=>{
 let binary='curl',argv=args;
 if(node.kind!=='native'){
  binary='docker';argv=['exec',node.container,'curl',...args];
  if(node.kind==='ssh-docker'){
   binary='ssh';argv=['-o','BatchMode=yes','-o','ConnectTimeout=5',node.ssh,['sudo','-n','docker',...argv].map(quote).join(' ')];
  }
 }
 const {stdout}=await exec(binary,argv,{timeout:10000,maxBuffer:8*1024*1024});return stdout;
};
for(const n of config.nodes){
 if(!/^[a-z0-9-]+$/.test(n.name)||names.has(n.name)||!['native','docker','ssh-docker'].includes(n.kind))throw Error('Invalid observer identity');
 names.add(n.name);
 const u=new URL(n.url);
 if(u.protocol!=='http:'||u.hostname!=='127.0.0.1'||u.username||u.password||u.pathname!=='/'||u.search||u.hash)throw Error('RPC must remain loopback');
 if(n.kind!=='native'&&!/^nano-qualification-[a-z0-9-]+$/.test(n.container))throw Error('Unexpected test container name');
 if(n.kind==='ssh-docker'&&!/^[a-z_][a-z0-9_-]*@[a-z0-9.-]+$/.test(n.ssh))throw Error('Invalid SSH target');
 const identity=n.kind==='native'?`native:${u.origin}`:`${n.kind}:${n.ssh??'local'}:${n.container}`;
 if(transports.has(identity))throw Error('Duplicate observer transport');transports.add(identity);

}
await mkdir(output);
const record=async value=>appendFile(join(output,'samples.jsonl'),JSON.stringify(value)+'\n');
const request=async(url,options={})=>{
 const u=new URL(url),node=config.nodes.find(n=>n.name===u.hostname);
 if(!node)throw Error('Unknown observer');
 const path=u.pathname+u.search;
 const args=['-sS','--max-time','5','-w','\n%{http_code}','-H','Accept: application/json'];
 if(options.method==='POST')args.push('-H','Content-Type: application/json','--data',options.body);
 args.push(node.url.replace(/\/$/,'')+path);
 const text=await command(node,args),last=text.lastIndexOf('\n'),status=Number(text.slice(last+1));
 if(!Number.isInteger(status)||status<100||status>599)throw Error('Missing HTTP status');
 return {ok:status>=200&&status<300,status,json:async()=>JSON.parse(text.slice(0,last))};
};
const rpc=async(node,method,params=[])=>{
 const r=await request(`http://${node.name}`,{method:'POST',body:JSON.stringify({jsonrpc:'2.0',id:1,method,params})});
 const body=await r.json();if(!r.ok||body.error)throw Error(`${node.name}: ${method} unavailable`);return body.result;
};
try{
 for(const node of config.nodes){
  if(node.kind!=='native'){
   const args=['inspect','--format','{{json .}}',node.container];
   const bin=node.kind==='ssh-docker'?'ssh':'docker';
   const argv=node.kind==='ssh-docker'?['-o','BatchMode=yes',node.ssh,['sudo','-n','docker',...args].map(quote).join(' ')]:args;
   const {stdout}=await exec(bin,argv,{timeout:10000,maxBuffer:2*1024*1024});const container=JSON.parse(stdout);
   if(container.Config.Labels?.['nano.qualification']!=='multihost'||container.Config.Labels?.['org.opencontainers.image.revision']!==config.revision||!container.State.Running)throw Error('Container provenance mismatch');
  }
  const chain=await rpc(node,'eth_chainId'),genesis=await rpc(node,'eth_getBlockByNumber',['0x0',false]);
  if(BigInt(chain)!==1337n||genesis?.hash!==config.genesisHash)throw Error('Unexpected chain/genesis');
  const info=await rpc(node,'nno_getNetworkInfo'),peerId=info?.node?.peerId;
  if(typeof peerId!=='string'||!peerId||peerIds.has(peerId))throw Error('Missing or duplicate P2P identity');
  peerIds.add(peerId);
  await record({event:'observer-bound',name:node.name,kind:node.kind,chain,genesisHash:genesis.hash,peerId});
 }
 const result=await observeStableFinality(config.nodes.map(n=>`http://${n.name}`),3,record,{request});
 await writeFile(join(output,'result.json'),JSON.stringify({...result,nodes:config.nodes.map(n=>n.name),revision:config.revision},null,2)+'\n');
 console.log(JSON.stringify(result));
}catch(error){await writeFile(join(output,'failure.txt'),error.stack+'\n');throw error;}
