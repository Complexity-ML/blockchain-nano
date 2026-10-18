// SOURCE_ONLY draft. Root runs these synthetic FS/clock tests; no campaign data.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile,lstat,realpath} from 'node:fs/promises';
import {isAbsolute,resolve} from 'node:path';
import {constants} from 'node:fs';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
const fields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
const sha=b=>createHash('sha256').update(b).digest('hex');
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const need=(v,m)=>{if(!v)throw Error(m);};
function between(source,start,end){
 const at=source.indexOf(start);assert(at>=0&&source.indexOf(start,at+1)===-1,'one reviewed source start');
 const stop=source.indexOf(end,at);assert(stop>at,'reviewed source end');return source.slice(at,stop);
}
async function fixture({samples,readError,existing=false,advanceRead=()=>0}={}){
 const path='/synthetic-owner/new.json',parent='/synthetic-owner',bytes=Buffer.from('{"public":true}');
 const creation=['1','2','33152','501','20','1',String(bytes.length),'100','200'],events=[];
 let clock=0,reads=0,created=existing,written=existing?Buffer.from(bytes):null,current=null,override=null;
 const stable=ctime=>({before:[...creation.slice(0,8),String(ctime)],after:[...creation.slice(0,8),String(ctime)],linked:[...creation.slice(0,8),String(ctime)],data:Buffer.from(bytes)});
 const stat=m=>({...Object.fromEntries(fields.map((k,i)=>[k,BigInt(m[i])])),isFile:()=>((BigInt(m[2])&0o170000n)===0o100000n),isDirectory:()=>((BigInt(m[2])&0o170000n)===0o040000n)});
 const directory=stat(['1','9','16832','501','20','2','0','1','1']);
 const open=async(file,flags,mode)=>{
  if(file===parent){events.push('parent-open');return {async sync(){events.push('parent-sync');},async close(){events.push('parent-close');}};}
  assert.equal(file,path);assert.equal(typeof flags,'number');
  if(flags&constants.O_CREAT){
   assert(flags&constants.O_EXCL);assert(flags&constants.O_NOFOLLOW);assert.equal(mode,0o600);
   if(created)throw Object.assign(Error('exists'),{code:'EEXIST'});created=true;events.push('create');
   return {async writeFile(value){written=Buffer.from(value);events.push('write');},async sync(){events.push('file-sync');},async stat(){events.push('creator-stat');return stat(creation);},async close(){events.push('file-close');}};
  }
  assert(flags&constants.O_NOFOLLOW);assert(flags&constants.O_NONBLOCK);
  if(readError)throw readError;
  const index=reads++;clock+=advanceRead(index);current=structuredClone(override??(samples?samples(index,stable):stable(201)));events.push('sample');
  let stats=0,offset=0;const data=Buffer.from(current.data??bytes);
  return {async stat(){return stat(stats++===0?current.before:current.after);},async readFile(){return Buffer.from(data);},async read(buffer,start,length){const n=Math.max(0,Math.min(length,data.length-offset));data.copy(buffer,start,offset,offset+n);offset+=n;return {bytesRead:n};},async close(){events.push('read-close');}};
 };
 const lstat=async p=>p===parent?directory:stat(current?.linked??creation);
 const source=await readFile(new URL('./lifecycle-runtime.mjs',import.meta.url),'utf8');
 const first=Math.min(source.indexOf('export async function readLifecycleKnownFile('),source.indexOf('const knownLifecycleCreations='));
 assert(first>=0);const end=source.indexOf('\nexport async function lifecyclePrivateDirectory',first);assert(end>first);
 const code=source.slice(first,end).replaceAll('export ','');
 const deps={open,lstat,realpath:async p=>p,resolve:p=>p,dirname:p=>p.slice(0,p.lastIndexOf('/')),constants,createHash,Buffer,fields,material:s=>fields.map(k=>String(s[k])),sha,shaPattern:/^[0-9a-f]{64}$/,canonical,copy:structuredClone,process:{getuid:()=>501},need,performance:{now:()=>clock},sleep:async ms=>{assert.equal(ms,1000);events.push('wait');clock+=ms;}};
 const api=Function(...Object.keys(deps),code+'\nreturn {writeLifecycleKnownFile,admitLifecycleKnownFiles,readLifecycleKnownFile,knownLifecycleCreations};')(...Object.values(deps));
 return {api,path,bytes,creation,events,stable,pin:{path,sha256:sha(bytes),bytes:bytes.length},setSample:x=>{override=x;},get reads(){return reads;},get clock(){return clock;},get written(){return written;}};
}
async function written(options){const f=await fixture(options);assert.deepEqual(await f.api.writeLifecycleKnownFile(f.path,f.bytes),f.pin);assert.deepEqual(f.written,f.bytes);assert.equal(f.reads,0,'writer must not perform an eager full9 read before first admission');return f;}

