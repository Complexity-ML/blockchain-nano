#!/usr/bin/env python3
"""Root-only thin AMD test build; all previously admitted products stay read-only."""
import argparse, hashlib, io, json, os, pathlib, shlex, signal, stat, subprocess, sys, tarfile, time, uuid
E=pathlib.Path(__file__).resolve().parent
R=pathlib.Path('/home/ubuntu/blockchain-nano-git')
OUT=E/'attempt01'
BASE=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
HEAD='1f6c0d0453156fd308de3f327fbf987b66e3d3bb'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
HELD={}
os.umask(0o077)
def interrupted(*_):raise RuntimeError('qualification interrupted')
for sig in (signal.SIGTERM,signal.SIGINT):signal.signal(sig,interrupted)

def material(p):
 p=pathlib.Path(p);assert p.is_absolute() and p.resolve()==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);assert stat.S_ISREG(a.st_mode) and a.st_nlink>=1
  row={k:getattr(a,k) for k in F};parts=[]
  while True:
   b=os.read(fd,1048576)
   if not b:break
   parts.append(b)
  raw=b''.join(parts);assert row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(p.lstat(),k) for k in F}
  return dict(row,sha256=hashlib.sha256(raw).hexdigest()),raw
 finally:os.close(fd)

def hold(p,pin=None):
 p=pathlib.Path(p);row,raw=material(p)
 if pin:
  if 'st_dev' in pin:assert row==pin
  else:assert row['sha256']==pin['sha256'] and row['st_size']==pin['bytes']
 assert str(p) not in HELD or HELD[str(p)]==row
 HELD[str(p)]=row;return raw

def fence():
 for p,row in HELD.items():assert material(pathlib.Path(p))[0]==row

def save(p,z):
 with p.open('x') as f:json.dump(z,f,sort_keys=True);f.write('\n')

