import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
const root=resolve(process.argv[2]??'.');
const files={
 'LICENSE.ethereumjs':'third_party/opcodes/LICENSE.ethereumjs',
 'LICENSE.evm-codes':'third_party/opcodes/LICENSE.evm-codes',
 'bip39-LICENSE':'third_party/bip39/LICENSE',
 'blst-LICENSE':'third_party/ckzg/blst/LICENSE',
 'ckzg-LICENSE':'third_party/ckzg/LICENSE',
 'json-COPYRIGHTS':'third_party/notices/json-COPYRIGHTS',
 'json-LICENSE.MIT':'third_party/notices/json-LICENSE.MIT',
 'libp2p-LICENSE-APACHE':'third_party/notices/libp2p-LICENSE-APACHE',
 'libp2p-LICENSE-MIT':'third_party/notices/libp2p-LICENSE-MIT',
 'mcl-COPYRIGHT':'third_party/mcl/COPYRIGHT',
 'secp256k1-LICENSE':'third_party/secp256k1/COPYING',
};
try{
 const records=new Map();
 for(const line of readFileSync(resolve(root,'deploy/docker/notices.sha256'),'utf8').trimEnd().split('\n')){
  const match=/^([0-9a-f]{64})  ([A-Za-z0-9._-]+)$/.exec(line);
  if(!match||records.has(match[2])||!Object.hasOwn(files,match[2]))throw Error('Invalid or duplicate notice record');
  records.set(match[2],match[1]);
 }
 if(records.size!==Object.keys(files).length)throw Error('Incomplete notice inventory');
 for(const [name,path] of Object.entries(files)){
  const hash=createHash('sha256').update(readFileSync(resolve(root,path))).digest('hex');
  if(hash!==records.get(name))throw Error('Notice hash mismatch: '+name);
 }
 const header=readFileSync(resolve(root,'third_party/json.hpp'),'utf8');
 const credits=[...new Set(header.split('\n').filter(line=>line.startsWith('// SPDX-FileCopyrightText: ')).map(line=>line.slice(3)))].sort();
 const notice=readFileSync(resolve(root,files['json-COPYRIGHTS']),'utf8');
 const shipped=notice.split('\n').filter(line=>line.startsWith('SPDX-FileCopyrightText: ')).sort();
 if(JSON.stringify(credits)!==JSON.stringify(shipped)||credits.length===0)throw Error('JSON copyrights differ from the bundled source');
 console.log(JSON.stringify({notices:records.size,jsonCopyrights:credits.length,verified:true}));
}catch(error){console.error(error.message);process.exitCode=1;}
