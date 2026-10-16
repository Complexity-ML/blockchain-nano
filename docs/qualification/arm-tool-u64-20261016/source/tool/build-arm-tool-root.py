#!/usr/bin/env python3
"""ROOT only: one ARM Tool CXX/link and two bounded synthetic selectors.
No operation on import; no configure, Core/image build, seed or runtime.
"""
import argparse, ast, hashlib, json, os, pathlib, re, selectors, shlex
import signal, stat, subprocess, sys, time

F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
UTILITY='eeccfe7000685c704242c8554dc844047738c98edb597489c79afb85bec7b19e'
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
CORE=('9ccadf8f6479af8610a823edd3a7b4de5752cf3bde5d6251532b734c3c0a6974',41048912)
BEFORE=('07993c65ce97b3e2db1bd640abaabd7abce393e1dea57135bb08c0ad8f2729b8',24334)
AFTER=('6681926f496290afc2b98495315b8ae5ba1c646095aece0e21287cca83b28629',24376)
HEADER='/overlay/tests/support/fresh_lifecycle_terminal.hpp'
TARGET='nano-capella-deposit-audit'
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
def run(label,argv,env,seconds=60,input_bytes=None,cleanup=False):
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
  code=p.wait(timeout=max(.01,min(5,limit-(time.monotonic()-began))));need(code==0,'command nonzero')
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

