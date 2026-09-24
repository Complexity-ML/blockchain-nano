import {createRequire} from 'node:module';import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';import {resolve} from 'node:path';import {spawnSync,execFileSync} from 'node:child_process';import assert from 'node:assert/strict';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));const {Wallet,encodeRlp,decodeRlp,keccak256}=require('ethers');
const [geth,nano,templateFile,output]=process.argv.slice(2);assert(geth&&nano&&templateFile&&output,'usage: geth nano environment-template output');
const template=JSON.parse(readFileSync(templateFile)),wallet=new Wallet('0x'+'1'.padStart(64,'0'));const rows=[];mkdirSync(output,{recursive:true});const write=(file,value)=>writeFileSync(file,JSON.stringify(value,null,2)+'\n');
for(const fork of ['Paris','Shanghai','Cancun','Prague','Osaka'])for(const type of [0,1,2])for(const length of [31,32,33,49151,49152,49153]){
 const name=fork+'-type'+type+'-'+length,dir=resolve(output,name);mkdirSync(dir,{recursive:true});
 const data='0x60006000f3'+'00'.repeat(length-5),fees=type===2?{maxFeePerGas:3000000000,maxPriorityFeePerGas:1000000000}:{gasPrice:3000000000};
 const raw=await wallet.signTransaction({chainId:1337,type,nonce:0,gasLimit:3000000,data,...fees});
 const input={...template,fork,pre:{[wallet.address.toLowerCase()]:{balance:'0xd3c21bcecceda1000000',nonce:'0x0',code:'0x',storage:{}}},rawTransaction:raw};write(resolve(dir,'nano-input.json'),input);
 write(resolve(dir,'txs.rlp'),encodeRlp([type===0?decodeRlp(raw):raw]));
 const args=['t9n','--state.fork='+fork,'--state.chainid=1337','--input.txs='+resolve(dir,'txs.rlp')];
 const g=spawnSync(geth,args,{encoding:'utf8',timeout:15000}),n=spawnSync(nano,[],{input:JSON.stringify(input),encoding:'utf8',timeout:15000});
 write(resolve(dir,'geth-process.json'),{args,status:g.status,stdout:g.stdout,stderr:g.stderr,error:g.error?.message});write(resolve(dir,'nano-process.json'),{status:n.status,stdout:n.stdout,stderr:n.stderr,error:n.error?.message});
 let error;const expectedRejected=fork!=='Paris'&&length>49152;let reference;
 try{
 assert.ifError(g.error);assert.ifError(n.error);assert.equal(g.status,0,g.stderr);const refs=JSON.parse(g.stdout);assert.equal(refs.length,1);reference=refs[0];
 if(expectedRejected){assert.match(reference.error??'',/max initcode size exceeded/);assert.equal(n.status,1);assert.match(n.stderr,/initcode size exceeds limit/);}
 else{assert.equal(reference.error,undefined);assert.equal(n.status,0,n.stderr);const actual=JSON.parse(n.stdout);assert.equal(reference.hash,keccak256(raw));assert.equal(actual.receipt.transactionHash,keccak256(raw));assert.equal(actual.receipt.status,'0x1');assert.equal(BigInt(actual.intrinsicGas),BigInt(reference.intrinsicGas));}
 }catch(e){error=e.message;}
 const row={case:name,fork,type,initcodeBytes:length,expectedRejected,status:error?'mismatch':'matched',referenceError:reference?.error??null,...(error?{error}:{})};rows.push(row);console.log(JSON.stringify(row));
}
const mismatched=rows.filter(r=>r.status!=='matched').length,summary={matched:rows.length-mismatched,mismatched,rejected:rows.filter(r=>r.expectedRejected).length,version:execFileSync(geth,['--version'],{encoding:'utf8'}).trim(),scope:'Same signed envelopes: Geth t9n validity versus Nano offline execution admission; valid intrinsic gas and success; not a mempool or full block validation'};write(resolve(output,'summary.json'),{summary,results:rows});console.log(JSON.stringify({summary}));process.exitCode=mismatched?1:0;
