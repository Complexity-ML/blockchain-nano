import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,readFile,symlink,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile),repo=process.cwd();
const base='sha256:'+'a'.repeat(64),builder='sha256:'+'b'.repeat(64),revision='c'.repeat(40),hash='d'.repeat(64);
async function exercise({subdir,networkSubdir,mode='normal',override}={}){
 const dir=await mkdtemp(join(tmpdir(),'nano-package-path-'));
 try{
  const bin=join(dir,'bin'),assets=join(dir,'assets'),log=join(dir,'docker.jsonl'),archive=join(dir,'artifacts.tar');
  await mkdir(bin);await mkdir(join(assets,'network'),{recursive:true});
  if(mode==='symlink')await symlink('production-source-revision.txt',join(assets,'nano'));else await writeFile(join(assets,'nano'),'nano-fixture');
  await writeFile(join(assets,'network','nano-p2p'),'p2p-fixture');
  await writeFile(join(assets,'production-source-revision.txt'),mode==='wrong-revision'?'e'.repeat(40):revision);
  await exec('tar',['-C',assets,'-cf',archive,'nano','network/nano-p2p','production-source-revision.txt']);
  await writeFile(join(bin,'docker'),'#!'+process.execPath+'\n'+`const fs=require('node:fs'),p=require('node:path'),a=process.argv.slice(2);fs.appendFileSync(process.env.TEST_LOG,JSON.stringify(a)+'\\n');if(a[0]==='image'&&a[1]==='inspect'){console.log(a.at(-1).includes('.Architecture')?(process.env.TEST_MODE==='wrong-arch'&&a[2]==='${builder}'?'amd64':'arm64'):'${base}');}else if(a[0]==='run'){if(process.env.TEST_MODE==='export-failure')process.exitCode=17;else process.stdout.write(fs.readFileSync(process.env.TEST_ARCHIVE));}else if(a[0]==='build'){const context=a.at(-1);if(fs.readFileSync(p.join(context,'nano'),'utf8')!=='nano-fixture'||fs.readFileSync(p.join(context,'nano-p2p'),'utf8')!=='p2p-fixture')throw Error('wrong artifacts');fs.writeFileSync(a[a.indexOf('--iidfile')+1],'sha256:'+'f'.repeat(64));}`,{mode:0o700});
  const args=[base,builder,'build-volume',revision,hash,hash];if(subdir!==undefined)args.push(subdir);if(networkSubdir!==undefined)args.push(networkSubdir);if(override)args[override.index]=override.value;
  let code=0,stderr='';try{await exec('sh',[join(repo,'scripts/operations/package-candidate.sh'),...args],{cwd:repo,env:{...process.env,PATH:bin+':'+process.env.PATH,TEST_LOG:log,TEST_ARCHIVE:archive,TEST_MODE:mode},timeout:30000});}catch(e){code=e.code;stderr=e.stderr;}
  let calls=[];try{calls=(await readFile(log,'utf8')).trim().split('\n').filter(Boolean).map(l=>JSON.parse(l));}catch(e){if(e.code!=='ENOENT')throw e;}
  return {code,stderr,calls};
 }finally{await rm(dir,{recursive:true,force:true});}
}
test('isolated build is exported without copying it over root build',async()=>{const r=await exercise({subdir:'production-evm-c10426b2-20261001'});assert.equal(r.code,0,r.stderr);const run=r.calls.find(a=>a[0]==='run');assert.equal(run.at(-2),'/build/production-evm-c10426b2-20261001');assert.equal(run.at(-1),'/build/network');assert.equal(r.calls.filter(a=>a[0]==='build').length,1);});
test('six-argument legacy export remains supported',async()=>{const r=await exercise();assert.equal(r.code,0,r.stderr);assert.equal(r.calls.find(a=>a[0]==='run').at(-2),'/build');assert.equal(r.calls.find(a=>a[0]==='run').at(-1),'/build/network');});
test('unsafe subdirectory rejected before Docker is invoked',async()=>{for(const subdir of ['../other','/absolute','a/b','a\\b','-option','.','', 'valid\n../other']){const r=await exercise({subdir});assert.notEqual(r.code,0);assert.equal(r.calls.length,0);}});
test('multiline revision hash or volume cannot pass single-line regex validation',async()=>{for(const override of [{index:2,value:'valid\nother'},{index:3,value:revision+'\nextra'},{index:4,value:hash+'\nextra'}]){const r=await exercise({override});assert.notEqual(r.code,0);assert.equal(r.calls.length,0);}});
test('wrong revision marker aborts before image assembly',async()=>{const r=await exercise({mode:'wrong-revision'});assert.notEqual(r.code,0);assert.match(r.stderr,/revision mismatch/);assert.ok(!r.calls.some(a=>a[0]==='build'));});
test('export failure is retained before image assembly',async()=>{const r=await exercise({mode:'export-failure'});assert.equal(r.code,17);assert.ok(!r.calls.some(a=>a[0]==='build'));});
test('nonregular exported artifact is rejected before image assembly',async()=>{const r=await exercise({mode:'symlink'});assert.notEqual(r.code,0);assert.match(r.stderr,/regular build artifacts/);assert.ok(!r.calls.some(a=>a[0]==='build'));});
test('architecture mismatch is rejected before artifact export',async()=>{const r=await exercise({mode:'wrong-arch'});assert.notEqual(r.code,0);assert.ok(!r.calls.some(a=>a[0]==='run'||a[0]==='build'));});

test('coherent core and P2P directories are explicitly selected together',async()=>{const r=await exercise({subdir:'coherent-efbeb9bc-arm64',networkSubdir:'coherent-efbeb9bc-arm64'});assert.equal(r.code,0,r.stderr);const run=r.calls.find(a=>a[0]==='run');assert.equal(run.at(-2),'/build/coherent-efbeb9bc-arm64');assert.equal(run.at(-1),'/build/coherent-efbeb9bc-arm64/network');assert.match(run[run.indexOf('-c')+1],/\$2\/nano-p2p/);assert.doesNotMatch(run[run.indexOf('-c')+1],/\/build\/network\/nano-p2p/);});
test('unsafe network build directory is rejected before Docker calls',async()=>{for(const networkSubdir of ['../other','/absolute','a/b','a\\b','-option','.','', 'valid\nother']){const r=await exercise({subdir:'core',networkSubdir});assert.notEqual(r.code,0);assert.equal(r.calls.length,0);}});