def main(a):
 global OUT,CLEAN_DEADLINE,DOCKER_FILE,DOCKER_RECORD
 need(sys.platform=='darwin'and os.getuid()==501,'Mac host owner501')
 c=json.loads(first({'path':a.inputs,'sha256':a.sha256,'bytes':a.bytes}))
 exact(c,('format','controller','utility','oldObserveInput','oldObserveAdmission','overlayAdmission','docker','dockerEndpoint','controlRoot','containerPrefix'),'input10')
 need(c['format']=='nano-arm-tool-u64-input-v1'and artifact(c['controller'])==physical(__file__),'controller input');first(c['controller'])
 uraw=first(c['utility']);need(hashlib.sha256(uraw).hexdigest()==UTILITY,'frozen utility source')
 tree=ast.parse(uraw.decode());nodes=[]
 for node in tree.body:
  if isinstance(node,ast.FunctionDef)and node.name=='main':break
  need(not isinstance(node,(ast.If,ast.For,ast.While,ast.With,ast.Try)),'utility definitions only');nodes.append(node)
 u={'__name__':'arm_tool_definitions_only','__file__':str(artifact(c['utility']))};exec(compile(ast.Module(body=nodes,type_ignores=[]),u['__file__'],'exec'),u);deps=u['deps']
 oi=json.loads(first(c['oldObserveInput']));need(oi['format']=='nano-fresh-lifecycle-observe-arm-thin-input-v1'and oi['sourceManifestSha256']=='811f4ff16cc04e3205761bb7a90500f3f8858d32bac462613b51da48b51295cd'and oi['baselineSourceManifestSha256']=='97ab4ea05b1627c0884b12178f48d795f7c5734427e9750393ee531b93e4dc28','original public source profiles')
 obs=json.loads(first(c['oldObserveAdmission']));old=json.loads(first(oi['oldAdmission']));cfg=json.loads(first(oi['oldConfig']))
 need(isinstance(obs['hostFiles'],dict)and isinstance(old['hostFiles'],dict),'retained host file tables')
 originals=dict(old['hostFiles'])
 for p,row in obs['hostFiles'].items():need(p not in originals or originals[p]==row,'original authority overlap');originals[p]=row
 need(obs.get('aliases',{})=={}and old.get('aliases',{})=={},'unimplemented old aliases refuse rather than refresh')
 def prior(p):
  p=physical(str(p));need(str(p)in originals,'old material absent no hash fallback');return recorded(p,originals[str(p)])
 bundle=json.loads(first(c['overlayAdmission']));need(bundle['format']=='nano-arm-tool-u64-staged-sources-v1'and bundle['stableSamples']>=3 and bundle['stableSpanAtLeastTwoSeconds']is True and bundle['historicalObserveAdmissionRawDigestRecovered']is False and bundle['oldAuthoritiesRefreshed']is False,'prospective staged overlay scope')
 need(bundle['oldObserveInput']==c['oldObserveInput']and bundle['oldObserveAdmission']==c['oldObserveAdmission'],'staged original document joins')
 for key in('oldAdmission','oldConfig','linkRecipe','newCore','publicFixtures'):need(bundle[key]==oi[key],'staged original recipe/Profile joins')
 s,b,overlay=(physical(bundle[k])for k in('sourceRoot','buildRoot','overlayRoot'))
 need(str(s)==oi['sourceRoot']and str(b)==oi['buildRoot']and overlay!=physical(oi['overlayRoot']),'roots original except NEW overlay')
 before={r['guest']:r for r in oi['sources']};sources={r['guest']:r for r in bundle['sources']}
 need(len(oi['sources'])==len(bundle['sources'])==len(before)==len(sources)==9 and set(before)==set(sources)and HEADER in sources,'exact nine overlay guests')
 need(set(bundle['heldMaterials'])=={r['path']for r in sources.values()},'exact nine staged material namespace')
 for guest,row in sources.items():
  need(guest.startswith('/overlay/')and str(pathlib.PurePosixPath(guest))==guest and '..'not in pathlib.PurePosixPath(guest).parts,'overlay guest canonical')
  need(artifact(row)==overlay/pathlib.PurePosixPath(guest).relative_to('/overlay'),'overlay host projection')
  oldref=row['oldSourceArtifact'];need({k:oldref[k]for k in('path','sha256','bytes')}=={k:before[guest][k]for k in('path','sha256','bytes')},'original public source descriptor')
  oldraw=prior(artifact(oldref));need(digest(oldraw)=={k:oldref[k]for k in('sha256','bytes')},'original source bytes')
  raw=recorded(artifact(row),bundle['heldMaterials'][row['path']]);need(digest(raw)=={k:row[k]for k in('sha256','bytes')},'staged source byte pin')
  expected=AFTER if guest==HEADER else(oldref['sha256'],oldref['bytes']);need((row['sha256'],row['bytes'])==expected,'one header delta only')
  if guest==HEADER:need((oldref['sha256'],oldref['bytes'])==BEFORE,'original header before pin')
 fixtures=oi['publicFixtures'];need(len(fixtures)==7,'original seven fixture descriptors')
 for row in fixtures:need(digest(prior(s/row['relative']))=={k:row[k]for k in('sha256','bytes')},'all seven original fixture bytes')
 core=artifact(oi['newCore']);need((oi['newCore']['sha256'],oi['newCore']['bytes'])==CORE and digest(prior(core))=={'sha256':CORE[0],'bytes':CORE[1]},'qualified Core retained')
 rows=json.loads(prior(b/'compile_commands.json'));selected=[r for r in rows if r['file']=='/work/tests/capella/deposit_audit.cpp'];need(len(selected)==1 and selected[0]['directory']=='/outputs','unique existing CMake recipe')
 command=selected[0].get('arguments')or shlex.split(selected[0]['command']);need(command[0]=='/usr/bin/clang++'and command.count('-o')==command.count('-c')==1 and command.count('/work/tests/capella/deposit_audit.cpp')==1,'canonical compile argv')
 need(not any(x in command for x in('-M','-MM','-MD','-MMD','-MF','-MT','-MQ','-MP','-MJ','-save-temps','-ftime-trace')),'no secondary compile outputs')
 oldobj=command[command.index('-o')+1];command=[x.replace('/work/tests/capella/deposit_audit.cpp','/overlay/tests/capella/deposit_audit.cpp')for x in command];command[1:1]=['-DNANO_TEST_SOURCE="/work"','-iquote/work/tests/capella','-iquote/work/tests/support']
 linkraw=prior(artifact(oi['linkRecipe']));need(digest(linkraw)=={k:oi['linkRecipe'][k]for k in('sha256','bytes')},'original link recipe bytes')
 link=shlex.split(linkraw.decode());need(link[0]=='/usr/bin/clang++'and link.count(oldobj)==link.count('-o')==link.count('libnano_core.a')==1,'single object Core output slots')
 sdk={'/usr/bin/clang++','/usr/bin/ld','/bin/sh','/usr/bin/stat','/usr/bin/sha256sum','/usr/bin/readlink','/usr/bin/cat','/usr/bin/od','/usr/bin/env'};archives=asm=0
 for word in link:
  if word==oldobj:continue
  if word.endswith(('.a','.o')):need(not word.startswith('/'),'relative original archive object');prior(b/word);archives+=word.endswith('.a');asm+=word.endswith('.o')
  elif word.startswith('/usr/lib/')and '.so'in word:sdk.add(word)
 need(archives==5 and asm==2,'ARM original five archives two ASM')
 OUT=physical(c['controlRoot']);need(OUT.is_dir()and OUT.stat().st_uid==501 and stat.S_IMODE(OUT.stat().st_mode)==0o700 and not list(OUT.iterdir()),'NEW empty host0700 logs output')
 need(all(not OUT.is_relative_to(p)and not p.is_relative_to(OUT)for p in(s,b,overlay,physical(oi['overlayRoot']))),'outputs disjoint')
 exact(c['docker'],('path','sha256','bytes','material'),'Docker original utility');docker=artifact(c['docker']);need(digest(recorded(docker,c['docker']['material']))=={k:c['docker'][k]for k in('sha256','bytes')},'Docker utility full9 SHA');DOCKER_FILE=docker;DOCKER_RECORD=dict(c['docker']['material'])
 need(c['dockerEndpoint']=='unix:///Users/boris/.docker/run/docker.sock','explicit approved local Docker endpoint')
 prefix=c['containerPrefix'];need(re.fullmatch('nano-arm-tool-u64-[0-9a-f]{32}',prefix),'owned UUID namespace');volume=prefix+'-control';container=prefix+'-build';label='nano.arm.tool.u64';cid=None;volume_attempt=False;container_attempt=False;counter=0
 client=OUT/'docker-client';client.mkdir(mode=0o700);env={'PATH':'/usr/bin:/bin','HOME':str(client),'LANG':'C','LC_ALL':'C'}
 def dk(name,args,seconds=60,input_bytes=None,cleanup=False):
  nonlocal counter
  counter+=1;return run(str(counter)+'-'+name,[str(docker),'-H',c['dockerEndpoint'],'--config',str(client),*args],env,seconds,input_bytes,cleanup)
 def guest(name,args,seconds=120,input_bytes=None):
  return dk(name,['exec','--user','501:20',*(['-i']if input_bytes is not None else[]),cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C',*args],seconds,input_bytes)
 def snap(name,paths):
  paths=sorted(set(paths));need(0<len(paths)<5000 and all(re.fullmatch(r'/(?:usr|bin|lib|lib64|src/\.deps|control|overlay)/[A-Za-z0-9_+./:@=~-]+',p)for p in paths),'guest snapshot roots')
  raw,err=guest(name,['/bin/sh','-s','--',*paths],120,SDK_SCRIPT);need(not err,'guest snapshot stderr');lines=raw.decode().splitlines();need(len(lines)==6*len(paths),'six rows per guest FD')
  result={}
  for i,p in enumerate(paths):
   observed_name,resolved,alias,linktext,row,sha=lines[6*i:6*i+6];parts=row.split('|');apart=alias.split('|')
   need(observed_name==p and resolved.startswith(('/usr/','/bin/','/lib/','/lib64/','/src/.deps/','/control/','/overlay/'))and len(parts)==len(apart)==9 and re.fullmatch('[0-9a-f]{64}',sha),'guest FD snapshot shape')
   need(stat.S_ISREG(int(parts[2],16))and int(parts[3])>=1 and all(not re.search(r'[\r\n\x00]',v)for v in parts+apart+[resolved,linktext]),'guest FD regular nine fields')
   result[p]={'resolvedPath':resolved,'aliasMaterial':apart,'linkText':linktext,'material':parts,'sha256':sha,'bytes':int(parts[6])}
  publish(name+'-snapshot.private.json',result);return result
 def stable_guest(name,paths,expected=None):
  previous=None;count=0;since=0;until=min(DEADLINE,time.monotonic()+25);n=0
  while True:
   value=snap(name+'-'+str(n),paths);n+=1;now=time.monotonic()
   if expected is not None:need(value==expected,'held guest snapshot changed')
   if value==previous:count+=1
   else:previous=value;count=1;since=now
   if count>=3 and now-since>=2:return value
   need(now<until,'guest first admission bound');time.sleep(.75)
 def cleanup_owned():
  global CLEAN_DEADLINE
  nonlocal cid
  CLEAN_DEADLINE=time.monotonic()+60
  if container_attempt:
   if cid is None:
    raw,err=dk('cleanup-discover',['ps','-a','--no-trunc','--filter','name=^/'+container+'$','--format','{{.ID}}'],10,cleanup=True);need(not err and len(raw.splitlines())<=1,'owned cleanup discover')
    if raw.strip():cid=raw.decode().strip()
   if cid:
    need(re.fullmatch('[0-9a-f]{64}',cid),'owned cleanup full container ID')
    raw,err=dk('cleanup-inspect',['inspect',cid,'--format','{{json .}}'],10,cleanup=True);item=json.loads(raw);need(not err and item['Id']==cid and item['Name']=='/'+container and item['Image']==IMAGE and item['Config']['Labels'].get(label)==prefix,'owned full-ID cleanup guard')
    dk('cleanup-remove',['rm','-f',cid],15,cleanup=True)
   raw,err=dk('cleanup-absence',['ps','-a','--filter','name=^/'+container+'$','--format','{{.ID}}'],10,cleanup=True);need(not raw.strip()and not err,'owned container absent')
  if volume_attempt:
   raw,err=dk('cleanup-volume-discover',['volume','ls','--filter','name=^'+volume+'$','--format','{{.Name}}'],10,cleanup=True)
   if raw.strip():
    need(raw.decode().strip()==volume and not err,'owned volume lookup');raw,err=dk('cleanup-volume-inspect',['volume','inspect',volume,'--format','{{json .}}'],10,cleanup=True);v=json.loads(raw);need(not err and v['Name']==volume and v['Labels'].get(label)==prefix,'owned volume cleanup guard');dk('cleanup-volume-remove',['volume','rm',volume],10,cleanup=True)
   raw,err=dk('cleanup-volume-absent',['volume','ls','--filter','name=^'+volume+'$','--format','{{.Name}}'],10,cleanup=True);need(not raw.strip()and not err,'owned volume absent')
  publish('cleanup.json',{'ownedContainerCleanupAttempted':container_attempt,'ownedVolumeCleanupAttempted':volume_attempt,'candidatePassedBeforeCleanup':passed})
 passed=False;result=None;failure=None
 try:
  raw,err=dk('image-inspect',['image','inspect',IMAGE,'--format','{{json .}}']);im=json.loads(raw);need(not err and im['Id']==IMAGE and im['Architecture']=='arm64'and im['Os']=='linux','existing exact ARM image')
  raw,err=dk('container-absent',['ps','-a','--filter','name=^/'+container+'$','--format','{{.ID}}']);need(not raw.strip()and not err,'container absent before create')
  raw,err=dk('volume-absent',['volume','ls','--filter','name=^'+volume+'$','--format','{{.Name}}']);need(not raw.strip()and not err,'volume absent before create')
  volume_attempt=True
  raw,err=dk('volume-create',['volume','create','--label',label+'='+prefix,volume]);need(raw.decode().strip()==volume and not err,'owned new volume create')
  raw,err=dk('volume-owned-before-mount',['volume','inspect',volume,'--format','{{json .}}']);v=json.loads(raw);need(not err and v['Name']==volume and v['Labels'].get(label)==prefix,'owned labeled volume before any setup mutation')
  mounts=['--mount','type=bind,src='+str(s)+',dst=/work,readonly','--mount','type=bind,src='+str(b)+',dst=/outputs,readonly','--mount','type=bind,src='+str(core)+',dst=/qualified-core.a,readonly','--mount','type=volume,src='+volume+',dst=/control']
  # File binds avoid private Mac directory UID/0700 mappings. New 0444 leaves
  # must prove readable guest bytes; never assume guest UID or chown a host bind.
  for guest_path,row in sorted(sources.items()):mounts+=['--mount','type=bind,src='+row['path']+',dst='+guest_path+',readonly']
  raw,err=dk('container-absent-at-create',['ps','-a','--filter','name=^/'+container+'$','--format','{{.ID}}']);need(not raw.strip()and not err,'container absent immediately before owned create')
  container_attempt=True
  raw,err=dk('container-create',['create','--pull','never','--name',container,'--label',label+'='+prefix,'--network','none','--read-only','--cap-drop','ALL','--cap-add','CHOWN','--security-opt','no-new-privileges','--pids-limit','256','--memory','4g','--cpus','2','--user','0:0','--env','PATH=/usr/bin:/bin','--tmpfs','/tmp:rw,nosuid,nodev,size=134217728',*mounts,'--workdir','/outputs','--entrypoint','/bin/sh',IMAGE,'-ec','test -z "$(ls -A /control)"; chmod 700 /control; chown 501:20 /control; exec sleep 1050']);cid=raw.decode().strip();need(re.fullmatch('[0-9a-f]{64}',cid)and not err,'owned full container ID')
  raw,err=dk('container-start',['start',cid]);need(raw.decode().strip()==cid and not err,'container start')
  raw,err=guest('volume-owner',['/bin/sh','-ec','test "$(stat -c %u:%g:%a /control)" = 501:20:700; test -z "$(ls -A /control)"; printf "OWNED501:20:700\\n"']);need(raw==b'OWNED501:20:700\n'and not err,'actual Linux volume owner before CXX')
  raw,err=guest('readable-ro-inputs',['/bin/sh','-ec','test -r /work/tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json; test -r /outputs/compile_commands.json; test -r /qualified-core.a; printf "READABLE\\n"']);need(raw==b'READABLE\n'and not err,'old read-only roots readable by actual guest501')
  source_guest=stable_guest('source-before',sources)
  for name,row in source_guest.items():need(row['sha256']==sources[name]['sha256']and row['bytes']==sources[name]['bytes'],'effective guest overlay byte pin')
  base=list(command);i=base.index('-o');del base[i:i+2];base.remove('-c');raw,err=guest('M',base+['-M','-MT','arm-tool-u64']);need(not err,'M stderr');found=deps(raw,pathlib.Path('/outputs'));need(20<len(found)<5000 and set(sources).issubset(found),'real M reaches all nine overlays')
  for value in sorted(found):
   p=pathlib.PurePosixPath(value)
   if value in sources:recorded(artifact(sources[value]),bundle['heldMaterials'][sources[value]['path']])
   elif p.is_relative_to('/work'):prior(s/str(p.relative_to('/work')))
   elif p.is_relative_to('/outputs'):prior(b/str(p.relative_to('/outputs')))
   else:need(value.startswith(('/usr/','/lib/','/lib64/','/src/.deps/')),'M outside profile');sdk.add(value)
  sdk_before=stable_guest('sdk-before',sdk);fence()
  argv=list(command);argv[argv.index('-o')+1]='/control/deposit_audit.cpp.o';argv+=['-MD','-MT','arm-tool-u64','-MF','/control/deposit_audit.cpp.o.d'];raw,err=guest('CXX',argv,180);need(not err,'CXX stderr')
  raw,err=guest('MD',['/usr/bin/cat','/control/deposit_audit.cpp.o.d']);need(not err and deps(raw,pathlib.Path('/outputs'))==found,'genuine M equals actual MD')
  argv=['/control/deposit_audit.cpp.o'if x==oldobj else('/qualified-core.a'if x=='libnano_core.a'else x)for x in link];argv[argv.index('-o')+1]='/control/'+TARGET;raw,err=guest('LINK',argv,180);need(not err,'link stderr')
  products=stable_guest('products',['/control/deposit_audit.cpp.o','/control/deposit_audit.cpp.o.d','/control/'+TARGET])
  for row in products.values():need(row['material'][4:6]==['501','20']and row['material'][3]=='1','owned Linux output before test')
  raw,err=guest('ELF',['/bin/sh','-ec','/usr/bin/od -An -tx1 -N20 /control/deposit_audit.cpp.o; /usr/bin/od -An -tx1 -N20 /control/'+TARGET]);tokens=raw.decode().split();need(not err and len(tokens)==40,'ELF header rows')
  for j in(0,20):need(tokens[j:j+6]==['7f','45','4c','46','02','01']and tokens[j+18:j+20]==['b7','00'],'ARM ELF64 LE machine')
  need(tokens[16:18]==['01','00']and tokens[36:38]in(['02','00'],['03','00']),'ELF object/executable types')
  summaries=[]
  for name,selector,scenario,refusals,sigs in(('policy','--fresh-lifecycle-self-test','fresh-lifecycle-summary',12,3),('observe','--fresh-lifecycle-observe-self-test','fresh-lifecycle-observe-summary',9,4)):
   raw,err=guest(name,['/control/'+TARGET,selector],70);rows=[json.loads(x)for x in raw.decode().splitlines()];need(not err and len(rows)==4 and all(x['passed']is True for x in rows),'four exact CLI rows');summary=rows[-1]
   expected={'scenario':scenario,'passed':True,'groups':3,'expectedRefusals':refusals,'fixtureBlsSignatures':sigs,'operationalSignatures':0,'envRead':False,'databaseAccessed':False,'networkUsed':False,'runtimeStarted':False,'productionQualified':False};need(all(summary.get(k)==v for k,v in expected.items()),'typed exact CLI summary');summaries.append(summary)
  need(snap('sdk-after',sdk)==sdk_before and snap('products-after',products)==products and snap('source-after',sources)==source_guest,'same-container SDK source program FD nine SHA unchanged');fence()
  exported={}
  for guest_path,row in products.items():
   leaf=pathlib.PurePosixPath(guest_path).name;dest=OUT/leaf;need(not dest.exists(),'new host output only');raw,err=dk('copy-'+leaf,['cp',cid+':'+guest_path,str(dest)]);need(not raw and not err,'copy only new output')
   need(dest.lstat().st_uid==501 and dest.lstat().st_nlink==1,'new host copy owner link');os.chmod(dest,0o500 if leaf==TARGET else 0o400)
   fd=os.open(dest,os.O_RDONLY|os.O_NOFOLLOW)
   try:os.fsync(fd)
   finally:os.close(fd)
   fd=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
   try:os.fsync(fd)
   finally:os.close(fd)
   ref={'path':str(dest),'sha256':row['sha256'],'bytes':row['bytes']};data=first(ref);need(digest(data)=={k:ref[k]for k in('sha256','bytes')},'copy bytes equal held Linux producer');NEW[str(dest)]=digest(data);exported[leaf]=dict(ref,material=HELD[str(dest)])
  need(snap('products-after-copy',products)==products,'copy did not change Linux outputs');fence()
  admission=publish('new-software-admission.private.json',{'format':'nano-arm-tool-u64-new-software-admission-v1','imageId':IMAGE,'programs':exported,'guestProgramMaterials':products,'guestSdkMaterials':sdk_before,'guestSourceMaterials':source_guest,'guestDependencies':sorted(found),'sourceOverlayAdmission':c['overlayAdmission'],'originalObserveInput':c['oldObserveInput'],'originalObserveAdmissionPosthoc':c['oldObserveAdmission'],'historicalObserveAdmissionRawDigestRecovered':False,'oldAuthoritiesRefreshed':False,'newImageSdkNamespace':True,'guestSdkBeforeAfterEqual':True,'hostFiles':HELD,'productionQualified':False});first(dict(digest(material(admission)[1]),path=str(admission)))
  result={'format':'nano-arm-tool-u64-terminal-v1','passed':True,'owner':'mac-docker','imageId':IMAGE,'coreArchiveSha256':CORE[0],'sourceHeaderAfterSha256':AFTER[0],'consumerCompilations':1,'consumerLinks':1,'genuineMEqualsActualMd':True,'archiveInputs':5,'existingAsmObjects':2,'newCoreCompilations':0,'configureCommands':0,'imageBuilds':0,'imagePulls':0,'cliSelectors':2,'cliSummaries':summaries,'fixtureBlsSignatures':7,'operationalSignatures':0,'historicalObserveAdmissionRawDigestRecovered':False,'historicalSdkAuthorityRecovered':False,'newSameContainerSdkNineShaQualified':True,'oldAuthoritiesRefreshed':False,'softwareAdmission':digest(material(admission)[1]),'binary':digest(material(OUT/TARGET)[1]),'guestAndHostInodesCompared':False,'wholeLifecycleQualified':False,'httpExecTransportQualified':False,'withdrawalFullAmountPathQualified':False,'privateVpsAccessed':False,'runtimeStarted':False,'productionQualified':False};passed=True
 except BaseException as error:failure=error
 finally:
  try:cleanup_owned()
  except BaseException as cleanup_error:
   if failure is None:failure=cleanup_error
   try:publish('cleanup-refusal.private.json',{'errorType':type(cleanup_error).__name__,'firstFailureRetained':True,'detailsExported':False})
   except BaseException:pass
 if failure is not None:raise failure
 need(result is not None,'candidate absent');result.update(ownedContainerAbsent=True,ownedVolumeAbsent=True,events=EVENTS);fence();publish('terminal.json',result);print(json.dumps(result),flush=True)

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--inputs',required=True);parser.add_argument('--sha256',required=True);parser.add_argument('--bytes',required=True,type=int);args=parser.parse_args()
 def interrupted(*_):raise InterruptedError('ARM Tool controller stopped')
 signal.signal(signal.SIGTERM,interrupted);signal.signal(signal.SIGINT,interrupted)
 try:main(args)
 except BaseException as error:
  r={'format':'nano-arm-tool-u64-refused-v1','passed':False,'phase':PHASE,'errorType':type(error).__name__,'detailsExported':False,'runtimeStarted':False,'productionQualified':False,'events':EVENTS}
  if OUT is not None:
   try:publish('refusal.private.json',r)
   except BaseException:pass
  print(json.dumps(r),flush=True);raise SystemExit(1)