test('known NEW writer durably records creation before sampling; transient intra-FD ctime does not count',async()=>{
 const f=await written({samples:(i,stable)=>{const x=stable(i===0?200:201);if(i===0){x.after[8]='201';x.linked[8]='201';}return x;}});
 const a=await f.api.admitLifecycleKnownFiles([f.pin]);assert.equal(f.reads,5);assert.equal(f.clock,3000);assert.equal(a.probeCount,4);assert(a.stableSpanMs>=2000);
 assert(f.events.indexOf('file-sync')<f.events.indexOf('creator-stat'));assert(f.events.indexOf('parent-sync')<f.events.indexOf('sample'));
 await a.verify();assert.equal(f.reads,6);
});
test('NEW ctime change between stable reads resets the full three-sample two-second run',async()=>{
 const f=await written({samples:(i,stable)=>stable(i<2?200:201)});await f.api.admitLifecycleKnownFiles([f.pin]);assert.equal(f.reads,6);assert.equal(f.clock,4000);
 const clean=await written();await clean.api.admitLifecycleKnownFiles([clean.pin]);assert.equal(clean.reads,4);assert.equal(clean.clock,2000);
});
test('known bytes and each of eight creation fields refuse immediately, even during ctime churn',async()=>{
 for(const changed of ['sha256','bytes',...fields.slice(0,8)]){
  const f=await written({samples:(_i,stable)=>{const x=stable(201);x.after[8]='202';x.linked[8]='202';if(changed==='sha256')x.data=Buffer.from('X'+x.data.toString().slice(1));else if(changed==='bytes')x.data=Buffer.concat([x.data,Buffer.from('x')]);else{const at=fields.indexOf(changed);for(const row of [x.before,x.after,x.linked])row[at]=String(BigInt(row[at])+1n);}return x;}});
  await assert.rejects(f.api.admitLifecycleKnownFiles([f.pin]));assert.equal(f.reads,1,changed);assert.equal(f.clock,0,changed);
  const reads=f.reads;await assert.rejects(f.api.admitLifecycleKnownFiles([f.pin]));assert.equal(f.reads,reads,'failed NEW creation cannot be re-admitted');
 }
});
test('continuous NEW ctime churn is bounded; unrelated reader error remains the first error',async()=>{
 const f=await written({samples:(i,stable)=>{const x=stable(200+i);x.after[8]=String(201+i);x.linked[8]=String(201+i);return x;}});
 await assert.rejects(f.api.admitLifecycleKnownFiles([f.pin]),/deadline|stable/i);assert(f.clock<=30000);assert(f.reads<=30);
 const error=Error('injected nofollow read failure'),failed=await written({readError:error});await assert.rejects(failed.api.admitLifecycleKnownFiles([failed.pin]),e=>e===error);assert.equal(failed.clock,0);
});
test('adopted creator rows and strict retained reads never reset ctime or another old field',async()=>{
 const f=await written();const admitted=await f.api.admitLifecycleKnownFiles([f.pin]);const row=structuredClone(f.api.knownLifecycleCreations.get(f.path));f.setSample(f.stable(202));
 await assert.rejects(admitted.verify());await assert.rejects(f.api.admitLifecycleKnownFiles([f.pin]));assert.deepEqual(f.api.knownLifecycleCreations.get(f.path),row);
 const old=await fixture({existing:true});old.setSample(old.stable(201));await assert.rejects(old.api.readLifecycleKnownFile(old.path,old.pin,{expectedMaterial:old.creation}));await assert.rejects(old.api.writeLifecycleKnownFile(old.path,old.bytes),e=>e.code==='EEXIST');assert.equal(old.api.knownLifecycleCreations.size,0);
});
test('NEW intra-read mtime or path inode mutation cannot masquerade as ctime stabilization',async()=>{
 for(const [at,where]of [[7,'after'],[1,'linked']]){
  const f=await written({samples:(_i,stable)=>{const x=stable(201);x[where][at]=String(BigInt(x[where][at])+1n);return x;}});
  await assert.rejects(f.api.admitLifecycleKnownFiles([f.pin]));assert.equal(f.reads,1);assert.equal(f.clock,0);
 }
});
test('caller-supplied old material remains strict and the immediate post-adoption fence is inside the deadline',async()=>{
 const old=await fixture({existing:true});old.setSample(old.stable(201));
 await assert.rejects(old.api.admitLifecycleKnownFiles([{...old.pin,material:old.creation}]));assert.equal(old.reads,1);assert.equal(old.clock,0);
 const late=await written({advanceRead:i=>i===3?30000:0});
 await assert.rejects(late.api.admitLifecycleKnownFiles([late.pin]),/deadline|clock/i);assert.equal(late.reads,4);assert.equal(late.clock,32000);
});

