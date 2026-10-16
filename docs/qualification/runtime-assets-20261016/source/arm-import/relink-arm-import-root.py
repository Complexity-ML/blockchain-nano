#!/usr/bin/env python3
"""ROOT only: one ARM importer M/relink, zero CXX.
No operation on import; inputs require authenticated selected closures.
No configure, Core/image build, seed, real bootstrap import or runtime.
"""
import argparse, ast, hashlib, json, os, pathlib, re, selectors, shlex
import signal, stat, subprocess, sys, time

F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
CORE=('9ccadf8f6479af8610a823edd3a7b4de5752cf3bde5d6251532b734c3c0a6974',41048912)
OLD_CORE=('9a03ae140676efb721b6e3562fcd55cd194ea7212e8ff7b72be107d547a95832',41006148)
OLD_IMPORTER=('3b635993dccc50458343622a148345692b0d88b0a066a68c78381cc659247095',4765512)
TARGET='nano-capella-isolated-bootstrap-import'
SOURCE='/work/tests/capella/isolated_bootstrap_import.cpp'
HELD={};NEW={};EVENTS=[];OUT=None;DOCKER_FILE=None;DOCKER_RECORD=None;PHASE='admission';START=time.monotonic();DEADLINE=START+1100

def need(v,m):
 if not v:raise ValueError(m)
def exact(v,k,m):need(isinstance(v,dict)and set(v)==set(k),m)
def physical(v):
 need(isinstance(v,str)and v.startswith('/')and not re.search(r'[\x00\r\n,\\]',v),'physical locator text')
 p=pathlib.Path(v);need(str(p)==os.path.normpath(v)and p.resolve(strict=True)==p,'physical canonical locator');return p
def digest(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def artifact(x):
 need(isinstance(x,dict)and re.fullmatch('[0-9a-f]{64}',x.get('sha256',''))and type(x.get('bytes'))is int and 0<x['bytes']<=268435456,'known byte descriptor')
 return physical(x['path'])
def material(p):
 p=physical(str(p));fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and 0<=a.st_size<=268435456,'bounded regular FD')
  chunks=[];size=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   size+=len(b);need(size<=a.st_size,'FD bound');chunks.append(b)
  row={k:getattr(a,k)for k in F};need(size==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'FD/path nine fields')
  b=b''.join(chunks);return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)
def recorded(p,row):
 exact(row,(*F,'sha256'),'original full nine SHA record');p=physical(str(p));seen,b=material(p)
 need(seen==row and(str(p)not in HELD or HELD[str(p)]==row),'retained original authority differs');HELD[str(p)]=dict(row);return b
def first(ref):
 p=artifact(ref);wanted={k:ref[k]for k in('sha256','bytes')}
 if str(p)in HELD:
  b=recorded(p,HELD[str(p)]);need(digest(b)==wanted,'held known bytes');return b
 prev=None;count=0;since=0;until=min(DEADLINE,time.monotonic()+15)
 while True:
  row,b=material(p);need(digest(b)==wanted,'producer byte commitment')
  now=time.monotonic()
  if row==prev:count+=1
  else:prev=row;count=1;since=now
  if count>=3 and now-since>=2:break
  need(now<until,'first-admission quiescence');time.sleep(.75)
 HELD[str(p)]=row;return b
def fence():
 for p,row in list(HELD.items()):recorded(p,row)
 for p,pin in list(NEW.items()):need(digest(material(p)[1])==pin,'new output bytes changed')
def write(name,b):
 need(OUT is not None and isinstance(b,bytes),'owned output bytes')
 need(isinstance(name,str)and name not in('','.','..')and not re.search(r'[/\\\x00\r\n]',name),'owned output relative leaf only')
 p=OUT/name
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  view=memoryview(b)
  while view:n=os.write(fd,view);need(n>0,'output write');view=view[n:]
  os.fsync(fd)
 finally:os.close(fd)
 fd=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(fd)
 finally:os.close(fd)
 need(material(p)[1]==b,'output readback');NEW[str(p)]=digest(b);return p
