#!/usr/bin/env python3
"""ROOT-only setup of a new, non-operational Linux consumer qualification.

Receives public controller bytes on stdin; private metadata remains on the VPS.
Does not run Docker, compile, execute candidate programs, or read an old SDK map.
"""
import base64, hashlib, json, os, pathlib, re, stat, subprocess, sys, time

FIELDS=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
BASE=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
REPO=pathlib.Path('/home/ubuntu/blockchain-nano-git')
REFS={
 'sourceAdmission':('control/source-first-admission.private.json','59375ca7bc7461839161d27172a987e06cdffe639151d478ab31550564ab2dd8',28972841),
 'sourceRetained':('control06/resume06-source-retained-on-site.private.json','71df9ee708d08243e083cd664224a8f1a779befdd56403df2e45fd9d61c1c9fe',1641),
 'artifactAdmission':('control06/new-artifacts-first-admission.private.json','4e9ff3b48b05c78e3fec34ac3f02f6c2088ae73d4b20e5a5ea493c496749f906',4945),
 'consumerLinks':('control06/actual-consumer-links.private.json','55ff7525510d2bad38cddd387157d3e9f4eeb1397e66f1712bde4e718a8b5b53',654679),
 'linkedArchives':('control06/actual-linked-new-archives.private.json','3a4f182a5e882a4f3713122ac79b781d94dbb8b87fd1a93cf90f14bfa18af5fb',6182185),
 'portableLlvm':('control06/portable-llvm-producers.private.json','ccc3bd6d6450ab0da86cc9d8afd991d1366f1b90898383872374daf4c6e212a1',3273),
}

def need(x,msg):
 if not x:raise ValueError(msg)

def digest(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}

def observe(p):
 need(p.resolve(strict=True)==p,'physical path')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_nlink>=1 and 0<a.st_size<=268435456,'bounded regular input')
  parts=[];size=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   size+=len(b);need(size<=a.st_size,'fixed read bound');parts.append(b)
  row=[str(getattr(a,k))for k in FIELDS]
  need(size==a.st_size and row==[str(getattr(os.fstat(fd),k))for k in FIELDS]==[str(getattr(p.lstat(),k))for k in FIELDS],'FD and path unchanged')
  return dict(path=str(p),material=row,**digest(b''.join(parts)))
 finally:os.close(fd)

def stable(paths):
 previous=None;count=0;since=0;deadline=time.monotonic()+20
 while True:
  rows={k:observe(p)for k,p in paths.items()};now=time.monotonic()
  if rows==previous:count+=1
  else:previous=rows;count=1;since=now
  if count>=3 and now-since>=2:return rows,count,int((now-since)*1000)
  need(now<deadline,'stable observation timeout');time.sleep(.75)

def write(p,b,mode):
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  v=memoryview(b)
  while v:n=os.write(fd,v);need(n>0,'write');v=v[n:]
  os.fchmod(fd,mode);os.fsync(fd)
 finally:os.close(fd)
 fd=os.open(p.parent,os.O_DIRECTORY|os.O_RDONLY|os.O_NOFOLLOW)
 try:os.fsync(fd)
 finally:os.close(fd)

def main():
 need(sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000,'Ubuntu owner')
 raw=sys.stdin.buffer.read(131073);need(len(raw)<=131072,'bounded setup payload');c=json.loads(raw)
 need(set(c)=={'format','namespace','revision','controller'} and c['format']=='nano-amd-public-controller-setup-v1','exact public setup')
 need(re.fullmatch('[0-9a-f]{32}',c['namespace']) and re.fullmatch('[0-9a-f]{40}',c['revision']),'known namespace/revision')
 src=c['controller'];need(set(src)=={'sha256','bytes','base64'},'controller identity');body=base64.b64decode(src['base64'],validate=True)
 need(digest(body)=={k:src[k]for k in('sha256','bytes')} and 0<len(body)<100000,'exact reviewed controller bytes')
 need(BASE.resolve(strict=True)==BASE and BASE.is_dir(),'existing public build namespace')
 ctl=BASE/('lifecycle-consumers-control-'+c['namespace']);out=BASE/('lifecycle-consumers-'+c['namespace'])
 need(not ctl.exists() and not out.exists(),'new owned namespaces')
 ctl.mkdir(mode=0o700);out.mkdir(mode=0o700)
 script=ctl/'build-amd-consumers-root.py';write(script,body,0o400)
 header=REPO/'tests/support/fresh_lifecycle_terminal.hpp'
 rows,samples,span=stable({'controller':script,'headerAfter':header,'docker':pathlib.Path('/usr/bin/docker'),'sudo':pathlib.Path('/usr/bin/sudo'),'git':pathlib.Path('/usr/bin/git')})
 current=subprocess.check_output(['/usr/bin/git','--no-optional-locks','-C',str(REPO),'rev-parse','HEAD'],env={'PATH':'/usr/bin:/bin','HOME':'/home/ubuntu','LANG':'C','GIT_OPTIONAL_LOCKS':'0','GIT_TERMINAL_PROMPT':'0'},timeout=20).decode().strip()
 need(current==c['revision'] and observe(pathlib.Path('/usr/bin/git'))==rows['git'],'current Git revision and unchanged client')
 need((rows['headerAfter']['sha256'],rows['headerAfter']['bytes'])==('6681926f496290afc2b98495315b8ae5ba1c646095aece0e21287cca83b28629',24376),'current header known bytes')
 inp={'format':'nano-amd-lifecycle-consumers-input-v1','controller':{k:rows['controller'][k]for k in('path','sha256','bytes')},'headerAfter':rows['headerAfter'],'outputRoot':str(out),'effectiveSourceRevision':c['revision'],**{k:rows[k]for k in('docker','sudo','git')},**{k:{'path':str(BASE/rel),'sha256':sha,'bytes':size}for k,(rel,sha,size)in REFS.items()}}
 data=(json.dumps(inp,sort_keys=True,separators=(',',':'))+'\n').encode();p=ctl/'inputs.private.json';write(p,data,0o400)
 allrows,n,ms=stable({**{k:pathlib.Path(r['path'])for k,r in rows.items()},'input':p})
 need(all(allrows[k]==v for k,v in rows.items()),'original setup rows retained')
 descriptor=ctl/'setup-admission.private.json';write(descriptor,(json.dumps({'format':'nano-amd-new-setup-admission-v1','samples':n,'stableSpanMs':ms,'materials':allrows,'operationalAuthority':False,'historicalAuthoritiesRefreshed':False},sort_keys=True,separators=(',',':'))+'\n').encode(),0o400)
 need(not list(out.iterdir()),'empty qualification output')
 print(json.dumps({'format':'nano-amd-public-controller-prepared-v1','passed':True,'namespace':c['namespace'],'revision':current,'controller':digest(body),'input':digest(data),'setupAdmission':digest(descriptor.read_bytes()),'samples':n,'stableSpanMs':ms,'oldAuthoritiesRefreshed':False,'dockerCommands':0,'compilations':0,'runtimeStarted':False,'productionQualified':False}),flush=True)

if __name__=='__main__':main()