// Root executes only this extracted Python helper slice, with all FS/time
// operations mocked. No owner program entrypoint, files, SSH, DB or network.
const pythonHarness=String.raw`
import sys,json,hashlib,stat,types
source=json.loads(sys.stdin.read())['source']
fields=('st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns')
class P(str):
 def is_absolute(self):return self.startswith('/')
 def resolve(self):return self
 def is_relative_to(self,other):return self==other or self.startswith(str(other)+'/')
 @property
 def parent(self):return P(self.rsplit('/',1)[0])
class FS:
 def __init__(self,case='clean'):
  self.case=case;self.clock=0.;self.reads=0;self.events=[];self.data=b'{"public":true}';self.created=False;self.fds={};self.seq=10;self.force=None;self.written=None
  self.creation=['1','2','33152','501','20','1',str(len(self.data)),'100','200']
 def row(self,m):return types.SimpleNamespace(**dict(zip(fields,map(int,m))))
 def stable(self,c):return {'before':self.creation[:8]+[str(c)],'after':self.creation[:8]+[str(c)],'linked':self.creation[:8]+[str(c)],'data':self.data}
 def sample(self):
  i=self.reads;self.reads+=1;x=self.stable(201)
  if self.force is not None:return self.force
  if self.case in ('intra','churn'):
   x=self.stable(200+i if self.case=='churn'else(200 if i==0 else 201))
   if self.case=='churn'or i==0:x['after'][8]=str(201+i);x['linked'][8]=str(201+i)
  elif self.case=='between':x=self.stable(200 if i<2 else 201)
  elif self.case=='bytes':x['data']=b'X'+self.data[1:]
  elif self.case.startswith('field-'):
   at=int(self.case[6:])
   for k in ('before','after','linked'):x[k][at]=str(int(x[k][at])+1)
  return x
 def open(self,p,flags,mode=0):
  p=str(p);self.seq+=1;fd=self.seq
  if p=='/synthetic-owner':self.fds[fd]={'kind':'directory'};return fd
  assert p=='/synthetic-owner/new.json'
  if flags&64:
   assert flags&128 and not self.created;self.created=True;self.events.append('create');self.fds[fd]={'kind':'write','data':bytearray()}
  else:self.events.append('sample');self.fds[fd]={'kind':'read','sample':self.sample(),'stats':0,'offset':0}
  return fd
 def fstat(self,fd):
  f=self.fds[fd]
  if f['kind']=='directory':return self.row(['1','9','16832','501','20','2','0','1','1'])
  if f['kind']=='write':self.events.append('creator-stat');return self.row(self.creation)
  key='before'if f['stats']==0 else'after';f['stats']+=1;return self.row(f['sample'][key])
 def lstat(self,p):
  if str(p)=='/synthetic-owner':return self.row(['1','9','16832','501','20','2','0','1','1'])
  read=next((f for f in reversed(list(self.fds.values()))if f['kind']=='read'),None)
  return self.row(self.creation if read is None else read['sample']['linked'])
 def read(self,fd,n):
  f=self.fds[fd];b=f['sample']['data'][f['offset']:f['offset']+n];f['offset']+=len(b);return b
 def write(self,fd,b):self.fds[fd]['data'].extend(b);self.events.append('write');return len(b)
 def fsync(self,fd):self.events.append('parent-sync'if self.fds[fd]['kind']=='directory'else'file-sync')
 def close(self,fd):
  f=self.fds.pop(fd)
  if f['kind']=='write':self.written=bytes(f['data'])
  self.events.append('close')
 def sleep(self,n):assert n==1;self.clock+=n
def env(case='clean'):
 fs=FS(case);os=types.SimpleNamespace(open=fs.open,read=fs.read,write=fs.write,fstat=fs.fstat,lstat=fs.lstat,fsync=fs.fsync,close=fs.close,geteuid=lambda:501,O_RDONLY=0,O_RDWR=2,O_CREAT=64,O_EXCL=128,O_NOFOLLOW=131072,O_NONBLOCK=2048,O_DIRECTORY=65536)
 ns={'os':os,'sys':sys,'json':json,'hashlib':hashlib,'stat':stat,'pathlib':types.SimpleNamespace(Path=lambda p:P(p)),'root':P('/synthetic-owner'),'fields':fields,'time':types.SimpleNamespace(monotonic=lambda:fs.clock,sleep=fs.sleep),'new_files':{}}
 exec(compile(source,'reviewed-owner-helpers','exec'),ns);return fs,ns
def reject(f):
 try:f()
 except Exception:return
 raise AssertionError('expected strict refusal')
def written(case='clean'):
 fs,n=env(case);p=P('/synthetic-owner/new.json');pin=n['write'](p,fs.data);assert fs.reads==0 and fs.written==fs.data;return fs,n,p,pin
checks=0
for case,reads,clock in [('intra',5,3),('between',6,4),('clean',4,2)]:
 fs,n,p,pin=written(case);rows=n['stable']([pin]);assert fs.reads==reads and fs.clock==clock
 assert fs.events.index('file-sync')<fs.events.index('creator-stat') and fs.events.index('parent-sync')<fs.events.index('sample')
 assert len(rows)==1 and rows[0]['material'][8]=='201';checks+=1
for case in ['bytes']+['field-'+str(i)for i in range(8)]:
 fs,n,p,pin=written(case);reject(lambda:n['stable']([pin]));assert fs.reads==1 and fs.clock==0
 before=fs.reads;reject(lambda:n['stable']([pin]));assert fs.reads==before;checks+=1
fs,n,p,pin=written('churn');reject(lambda:n['stable']([pin]));assert fs.clock<=30 and fs.reads<=30;checks+=1
fs,n,p,pin=written();rows=n['stable']([pin]);old=json.dumps(n['new_files'],sort_keys=True,default=str);fs.force=fs.stable(202)
reject(lambda:n['read'](p,rows[0]));reject(lambda:n['stable']([pin]));assert json.dumps(n['new_files'],sort_keys=True,default=str)==old;checks+=1
for field,value in [('material',FS().creation),('materialSha256',hashlib.sha256(json.dumps(FS().creation,separators=(',',':')).encode()).hexdigest())]:
 fs,n=env();fs.created=True;fs.force=fs.stable(201);pin={'path':'/synthetic-owner/new.json','sha256':hashlib.sha256(fs.data).hexdigest(),'bytes':len(fs.data),field:value}
 reject(lambda:n['stable']([pin]));assert fs.reads==1 and fs.clock==0;checks+=1
print(json.dumps({'passed':True,'scenarios':checks,'filesystemMocked':True,'clockMocked':True,'ownerEntrypointExecuted':False,'networkStarted':False,'newSignaturesCreated':0},sort_keys=True))
`;
test('VPS Python exact write/sample/stable/read helpers obey the same NEW-only and retained full9 contract',{timeout:12000},async()=>{
 const source=await readFile(new URL('./lifecycle-owner.mjs',import.meta.url),'utf8');
 const helpers=between(source,'def need(v):','def artifact(');
 const configured=process.env.NANO_LIFECYCLE_TEST_PYTHON??'/usr/bin/python3';
 assert(isAbsolute(configured)&&resolve(configured)===configured&&!/[\0\r\n]/.test(configured),'NANO_LIFECYCLE_TEST_PYTHON must be a normalized absolute executable path');
 const python=await realpath(configured),interpreter=await lstat(python);assert(interpreter.isFile()&&(interpreter.mode&0o111)!==0,'NANO_LIFECYCLE_TEST_PYTHON must resolve to an executable regular file');
 const child=spawnSync(python,['-I','-c',pythonHarness],{input:JSON.stringify({source:helpers}),encoding:'utf8',timeout:10000,maxBuffer:1048576,env:{PATH:'/usr/bin:/bin',HOME:'/nonexistent',LANG:'C',LC_ALL:'C'}});
 assert.ifError(child.error);assert.equal(child.signal,null);assert.equal(child.status,0,child.stderr);assert.equal(child.stderr,'');
 assert.deepEqual(JSON.parse(child.stdout),{passed:true,scenarios:16,filesystemMocked:true,clockMocked:true,ownerEntrypointExecuted:false,networkStarted:false,newSignaturesCreated:0});
});
