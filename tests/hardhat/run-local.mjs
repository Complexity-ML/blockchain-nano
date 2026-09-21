import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const cwd=path.dirname(fileURLToPath(import.meta.url));
const binary=process.env.NANO_BINARY;
if(!binary||!path.isAbsolute(binary))throw Error('NANO_BINARY must be absolute');
const root=fs.mkdtempSync(path.join(os.tmpdir(),'nano-hardhat-'));
const signer='0x7e5f4552091a69125d5dfcb7b8c2659029395bdf';
const genesis={config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bftVersion:1,bootstrapValidator:signer},alloc:{[signer]:{balance:'0xd3c21bcecceda1000000'},'0x2b5ad5c4795c026514f8317c7a215e218dccd6cf':{balance:'0xd3c21bcecceda1000000'}}};
if(process.env.NANO_TEST_EVM_FORK)genesis.config.evmFork=process.env.NANO_TEST_EVM_FORK;
fs.writeFileSync(path.join(root,'genesis.json'),JSON.stringify(genesis));
fs.writeFileSync(path.join(root,'node.json'),JSON.stringify({bft:{key_env:'TEST_BFT_KEY',journal:path.join(root,'votes')}}));
const core=spawn(binary,['serve',path.join(root,'genesis.json'),path.join(root,'db'),'0','127.0.0.1',path.join(root,'node.json')],{env:{...process.env,TEST_BFT_KEY:'0x'+'1'.padStart(64,'0')},stdio:['ignore','pipe','pipe']});
let output='';core.stderr.on('data',d=>process.stderr.write(d));
let runner;
try {
 const port=await new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Nano startup timeout')),10000);core.once('exit',()=>{clearTimeout(timer);reject(Error('Nano exited during startup'));});core.stdout.on('data',d=>{output+=d;const match=output.match(/listening on 127\.0\.0\.1:(\d+)/);if(match){clearTimeout(timer);resolve(match[1]);}});});
 runner=spawn(process.execPath,['node_modules/hardhat/dist/src/cli.js','run',process.argv[2]??'scripts/publish.ts','--network','nano'],{cwd,env:{...process.env,NANO_TEST_RPC:`http://127.0.0.1:${port}`},stdio:'inherit'});
 const code=await new Promise((resolve,reject)=>{const timer=setTimeout(()=>{runner.kill('SIGKILL');reject(Error('Hardhat test timeout'));},90000);runner.once('error',reject);runner.once('exit',c=>{clearTimeout(timer);resolve(c);});});
 if(code!==0)throw Error(`Hardhat exited ${code}`);
} finally {
 if(runner&&runner.exitCode===null)runner.kill('SIGKILL');
 if(core.exitCode===null){const stopped=new Promise(resolve=>core.once('exit',resolve));core.kill('SIGTERM');const timer=setTimeout(()=>core.kill('SIGKILL'),5000);await stopped;clearTimeout(timer);}
 fs.rmSync(root,{recursive:true,force:true});
}