def run(argv,name,timeout=180,expected=0):
 save(OUT/(name+'-argv.private.json'),argv)
 start=time.monotonic()
 with (OUT/(name+'-stdout.private.log')).open('xb') as out,(OUT/(name+'-stderr.private.log')).open('xb') as err:
  q=subprocess.run(argv,stdout=out,stderr=err,timeout=timeout,env=ENV)
 event={'step':name,'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-start)*1000)};save(OUT/(name+'-exit.json'),event)
 assert expected is None or q.returncode==expected
 return event

ENV={'PATH':'/usr/bin:/bin','HOME':'/home/ubuntu','LANG':'C','LC_ALL':'C','GIT_TERMINAL_PROMPT':'0'}

def old_pin(row):
 assert set(row)=={'path','material','sha256','bytes'} and len(row['material'])==9
 r={k:int(v) for k,v in zip(F,row['material'])};assert r['st_size']==row['bytes'];return dict(r,sha256=row['sha256'])

def main():
 assert sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000
 assert subprocess.check_output(['/usr/bin/git','rev-parse','HEAD'],cwd=R,env=ENV).decode().strip()==HEAD
 assert subprocess.check_output(['/usr/bin/git','status','--porcelain'],cwd=R,env=ENV)==b''
 OUT.mkdir(mode=0o700);(OUT/'source').mkdir();(OUT/'outputs').mkdir(mode=0o700);(OUT/'controls').mkdir();(OUT/'docker-client').mkdir(mode=0o700)
 references={
 'artifacts':('control06/new-artifacts-first-admission.private.json','4e9ff3b48b05c78e3fec34ac3f02f6c2088ae73d4b20e5a5ea493c496749f906',4945),
 'links':('control06/actual-consumer-links.private.json','55ff7525510d2bad38cddd387157d3e9f4eeb1397e66f1712bde4e718a8b5b53',654679),
 'archives':('control06/actual-linked-new-archives.private.json','3a4f182a5e882a4f3713122ac79b781d94dbb8b87fd1a93cf90f14bfa18af5fb',6182185),
 'llvm':('control06/portable-llvm-producers.private.json','ccc3bd6d6450ab0da86cc9d8afd991d1366f1b90898383872374daf4c6e212a1',3273)}
 docs={k:json.loads(hold(BASE/rel,{'sha256':sha,'bytes':n})) for k,(rel,sha,n) in references.items()}
 coreRow=docs['archives']['/outputs/libnano_core.a']['archive'];core=pathlib.Path(coreRow['path']);assert core==BASE/'core/libnano_core.a' and coreRow==docs['artifacts']['files'][str(core)]
 assert coreRow['sha256']=='3816f7723c061c9b3d5f5d13a335690dda775a7a50a4fb868b8d27fe08c630ff' and coreRow['bytes']==40815352
 hold(core,old_pin(coreRow))
 source=OUT/'source';expected={}
 raw=subprocess.check_output(['/usr/bin/git','archive',HEAD,'include','src','network','third_party','CMakeLists.txt','tests/test_rpc_simulation_block_id.cpp'],cwd=R,env=ENV)
 with tarfile.open(fileobj=io.BytesIO(raw)) as tf:
  for item in tf:
   assert item.isfile() or item.isdir();rel=pathlib.Path(item.name);assert not rel.is_absolute() and '..' not in rel.parts
   if item.isdir():(source/rel).mkdir(parents=True,exist_ok=True)
   else:
    p=source/rel;p.parent.mkdir(parents=True,exist_ok=True);body=tf.extractfile(item).read();p.write_bytes(body);expected['/work/'+str(rel)]={'sha256':hashlib.sha256(body).hexdigest(),'bytes':len(body)}
 for rel,sha,n in [('src/rpc.cpp','ebd21d4a0372feacfd0cdbfea7d5b7a242fe591dc8d28080870df9371c908818',83044),('tests/test_rpc_simulation_block_id.cpp','73b860603901249bcc9b3250929beeac989df9496af5df1b8a0242b163d96df1',12238),('CMakeLists.txt','a82279eaadb5661b98800fa746b51104ca8c5d32d673324581dc58727c3d0b45',174639)]:assert expected['/work/'+rel]=={'sha256':sha,'bytes':n}
 # Join retained producer byte pins to this Git projection, without reading or
 # refreshing the old source/SDK tables. Only rpc.cpp is intentionally replaced.
 retainedProjects={}
 objects=docs['archives']['/outputs/libnano_core.a']['objects'];assert len(objects)==143
 for obj in objects.values():
  for path,row in obj['newSourceAndGeneratedDependencies'].items():
   if path.startswith('/work/'):
    pin={k:row[k] for k in ('sha256','bytes')};assert path not in retainedProjects or retainedProjects[path]==pin;retainedProjects[path]=pin
 for path,pin in retainedProjects.items():
  assert pin==({'sha256':'39f57f4bb015e7fac5519a50a1c6a68af9884ef3c59ee67e75c8e5ba1c186d6a','bytes':81499} if path=='/work/src/rpc.cpp' else expected[path])
 original=hold(R/'docs/qualification/rpc-eip1898-simulation-20261016/source/worker/worker.pl',{'sha256':'5c45bb5a8c65fd9d2cdd491c4ebb6ccc57d715adad938d0e948f5cf722806527','bytes':14145}).decode()
 changes=[('$>==501&&$<==501','$>==1000&&$<==1000'),('$s[4]==501&&$s[5]==20','$s[4]==1000&&$s[5]==1000'),('nano-rpc-eip1898-arm-thin-qualification-v1','nano-rpc-eip1898-amd-thin-qualification-v1')]
 for before,after in changes:assert original.count(before)==1;original=original.replace(before,after)
 worker=original.encode();assert hashlib.sha256(worker).hexdigest()=='33878e695b2661be97fc094d01337801821ee9d1290c5c4024aa27f342426bda' and len(worker)==14150
 rec=docs['archives']['/outputs/libnano_core.a']['objects']['/outputs/CMakeFiles/nano_core.dir/src/rpc.cpp.o']['compile'];assert rec['file']=='/work/src/rpc.cpp' and rec['directory']=='/outputs'
 flags=shlex.split(rec['command']);assert flags[-2:] == ['-c','/work/src/rpc.cpp'] and flags[-4]=='-o';flags=flags[:-4];assert flags[0]=='/usr/bin/clang++'
 flags=[x.replace('/outputs/third_party','/old/third_party') for x in flags]
 link=docs['links']['nano-capella-deposit-audit']['linkRecipe']['argv'];tail=link[link.index('-o')+2:];assert tail.count('libnano_core.a')+tail.count('/outputs/libnano_core.a')==1
 mounted=[];newtail=[]
 for word in tail:
  guest='/outputs/'+word if not word.startswith(('/','-')) else word
  if guest=='/outputs/libnano_core.a':newtail.append('/outputs/libnano_core.a');continue
  if guest.startswith('/outputs/'):
   row=docs['archives'][guest]['archive'] if guest in docs['archives'] else docs['llvm'][guest]['object']
   p=pathlib.Path(row['path']);assert p.is_relative_to(BASE/'core');hold(p,old_pin(row));dest='/old/'+guest[len('/outputs/'):]
   expected[dest]={'sha256':row['sha256'],'bytes':row['bytes']};mounted.extend(['--mount','type=bind,src='+str(p)+',dst='+dest+',readonly']);newtail.append(dest)
  else:
   assert guest.startswith(('/usr/','/lib/','-'));newtail.append(guest)
 assert len(mounted)==12
 controls={'recipes.json':(json.dumps({'coreFlags':flags,'testFlags':flags,'linkTail':newtail},sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'worker.pl':worker}
 # New sources/control bytes only. Original retained files are never chmodded.
 for p in source.rglob('*'):os.chmod(p,0o555 if p.is_dir() else 0o444)
 os.chmod(source,0o555)
 for leaf,body in controls.items():
  with (OUT/'controls'/leaf).open('xb') as f:f.write(body)
  os.chmod(OUT/'controls'/leaf,0o444)
 os.chmod(OUT/'controls',0o555)
 tools=[pathlib.Path('/usr/bin/docker'),pathlib.Path('/usr/bin/sudo'),pathlib.Path('/usr/bin/git')]
 observations=[];previous=None;count=0;start=time.monotonic();since=start
 while True:
  table={str(p):material(p)[0] for p in tools};now=time.monotonic();fence();observations.append({'elapsedMs':int((now-start)*1000),'table':table})
  if table==previous:count+=1
  else:count=1;since=now
  assert now-start<30
  if count>=3 and now-since>=2:break
  previous=table;time.sleep(1)
 for p,row in table.items():hold(pathlib.Path(p),row)
 save(OUT/'new-os-tool-first-admission.private.json',{'observations':observations,'oldAuthoritiesRefreshed':False})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(OUT/'docker-client')]
 name='nano-rpc-eip1898-amd-'+str(uuid.uuid4());label='nano.rpc.eip1898='+name
 run(docker+['ps','-a','--format','{{.Names}}'],'initial-container-names',30);assert name not in (OUT/'initial-container-names-stdout.private.log').read_text().splitlines()
 args=docker+['run','-d','--rm','--pull=never','--name',name,'--label',label,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--user','1000:1000','--tmpfs','/tmp:rw,nosuid,nodev,size=128m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(OUT/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(OUT/'outputs')+',dst=/outputs']+mounted+['--entrypoint','/bin/sh',IMAGE,'-ec','exec sleep 1100']
 save(OUT/'original-inputs-before-start.private.json',HELD);save(OUT/'owned-container.private.json',{'name':name,'label':label,'image':IMAGE})
 try:
  run(args,'start-owned-container',30)
  prime=r"use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=sorted(controls);run(docker+['exec','--user','1000:1000',name,'/usr/bin/perl','-e',prime,*['/controls/'+n for n in names]],'input-bind-prime',30)
  assert json.loads((OUT/'input-bind-prime-stdout.private.log').read_bytes())==[{'sha256':hashlib.sha256(controls[n]).hexdigest(),'bytes':len(controls[n])} for n in names]
  observed=[];previous=None;count=0;start=time.monotonic();since=start
  while True:
   table={str(OUT/'controls'/n):material(OUT/'controls'/n)[0] for n in names};now=time.monotonic();fence();observed.append({'elapsedMs':int((now-start)*1000),'table':table})
   for n in names:assert table[str(OUT/'controls'/n)]['sha256']==hashlib.sha256(controls[n]).hexdigest() and table[str(OUT/'controls'/n)]['st_size']==len(controls[n])
   if table==previous:count+=1
   else:count=1;since=now
   assert now-start<30
   if count>=3 and now-since>=2:break
   previous=table;time.sleep(1)
  for p,row in table.items():hold(pathlib.Path(p),row)
  save(OUT/'new-control-first-admission.private.json',{'observations':observed,'knownBytesMatched':True,'oldAuthoritiesRefreshed':False});save(OUT/'original-inputs.private.json',HELD)
  event=run(docker+['exec','--user','1000:1000',name,'/usr/bin/perl','/controls/worker.pl'],'docker-qualification',600,expected=None);assert event['exitCode']==0
 finally:
  for sig in (signal.SIGTERM,signal.SIGINT):signal.signal(sig,signal.SIG_IGN)
  q=subprocess.run(docker+['ps','-a','--filter','label='+label,'--format','{{.Names}}'],capture_output=True,timeout=30,env=ENV);assert q.returncode==0;names=q.stdout.decode().split();assert set(names)<=set([name])
  for target in names:
   q=subprocess.run(docker+['inspect',target],check=True,capture_output=True,timeout=15,env=ENV);owned=json.loads(q.stdout);assert len(owned)==1 and owned[0]['Name']=='/'+target and owned[0]['Config']['Labels'].get('nano.rpc.eip1898')==name and owned[0]['Config']['Image']==IMAGE and owned[0]['Image']==IMAGE
   subprocess.run(docker+['rm','-f',target],check=True,capture_output=True,timeout=15,env=ENV)
  q=subprocess.run(docker+['ps','-a','--filter','label='+label,'--format','{{.ID}}'],check=True,capture_output=True,timeout=15,env=ENV);assert not q.stdout.strip()
  save(OUT/'cleanup-public.json',{'ownedContainerAbsent':True,'ownedOutputDirectoryRetained':True,'imagesBuilt':0,'originalBuildRootsReadOnly':True,'oldSdkMapsRead':False})
 fence();result=json.loads((OUT/'outputs/result-public.json').read_bytes());assert json.loads((OUT/'docker-qualification-stdout.private.log').read_bytes())==result and result['passed'] is True and result['productionQualified'] is False and result['targetedTestSummary']['checks']==120;result['originalInputsFullNineAndShaUnchanged']=True;result['sourceRevision']=HEAD;result['retainedProducerProjectBytePinsMatchedExceptReplacedRpc']=len(retainedProjects);result['oldSdkMapsRead']=False;save(OUT/'result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 first_error=None
 try:main()
 except BaseException as exc:first_error=exc;raise
 finally:
  try:
   fence()
   if OUT.is_dir():save(OUT/'originals-final-fence-public.json',{'originalInputsFullNineAndShaUnchanged':True,'heldFiles':len(HELD),'mainFailurePreserved':first_error is not None})
  except BaseException as post_error:
   if OUT.is_dir() and not (OUT/'originals-final-fence-failure-public.json').exists():save(OUT/'originals-final-fence-failure-public.json',{'originalInputsFullNineAndShaUnchanged':False,'errorType':type(post_error).__name__,'firstFailurePreserved':first_error is not None})
   if first_error is None:raise