def publish(name,x):return write(name,(json.dumps(x,sort_keys=True,separators=(',',':'))+'\n').encode())
def run(label,argv,env,seconds=60,input_bytes=None,cleanup=False,expected_code=0):
 global PHASE
 PHASE=label
 if cleanup:recorded(DOCKER_FILE,DOCKER_RECORD)
 else:fence()
 limit=min(seconds,(CLEAN_DEADLINE if cleanup else DEADLINE)-time.monotonic());need(limit>0,'command deadline')
 publish(label+'-argv.private.json',{'argv':argv,'maximumSeconds':limit})
 fds=[];p=None;sel=selectors.DefaultSelector();sizes=[0,0];began=time.monotonic();failed=False
 paths=[OUT/(label+'-stdout.private.log'),OUT/(label+'-stderr.private.log')]
 try:
  for path in paths:fds.append(os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600))
  inp=subprocess.DEVNULL
  if input_bytes is not None:
   need(isinstance(input_bytes,bytes)and len(input_bytes)<=1048576,'bounded stdin');ip=write(label+'-stdin.private.txt',input_bytes);inp=open(ip,'rb')
  try:p=subprocess.Popen(argv,cwd=OUT,env=env,stdin=inp,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
  finally:
   if input_bytes is not None:inp.close()
  for i,stream in enumerate((p.stdout,p.stderr)):os.set_blocking(stream.fileno(),False);sel.register(stream,selectors.EVENT_READ,i)
  while sel.get_map():
   need(time.monotonic()-began<limit,'command timeout')
   for key,_ in sel.select(.1):
    data=os.read(key.fileobj.fileno(),65536)
    if not data:sel.unregister(key.fileobj);key.fileobj.close();continue
    i=key.data;sizes[i]+=len(data);need(sizes[i]<=33554432,'stream limit');v=memoryview(data)
    while v:n=os.write(fds[i],v);need(n>0,'log write');v=v[n:]
  code=p.wait(timeout=max(.01,min(5,limit-(time.monotonic()-began))));need(type(expected_code)is int and expected_code in(0,1)and code==expected_code,'command expected exit code')
 except BaseException:
  failed=True
  if p is not None and p.poll()is None:
   try:os.killpg(p.pid,signal.SIGTERM)
   except ProcessLookupError:pass
   try:p.wait(timeout=5)
   except subprocess.TimeoutExpired:
    try:os.killpg(p.pid,signal.SIGKILL)
    except ProcessLookupError:pass
    p.wait(timeout=5)
  raise
 finally:
  sel.close()
  if p is not None:
   for stream in(p.stdout,p.stderr):
    if stream is not None:stream.close()
  for fd in fds:
   try:os.fsync(fd)
   finally:os.close(fd)
  for path in paths:
   if path.exists():NEW[str(path)]=digest(material(path)[1])
  EVENTS.append({'phase':label,'exitCode':None if p is None else p.returncode,'passed':not failed,'elapsedMs':int((time.monotonic()-began)*1000),'stdoutBytes':sizes[0],'stderrBytes':sizes[1]})
  publish(label+'-exit.json',EVENTS[-1])
  if cleanup:recorded(DOCKER_FILE,DOCKER_RECORD)
  else:fence()
 return tuple(material(path)[1]for path in paths)

SDK_SCRIPT=b'''set -eu
for p do
 r=$(/usr/bin/readlink -f -- "$p"); test -f "$r"
 a=$(/usr/bin/stat -c '%d|%i|%f|%h|%u|%g|%s|%y|%z' -- "$p")
 t=REGULAR; if test -L "$p"; then t=$(/usr/bin/readlink -- "$p"); fi
 exec 3<"$r"
 b=$(/usr/bin/stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' /proc/self/fd/3)
 test "$b" = "$(/usr/bin/stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' -- "$r")"
 h=$(/usr/bin/sha256sum <&3); h=${h%% *}
 test "$b" = "$(/usr/bin/stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' /proc/self/fd/3)"
 test "$b" = "$(/usr/bin/stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' -- "$r")"
 exec 3<&-
 test "$a" = "$(/usr/bin/stat -c '%d|%i|%f|%h|%u|%g|%s|%y|%z' -- "$p")"
 test "$r" = "$(/usr/bin/readlink -f -- "$p")"
 u=REGULAR; if test -L "$p"; then u=$(/usr/bin/readlink -- "$p"); fi; test "$u" = "$t"
 printf '%s\\n' "$p" "$r" "$a" "$t" "$b" "$h"
done
'''

def dependencies(raw):
 text=raw.decode().replace('\\\n',' ');need(':'in text,'actual dependency rule')
 names={os.path.normpath(p if p.startswith('/')else'/outputs/'+p)for p in shlex.split(text.split(':',1)[1],comments=False)}
 need(0<len(names)<5000 and all(p.startswith(('/work/','/outputs/','/usr/','/bin/','/lib/','/lib64/','/src/.deps/'))and not re.search(r'[\x00\r\n]',p)for p in names),'bounded normalized dependency namespace');return names

def scan_arguments(argv):
 need(isinstance(argv,list)and all(isinstance(x,str)and not re.search(r'[\x00\r\n]',x)for x in argv),'canonical argv strings')
 need(argv[0]=='/usr/bin/clang++'and argv.count('-c')==argv.count('-o')==argv.count(SOURCE)==1,'unique importer canonical source/object slots')
 need(argv.index('-o')+1<len(argv),'canonical object argument')
 result=[];i=0
 while i<len(argv):
  word=argv[i]
  if word in('-o','-MF','-MT','-MQ'):need(i+1<len(argv),'canonical output argument');i+=2;continue
  if word in('-c','-M','-MM','-MD','-MMD','-MP'):i+=1;continue
  need(word not in('-MJ','-save-temps','-ftime-trace','-serialize-diagnostics'),'unowned compiler side effect');result.append(word);i+=1
 return result+['-M','-MT','arm-importer-current-core']

def relink_arguments(argv,old_core_argument,old_object_argument):
 need(isinstance(argv,list)and argv and argv[0]=='/usr/bin/clang++','canonical importer linker driver')
 need(argv.count('-o')==argv.count(old_core_argument)==argv.count(old_object_argument)==1,'unique output old-Core and original-object arguments')
 need(old_core_argument!=old_object_argument and argv.index('-o')+1<len(argv),'distinct canonical linker slots')
 changed=['/qualified-core.a'if word==old_core_argument else word for word in argv]
 changed[changed.index('-o')+1]='/control/'+TARGET
 need(all(a==b or(i==argv.index(old_core_argument)and b=='/qualified-core.a')or(i==argv.index('-o')+1 and b=='/control/'+TARGET)for i,(a,b)in enumerate(zip(argv,changed))),'only Core/output substitutions')
 return changed

def snapshot(phase,paths,guest):
 paths=sorted(set(paths));allowed=('usr','bin','lib','lib64','src/\\.deps','control','work','outputs')
 need(0<len(paths)<5000 and all(p=='/qualified-core.a'or re.fullmatch('/(?:'+'|'.join(allowed)+')/[A-Za-z0-9_+./:@=~-]+',p)for p in paths),'selected guest snapshot namespace')
 raw,err=guest(phase,['/bin/sh','-s','--',*paths],120,SDK_SCRIPT);need(not err,'FD snapshot stderr');lines=raw.decode().splitlines();need(len(lines)==6*len(paths),'six FD rows per path');found={}
 for i,path in enumerate(paths):
  observed,resolved,alias,linktext,fields,sha=lines[6*i:6*i+6];row=fields.split('|');arow=alias.split('|')
  need(observed==path and len(row)==len(arow)==9 and re.fullmatch('[0-9a-f]{64}',sha),'guest FD identity shape')
  need(resolved=='/qualified-core.a'or resolved.startswith(('/usr/','/bin/','/lib/','/lib64/','/src/.deps/','/control/','/work/','/outputs/')),'guest resolved namespace')
  need(stat.S_ISREG(int(row[2],16))and int(row[3])>=1 and all(not re.search(r'[\r\n\x00]',x)for x in row+arow+[resolved,linktext]),'regular guest FD nine SHA')
  found[path]={'resolvedPath':resolved,'aliasMaterial':arow,'linkText':linktext,'material':row,'sha256':sha,'bytes':int(row[6])}
 publish(phase+'-snapshot.private.json',found);return found

def stable_guest(phase,paths,guest,expected=None):
 previous=None;count=0;since=0;until=min(DEADLINE,time.monotonic()+25);i=0
 while True:
  value=snapshot(phase+'-'+str(i),paths,guest);i+=1;now=time.monotonic()
  if expected is not None:need(value==expected,'retained guest namespace changed')
  if value==previous:count+=1
  else:previous=value;count=1;since=now
  if count>=3 and now-since>=2:return value
  need(now<until,'new guest first-admission bound');time.sleep(.75)

def refusal_result(raw,err):
 need(not err and len(raw)<=4096,'bounded refusal stdout only')
 value=json.loads(raw)
 expected={'format':'nano-isolated-fixture-private-binding-audit-v1','passed':False,'refusedStage':'arguments','errorDetailsSuppressed':True,'newSignaturesCreated':0,'signerCallbacksInvoked':0,'runtimeStarted':False,'networkLaunched':False,'productionQualified':False}
 exact(value,expected,'exact nine refusal keys');need(value==expected,'exact no-extra-argument importer refusal');return value

class OwnedArmSession:
 def __init__(self,docker,endpoint,prefix,source,build,core):
  need(OUT is not None and re.fullmatch('nano-arm-import-current-core-[0-9a-f]{32}',prefix),'new owned session namespace')
  need(endpoint=='unix:///Users/boris/.docker/run/docker.sock','explicit local Docker endpoint')
  self.docker=str(docker);self.endpoint=endpoint;self.prefix=prefix;self.container=prefix+'-link';self.volume=prefix+'-control';self.label='nano.arm.import.current.core';self.cid=None;self.counter=0;self.volume_attempt=False;self.container_attempt=False
  self.source=physical(str(source));self.build=physical(str(build));self.core=physical(str(core));self.client=OUT/'docker-client';self.client.mkdir(mode=0o700)
  self.env={'PATH':'/usr/bin:/bin','HOME':str(self.client),'LANG':'C','LC_ALL':'C'}
 def docker_command(self,phase,args,seconds=60,input_bytes=None,cleanup=False,expected_code=0):
  self.counter+=1
  return run(str(self.counter)+'-'+phase,[self.docker,'-H',self.endpoint,'--config',str(self.client),*args],self.env,seconds,input_bytes,cleanup,expected_code)
 def guest(self,phase,args,seconds=120,input_bytes=None,expected_code=0):
  need(self.cid is not None,'owned session created')
  return self.docker_command(phase,['exec','--user','501:20',*(['-i']if input_bytes is not None else[]),self.cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C',*args],seconds,input_bytes,expected_code=expected_code)
 def start(self):
  raw,err=self.docker_command('image-inspect',['image','inspect',IMAGE,'--format','{{json .}}']);image=json.loads(raw)
  need(not err and image['Id']==IMAGE and image['Architecture']=='arm64'and image['Os']=='linux','existing immutable ARM image')
  raw,err=self.docker_command('container-absent',['ps','-a','--filter','name=^/'+self.container+'$','--format','{{.ID}}']);need(not raw.strip()and not err,'new container absent')
  raw,err=self.docker_command('volume-absent',['volume','ls','--filter','name=^'+self.volume+'$','--format','{{.Name}}']);need(not raw.strip()and not err,'new volume absent immediately before create')
  self.volume_attempt=True
  raw,err=self.docker_command('volume-create',['volume','create','--label',self.label+'='+self.prefix,self.volume]);need(not err and raw.decode().strip()==self.volume,'new volume created')
  raw,err=self.docker_command('volume-owned-before-mount',['volume','inspect',self.volume,'--format','{{json .}}']);volume=json.loads(raw)
  need(not err and volume['Name']==self.volume and volume['Labels'].get(self.label)==self.prefix,'owned volume label before any mutation')
  mounts=['--mount','type=bind,src='+str(self.source)+',dst=/work,readonly','--mount','type=bind,src='+str(self.build)+',dst=/outputs,readonly','--mount','type=bind,src='+str(self.core)+',dst=/qualified-core.a,readonly','--mount','type=volume,src='+self.volume+',dst=/control']
  raw,err=self.docker_command('container-absent-at-create',['ps','-a','--filter','name=^/'+self.container+'$','--format','{{.ID}}']);need(not raw.strip()and not err,'new container absent immediately before create')
  self.container_attempt=True
  raw,err=self.docker_command('container-create',['create','--pull','never','--name',self.container,'--label',self.label+'='+self.prefix,'--network','none','--read-only','--cap-drop','ALL','--cap-add','CHOWN','--security-opt','no-new-privileges','--pids-limit','256','--memory','4g','--cpus','2','--user','0:0','--env','PATH=/usr/bin:/bin','--tmpfs','/tmp:rw,nosuid,nodev,size=134217728',*mounts,'--workdir','/outputs','--entrypoint','/bin/sh',IMAGE,'-ec','test -z "$(ls -A /control)"; chmod 700 /control; chown 501:20 /control; exec sleep 1050']);self.cid=raw.decode().strip();need(not err and re.fullmatch('[0-9a-f]{64}',self.cid),'new full container ID')
  raw,err=self.docker_command('container-start',['start',self.cid]);need(not err and raw.decode().strip()==self.cid,'owned container started')
  raw,err=self.guest('volume-owner',['/bin/sh','-ec','test "$(/usr/bin/stat -c %u:%g:%a /control)" = 501:20:700; test -z "$(ls -A /control)"; test -r /qualified-core.a; test -r /work/tests/capella/isolated_bootstrap_import.cpp; test -r /outputs/compile_commands.json; printf "OWNED501:20:700\\n"']);need(not err and raw==b'OWNED501:20:700\n','actual Linux owner and read-only roots readable')
 def cleanup(self,candidate_passed):
  global CLEAN_DEADLINE
  CLEAN_DEADLINE=time.monotonic()+60
  if self.container_attempt:
   if self.cid is None:
    raw,err=self.docker_command('cleanup-discover',['ps','-a','--no-trunc','--filter','name=^/'+self.container+'$','--format','{{.ID}}'],10,cleanup=True);need(not err and len(raw.splitlines())<=1,'owned container discovery');self.cid=raw.decode().strip()or None
   if self.cid:
    need(re.fullmatch('[0-9a-f]{64}',self.cid),'owned full container ID')
    raw,err=self.docker_command('cleanup-inspect',['inspect',self.cid,'--format','{{json .}}'],10,cleanup=True);item=json.loads(raw)
    need(not err and item['Id']==self.cid and item['Name']=='/'+self.container and item['Image']==IMAGE and item['Config']['Labels'].get(self.label)==self.prefix,'owned container image name label')
    self.docker_command('cleanup-remove',['rm','-f',self.cid],15,cleanup=True)
   raw,err=self.docker_command('cleanup-container-absent',['ps','-a','--filter','name=^/'+self.container+'$','--format','{{.ID}}'],10,cleanup=True);need(not raw.strip()and not err,'owned container absent')
  if self.volume_attempt:
   raw,err=self.docker_command('cleanup-volume-discover',['volume','ls','--filter','name=^'+self.volume+'$','--format','{{.Name}}'],10,cleanup=True);need(not err and len(raw.splitlines())<=1,'owned volume discovery')
   if raw.strip():
    need(raw.decode().strip()==self.volume,'owned volume name')
    raw,err=self.docker_command('cleanup-volume-inspect',['volume','inspect',self.volume,'--format','{{json .}}'],10,cleanup=True);item=json.loads(raw)
    need(not err and item['Name']==self.volume and item['Labels'].get(self.label)==self.prefix,'owned volume label');self.docker_command('cleanup-volume-remove',['volume','rm',self.volume],10,cleanup=True)
   raw,err=self.docker_command('cleanup-volume-absent',['volume','ls','--filter','name=^'+self.volume+'$','--format','{{.Name}}'],10,cleanup=True);need(not raw.strip()and not err,'owned volume absent')
  publish('cleanup.json',{'ownedContainerCleanupAttempted':self.container_attempt,'ownedVolumeCleanupAttempted':self.volume_attempt,'candidatePassedBeforeCleanup':candidate_passed})

ROOT=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
HISTORICAL=ROOT/'historical-beacon-rpc-20261015'
CURRENT_REVISION='944aa77b6d92e3ac609938f6d5a024acd88cc118'
COMMON=('oldAuthoritiesRefreshed','historicalInputRawDigestRecoveredOffsite','historicalProtectedTableFullNineRecovered','originalProtectedDigestRecordedInInput','sdkBytesRead','productionQualified')
REFERENCES={
 'originalClosure':('selected-original-closure.private.json','566d2db8a58f544ca4574e9b08cd20b42c3f5384b1ad507d112322d78c6b39cb',107669),
 'currentSourceAdmission':('selected-current-source.private.json','28db2e08e9fe416f0bae8b9c4499789f559227417b972c1c0b128f4b8afdc79a',9016),
 'currentCoreAdmission':('selected-current-core.private.json','416695bc6e41d41b499a7ae1d5916a5e7a4deb24f2106109df361e5c8c3cf6d7',1124)
}

def common_scope(value):
 need(all(value[k]is False for k in COMMON if k!='originalProtectedDigestRecordedInInput')and value['originalProtectedDigestRecordedInInput']is True,'Root selected proof limitations unchanged')

def main(a):
 global OUT,DOCKER_FILE,DOCKER_RECORD
 need(sys.platform=='darwin'and os.getuid()==501,'Mac host owner501')
 c=json.loads(first({'path':a.inputs,'sha256':a.sha256,'bytes':a.bytes}))
 exact(c,('format','controller',*REFERENCES,'docker','dockerEndpoint','controlRoot','containerPrefix'),'input9')
 need(c['format']=='nano-arm-import-current-core-input-v1'and artifact(c['controller'])==physical(__file__),'controller input');first(c['controller'])
 proofs={}
 for key,(leaf,sha,size)in REFERENCES.items():
  ref=c[key];need(artifact(ref)==ROOT/'fresh-arm-bootstrap-import-current-core-root-20261016'/leaf and(ref['sha256'],ref['bytes'])==(sha,size),'fixed Root selected descriptor')
  proofs[key]=json.loads(first(ref));common_scope(proofs[key])
 old=proofs['originalClosure'];current=proofs['currentSourceAdmission'];new_core=proofs['currentCoreAdmission']
 exact(old,('format','sourceRoot','buildRoot','compileDatabasePath','compileRecipe','linkRecipePath','linkArgv','objectGuestPath','originalMdPath','originalExecutablePath','guestDependencies','projectDependencies','generatedDependencies','imageDependencies','linkInputsGuestOrder','files','originalInput','protectedTable','compileDbNineAlsoRetainedInRotationAdmission','consumerLinkOriginalNineRetained',*COMMON),'selected original closure keys')
 exact(current,('format','repositoryRoot','revision','projectFiles','projectDependencyCount','allSelectedSourceBytesMatchCurrentGit','changedTerminalHeaderReached',*COMMON),'current-source proof keys')
 exact(new_core,('format','corePath','coreMaterial','retainedAdmission','retainedCoreFullNineUnchanged','originalFirstProducerCoreNineRecovered',*COMMON),'current-Core proof keys')
 need(old['format']=='nano-arm-import-original-closure-v1'and current['format']=='nano-arm-import-current-source-v1'and new_core['format']=='nano-arm-import-current-core-v1','exact selected formats')
 source=physical(old['sourceRoot']);build=physical(old['buildRoot']);need(source==HISTORICAL/'source'and build==HISTORICAL/'linux-arm-build','original source/build roots')
 need(old['compileDbNineAlsoRetainedInRotationAdmission']is True and old['consumerLinkOriginalNineRetained']is True,'original DB and consumer recipe full nine retained')
 need(old['originalInput']=={'path':str(HISTORICAL/'controls/arm-historical-test-inputs.json'),'sha256':'ea754f3bc60c4aad479145a90da7893b6f350f387667d71ee92e9c0e2f8b4022','bytes':1358}and old['protectedTable']=={'path':str(HISTORICAL/'controls/arm-historical-test-protected-inputs.json'),'sha256':'24460bbd5eb05836ffe1835737f2cf36852075f81a28ba6a947fcd8500108ae7','bytes':6055965},'selected original provenance exact')
 originals=old['files'];need(isinstance(originals,dict)and len(originals)==77,'exact original selected material namespace')
 def prior(path):
  path=physical(str(path));need(str(path)in originals,'original selected authority missing');return recorded(path,originals[str(path)])
 for path in originals:
  p=physical(path);need(p.is_relative_to(source)or p.is_relative_to(build),'selected original source/build only');prior(p)
 def host(guest):
  need(isinstance(guest,str)and os.path.normpath(guest)==guest,'normalized guest locator')
  if guest.startswith('/work/'):return source/pathlib.PurePosixPath(guest).relative_to('/work')
  need(guest.startswith('/outputs/'),'only source/build guest projection');return build/pathlib.PurePosixPath(guest).relative_to('/outputs')
 db=physical(old['compileDatabasePath']);need(db==build/'compile_commands.json','original compile DB path')
 rows=json.loads(prior(db));selected=[row for row in rows if row['file']==SOURCE];need(len(selected)==1 and selected[0]==old['compileRecipe']and selected[0]['directory']=='/outputs','unique actual original importer recipe')
 recipe=selected[0];argv=recipe.get('arguments')or shlex.split(recipe['command']);scan=scan_arguments(argv)
 object_argument=argv[argv.index('-o')+1];object_guest=os.path.normpath(object_argument if object_argument.startswith('/')else'/outputs/'+object_argument)
 need(object_guest==old['objectGuestPath']and physical(old['originalMdPath'])==host(object_guest+'.d'),'actual original object and MD join');prior(host(object_guest))
 original_md=dependencies(prior(physical(old['originalMdPath'])))
 need(old['guestDependencies']==sorted(original_md)and len(original_md)==474 and SOURCE in original_md and '/work/tests/support/fresh_lifecycle_terminal.hpp'not in original_md,'exact original importer MD and terminal-header scope')
 project=old['projectDependencies'];generated=old['generatedDependencies'];image=old['imageDependencies']
 need(isinstance(project,dict)and len(project)==65 and generated=={}and isinstance(image,list)and len(image)==len(set(image))==409,'actual original dependency partition')
 need(set(project)|set(generated)|set(image)==original_md and not(set(project)&set(image))and not(set(generated)&set(image)),'complete original dependency partition')
 selected_project={};known_guest={}
 for group,prefix in((project,'/work/'),(generated,'/outputs/')):
  for guest,ref in group.items():
   exact(ref,('path','material'),'selected dependency record');need(guest.startswith(prefix)and physical(ref['path'])==host(guest)and originals[ref['path']]==ref['material'],'original dependency projection and old nine equality')
   raw=recorded(physical(ref['path']),ref['material']);known_guest[guest]=digest(raw)
   if prefix=='/work/':selected_project[str(host(guest).relative_to(source))]=digest(raw)
 need(all(isinstance(p,str)and p.startswith(('/usr/','/bin/','/lib/','/lib64/','/src/.deps/'))for p in image),'original SDK image names only')
 need(current['repositoryRoot']=='/Users/boris/Dev/blockchain-nano'and current['revision']==CURRENT_REVISION and current['projectFiles']==selected_project and current['projectDependencyCount']==65 and current['allSelectedSourceBytesMatchCurrentGit']is True and current['changedTerminalHeaderReached']is False,'Root-attested exact current Git selected-source equality')
 linkpath=physical(old['linkRecipePath']);need(linkpath==build/('CMakeFiles/'+TARGET+'.dir/link.txt'),'actual importer recipe path');linkraw=prior(linkpath)
 need(digest(linkraw)=={'sha256':'165ee34babd5461fdb8d6bddd76593b54ad39e1ad786511ef48f0ef0984343ea','bytes':597},'actual importer link byte commitment')
 link=shlex.split(linkraw.decode());need(link==old['linkArgv']and link[0]==argv[0]and link.count('-o')==1,'actual original link argv')
 actual=[];core_argument=None;own_argument=None;archive_count=0;asm_count=0;sdk=set(image)|{'/usr/bin/clang++','/usr/bin/ld','/bin/sh','/usr/bin/env','/usr/bin/stat','/usr/bin/readlink','/usr/bin/sha256sum','/usr/bin/cat','/usr/bin/od','/usr/bin/chmod','/usr/bin/ldd'}
 for i,word in enumerate(link):
  if i==link.index('-o')+1 or word.startswith('-'):continue
  if word.endswith(('.a','.o')):
   guest=os.path.normpath(word if word.startswith('/')else'/outputs/'+word);need(guest.startswith('/outputs/'),'original physical archive/object input');raw=prior(host(guest));known_guest[guest]=digest(raw);actual.append(guest)
   if guest==object_guest:need(own_argument is None,'one original importer object');own_argument=word
   elif guest=='/outputs/libnano_core.a':need(core_argument is None and digest(raw)=={'sha256':OLD_CORE[0],'bytes':OLD_CORE[1]},'historical original Core binding');core_argument=word;archive_count+=1
   else:archive_count+=word.endswith('.a');asm_count+=word.endswith('.o')
  elif word.endswith('.so')or re.search(r'\.so\.[0-9.]+$',word):need(word.startswith(('/usr/','/lib/','/lib64/','/src/.deps/')),'only image shared input');sdk.add(word)
 need(actual==old['linkInputsGuestOrder']and archive_count==5 and asm_count==2 and core_argument is not None and own_argument is not None,'actual importer ordered five archives and two standalone ASM')
 linked=relink_arguments(link,core_argument,own_argument)
 original_program=physical(old['originalExecutablePath']);need(original_program==build/TARGET and digest(prior(original_program))=={'sha256':OLD_IMPORTER[0],'bytes':OLD_IMPORTER[1]},'historical importer unchanged')
 core=physical(new_core['corePath']);need(core==ROOT/'beacon-validator-read-implementation-20261015/arm-03/libnano_core.a'and new_core['retainedCoreFullNineUnchanged']is True and new_core['originalFirstProducerCoreNineRecovered']is False,'retained current Core provenance scope')
 retained_ref=new_core['retainedAdmission'];need(retained_ref=={'path':str(ROOT/'fresh-linux-lifecycle-tool-u64-root-20261016/arm-02/new-software-admission.private.json'),'sha256':'a4772c9bc694d8f43c08b52f92b3e7982b089563e89c5c4507fa01acc7032827','bytes':298848},'known retained current Core descriptor')
 retained=json.loads(first(retained_ref));need(retained['hostFiles'][str(core)]==new_core['coreMaterial'],'retained physical Core row joins original record')
 need(digest(recorded(core,new_core['coreMaterial']))=={'sha256':CORE[0],'bytes':CORE[1]},'current Core known bytes and retained full nine');known_guest['/qualified-core.a']={'sha256':CORE[0],'bytes':CORE[1]}
 exact(c['docker'],('path','sha256','bytes','material'),'explicit Docker utility record');docker=artifact(c['docker']);need(digest(recorded(docker,c['docker']['material']))=={k:c['docker'][k]for k in('sha256','bytes')},'Docker utility full nine SHA');DOCKER_FILE=docker;DOCKER_RECORD=dict(c['docker']['material'])
 candidate_out=physical(c['controlRoot']);need(candidate_out.is_dir()and candidate_out.stat().st_uid==501 and stat.S_IMODE(candidate_out.stat().st_mode)==0o700 and not list(candidate_out.iterdir()),'NEW empty Mac output namespace')
 need(all(not candidate_out.is_relative_to(p)and not p.is_relative_to(candidate_out)for p in(source,build,core.parent))and all(not pathlib.Path(p).is_relative_to(candidate_out)and not candidate_out.is_relative_to(pathlib.Path(p))for p in HELD),'output disjoint from all held inputs/source/build/Core')
 OUT=candidate_out
 need(c['dockerEndpoint']=='unix:///Users/boris/.docker/run/docker.sock'and re.fullmatch('nano-arm-import-current-core-[0-9a-f]{32}',c['containerPrefix']),'explicit local endpoint/new UUID')
 session=None;failure=None;result=None;passed=False
 try:
  session=OwnedArmSession(docker,c['dockerEndpoint'],c['containerPrefix'],source,build,core);session.start()
  guest_sources=stable_guest('inputs-before',known_guest,session.guest)
  need(all({k:guest_sources[p][k]for k in('sha256','bytes')}==pin for p,pin in known_guest.items()),'read-only guest bytes equal retained host producers')
  compile_sdk_before=stable_guest('sdk-before-m',sdk,session.guest);sdk_before=dict(compile_sdk_before);fence()
  raw,err=session.guest('M',scan,120);need(not err and dependencies(raw)==original_md,'one genuine M equals original actual MD')
  need(snapshot('sdk-after-m',sdk,session.guest)==compile_sdk_before and snapshot('inputs-after-m',known_guest,session.guest)==guest_sources,'same-container SDK and old input first-nine unchanged before link')
  raw,err=session.guest('LINK',linked,180);need(not err,'exact importer relink stderr')
  raw,err=session.guest('final-mode',['/usr/bin/chmod','500','/control/'+TARGET]);need(not raw and not err,'final mode before first new program admission')
  programs=stable_guest('new-program',['/control/'+TARGET],session.guest);program=programs['/control/'+TARGET]
  need(program['material'][3:6]==['1','501','20']and stat.S_IMODE(int(program['material'][2],16))==0o500,'new owned Linux executable final mode')
  raw,err=session.guest('ELF',['/usr/bin/od','-An','-tx1','-N20','/control/'+TARGET]);tokens=raw.decode().split()
  need(not err and len(tokens)==20 and tokens[:6]==['7f','45','4c','46','02','01']and tokens[18:20]==['b7','00']and tokens[16:18]in(['02','00'],['03','00']),'new ARM64 ELF64 LE executable')
  raw,err=session.guest('loader',['/usr/bin/ldd','/control/'+TARGET],30);need(not err and raw and b'not found'not in raw,'new known importer loader resolution');loader=set()
  for line in raw.decode().splitlines():
   for token in line.split():
    if token.startswith(('/usr/','/lib/','/lib64/','/src/.deps/')):need(re.fullmatch(r'/[A-Za-z0-9_+./:@=~-]+',token),'loader path text');loader.add(token)
  need(loader,'loader shared files');extra=loader-set(sdk_before)
  if extra:sdk_before.update(stable_guest('loader-extra-before-refusal',extra,session.guest));sdk.update(extra)
  need(snapshot('sdk-before-refusal',sdk,session.guest)==sdk_before,'first compile SDK retained across loader resolution')
  raw,err=session.guest('arguments-refusal',['/control/'+TARGET],30,expected_code=1);refusal=refusal_result(raw,err)
  final_sdk=snapshot('sdk-after-refusal',sdk,session.guest)
  need(final_sdk==sdk_before and {p:final_sdk[p]for p in compile_sdk_before}==compile_sdk_before and snapshot('program-after-refusal',programs,session.guest)==programs and snapshot('inputs-after-refusal',known_guest,session.guest)==guest_sources,'same-container compile/runtime SDK/program/source/inputs final nine SHA');fence()
  dest=OUT/TARGET;need(not dest.exists(),'only new host program leaf');raw,err=session.docker_command('copy-new-program',['cp',session.cid+':/control/'+TARGET,str(dest)]);need(not raw and not err,'copy only new program')
  need(dest.lstat().st_uid==501 and dest.lstat().st_nlink==1,'new host copy owner/link');os.chmod(dest,0o500)
  fd=os.open(dest,os.O_RDONLY|os.O_NOFOLLOW)
  try:os.fsync(fd)
  finally:os.close(fd)
  fd=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
  try:os.fsync(fd)
  finally:os.close(fd)
  program_ref={'path':str(dest),'sha256':program['sha256'],'bytes':program['bytes']};first(program_ref);NEW[str(dest)]={k:program_ref[k]for k in('sha256','bytes')}
  need(snapshot('program-after-copy',programs,session.guest)==programs,'read-only copy preserved original Linux program admission');fence()
  admission=publish('new-software-admission.private.json',{'format':'nano-arm-import-current-core-software-admission-v1','sourceRevision':CURRENT_REVISION,'imageId':IMAGE,'inputProofs':{k:c[k]for k in REFERENCES},'retainedCurrentCoreProof':retained_ref,'usedHostMaterials':HELD,'guestSdkMaterials':sdk_before,'guestCompileSdkMaterials':compile_sdk_before,'guestAdditionalRuntimeSdkMaterials':{p:sdk_before[p]for p in extra},'guestReadOnlyInputs':guest_sources,'guestProgramMaterials':programs,'program':dict(program_ref,material=HELD[str(dest)]),'originalImporter':{'sha256':OLD_IMPORTER[0],'bytes':OLD_IMPORTER[1],'coreSha256':OLD_CORE[0]},'currentCoreSha256':CORE[0],'originalCompileArgv':argv,'originalLinkArgv':link,'newLinkArgv':linked,'genuineDependencies':sorted(original_md),'historicalInputRawDigestRecoveredOffsite':False,'historicalProtectedTableFullNineRecovered':False,'originalFirstProducerCoreNineRecovered':False,'oldAuthoritiesRefreshed':False,'productionQualified':False});first(dict(digest(material(admission)[1]),path=str(admission)));fence()
  result={'format':'nano-arm-import-current-core-terminal-v1','passed':True,'owner':'mac-docker','sourceRevision':CURRENT_REVISION,'imageId':IMAGE,'originalImporter':{'sha256':OLD_IMPORTER[0],'bytes':OLD_IMPORTER[1]},'originalCoreSha256':OLD_CORE[0],'newCore':{'sha256':CORE[0],'bytes':CORE[1]},'newImporter':{k:program_ref[k]for k in('sha256','bytes')},'originalSelectedRowsUnchanged':77,'independentMEqualsOriginalMd':True,'dependencyCount':474,'currentGitProjectDependenciesRootAttested':65,'generatedDependencies':0,'imageDependencyNames':409,'archiveInputs':5,'existingStandaloneAsmObjects':2,'dependencyOnlyM':1,'consumerLinks':1,'consumerCompilations':0,'newCoreCompilations':0,'configureCommands':0,'imageBuilds':0,'imagePulls':0,'argumentsRefusalProbes':1,'argumentsRefusal':refusal,'newSignaturesCreated':0,'signerCallbacksInvoked':0,'historicalInputRawDigestRecoveredOffsite':False,'historicalProtectedTableFullNineRecovered':False,'originalFirstProducerCoreNineRecovered':False,'historicalSdkAuthorityRecovered':False,'newCompileSdkBeforeMAndAfterProbeEqual':True,'newRuntimeSdkAdmittedBeforeProbe':True,'additionalRuntimeSdkRows':len(extra),'guestAndHostInodesCompared':False,'oldAuthoritiesRefreshed':False,'softwareAdmission':digest(material(admission)[1]),'import65Replayed':False,'all24RolesAdmitted':False,'wholeLifecycleQualified':False,'runtimeStarted':False,'commonFinalityQualified':False,'productionQualified':False};passed=True
 except BaseException as error:failure=error
 finally:
  if session is not None:
   try:session.cleanup(passed)
   except BaseException as error:
    if failure is None:failure=error
    try:publish('cleanup-refusal.private.json',{'errorType':type(error).__name__,'detailsExported':False,'firstFailureRetained':True})
    except BaseException:pass
 if failure is not None:raise failure
 need(result is not None,'candidate absent');fence();result.update(ownedContainerAbsent=True,ownedVolumeAbsent=True,events=EVENTS);publish('terminal.json',result);print(json.dumps(result),flush=True)

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--inputs',required=True);parser.add_argument('--sha256',required=True);parser.add_argument('--bytes',required=True,type=int);a=parser.parse_args()
 def interrupted(*_):raise InterruptedError('ARM importer controller stopped')
 signal.signal(signal.SIGTERM,interrupted);signal.signal(signal.SIGINT,interrupted)
 try:main(a)
 except BaseException as error:
  result={'format':'nano-arm-import-current-core-refused-v1','passed':False,'phase':PHASE,'errorType':type(error).__name__,'detailsExported':False,'runtimeStarted':False,'productionQualified':False,'events':EVENTS}
  if OUT is not None:
   try:publish('refusal.private.json',dict(result,privateReason=str(error)))
   except BaseException:pass
  print(json.dumps(result),flush=True);raise SystemExit(1)
