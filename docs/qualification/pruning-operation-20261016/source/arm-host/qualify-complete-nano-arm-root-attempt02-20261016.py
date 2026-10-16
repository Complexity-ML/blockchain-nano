#!/usr/bin/env python3
"""Root-only thin ARM test build; all previously admitted products stay read-only."""
import argparse, hashlib, io, json, os, pathlib, shlex, signal, stat, subprocess, sys, tarfile, time, uuid
E=pathlib.Path(__file__).resolve().parent
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
OUT=E/'complete-nano-arm-root-qualification-attempt02-20261016'
PROPOSAL=E/'rpc-eip1898-simulation-source-proposal-20261016'
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
HEAD='2443a5c058c75361349daaf66645b62f489c2e6f'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
HELD={}
os.umask(0o077)
def interrupted(*_):raise RuntimeError('qualification interrupted')
for sig in (signal.SIGTERM,signal.SIGINT):signal.signal(sig,interrupted)

def material(p):
 p=pathlib.Path(p);assert p.is_absolute() and p.resolve()==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);assert stat.S_ISREG(a.st_mode) and a.st_nlink==1
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
  q=subprocess.run(argv,stdout=out,stderr=err,timeout=timeout)
 event={'step':name,'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-start)*1000)};save(OUT/(name+'-exit.json'),event)
 assert expected is None or q.returncode==expected
 return event

GUEST = None

def main(a):
 global GUEST
 GUEST=hold(E/'nano-current-core-linux-worker-source-20261016/worker.pl',{'sha256':'b1eb3cabeb5778b8876daf72cf85946ed839d2da56ecd49476cabdaaaf2dc581','bytes':15789}).decode()
 assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip()==HEAD
 assert subprocess.check_output(['git','status','--porcelain'],cwd=R)==b''
 OUT.mkdir(mode=0o700);(OUT/'source').mkdir();(OUT/'outputs').mkdir();(OUT/'controls').mkdir()
 source=OUT/'source'
 raw=subprocess.check_output(['git','archive',HEAD,'include','src','network','third_party','CMakeLists.txt'],cwd=R);expected={}
 with tarfile.open(fileobj=io.BytesIO(raw)) as tf:
  for item in tf:
   assert item.isfile() or item.isdir();rel=pathlib.Path(item.name);assert not rel.is_absolute() and '..' not in rel.parts
   if item.isdir():(source/rel).mkdir(parents=True,exist_ok=True)
   else:
    p=source/rel;p.parent.mkdir(parents=True,exist_ok=True);body=tf.extractfile(item).read();p.write_bytes(body);expected['/work/'+str(rel)]={'sha256':hashlib.sha256(body).hexdigest(),'bytes':len(body)}
 base=E/'fresh-arm-bootstrap-import-current-core-root-20261016';ad=json.loads(hold(base/'selected-descriptor-admission.private.json',{'sha256':'9a2caf6a466b7dd103c4100d29ed7bbe9df50f475bfd340d340ccb961ef1248a','bytes':1920}))
 current=json.loads(hold(base/'selected-current-core.private.json',ad['materials']['currentCoreAdmission']))
 closure=json.loads(hold(base/'selected-original-closure.private.json',ad['materials']['originalClosure']))
 oldroot=pathlib.Path(closure['buildRoot'])
 oldvol=json.loads(hold(E/'rpc-eip1898-arm-root-qualification-attempt02-20261016/owned-output-volume.private.json'));assert oldvol['image']==IMAGE and oldvol['volume'].startswith('nano-rpc-eip1898-') and oldvol['label'].startswith('nano.rpc.eip1898=')
 compilepath=oldroot/'compile_commands.json';database=json.loads(hold(compilepath,closure['files'][str(compilepath)]))
 def flags(ending):
  rows=[x for x in database if x['file'].endswith(ending)];assert len(rows)==1
  argv=shlex.split(rows[0]['command']);assert argv[-2]=='-c';argv=argv[:-2];assert argv[-2]=='-o';argv=argv[:-2]
  return [x.replace('/outputs/third_party','/old/third_party') for x in argv]
 mainflags=flags('/src/main.cpp')
 cmake=(source/'CMakeLists.txt').read_text();assert cmake.count('add_executable(nano src/main.cpp)')==1 and cmake.count('target_link_libraries(nano PRIVATE nano_core)')==1
 link=closure['linkArgv'];tail=link[link.index('-o')+2:];assert tail.count('libnano_core.a')==1
 tail=[('/baseline/libnano_core.a' if p=='libnano_core.a' else '/old/'+p if not p.startswith(('/','-')) else p) for p in tail]
 inputMounts=[]
 for guest in tail:
  if guest.startswith('/old/'):
   p=oldroot/guest[len('/old/'):];hold(p,closure['files'][str(p)]);row=HELD[str(p)];expected[guest]={'sha256':row['sha256'],'bytes':row['st_size']};inputMounts.extend(['--mount','type=bind,src='+str(p)+',dst='+guest+',readonly'])
 client=json.loads(hold(E/'fresh-linux-lifecycle-tool-u64-root-20261016/docker-client-first-admission.private.json'))['docker'];hold(pathlib.Path(client['path']),client['material9'])
 docker=[client['path'],'--host','unix:///Users/boris/.docker/run/docker.sock']
 name='nano-current-nano-arm-'+str(uuid.uuid4());label='nano.current.nano='+name;volume=name+'-outputs'
 run(docker+['volume','inspect',oldvol['volume']],'inspect-retained-qualified-volume',30)
 retained=json.loads((OUT/'inspect-retained-qualified-volume-stdout.private.log').read_bytes());labelkey,labelvalue=oldvol['label'].split('=',1);assert len(retained)==1 and retained[0]['Labels'].get(labelkey)==labelvalue
 probe="use Digest::SHA ();use JSON::PP ();open(my $f,'<','/baseline/libnano_core.a')or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);print JSON::PP->new->canonical->encode({sha256=>$d->hexdigest,bytes=>-s '/baseline/libnano_core.a'});"
 run(docker+['run','--rm','--name',name+'-read','--label',label,'--pull=never','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--mount','type=volume,src='+oldvol['volume']+',dst=/baseline,readonly','--entrypoint','/usr/bin/perl',IMAGE,'-e',probe],'known-core-byte-prime',30)
 corepin=json.loads((OUT/'known-core-byte-prime-stdout.private.log').read_bytes());assert corepin['sha256']=='e2127db4aa30c39fc0891eb22f008410898717ce75c7aff8d46a25d62345499d'
 controlBodies={'recipes.json':(json.dumps({'mainFlags':mainflags,'linkArgvTail':tail,'uid':501,'gid':20,'reuseMain':False},sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'core-pin.json':(json.dumps(corepin,sort_keys=True)+'\n').encode(),'worker.pl':GUEST.encode()}
 controlExpected={str(OUT/'controls'/name):{'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)} for name,raw in controlBodies.items()}
 for leaf,raw in controlBodies.items():
  with (OUT/'controls'/leaf).open('xb') as f:f.write(raw)
 # Only the NEW public-source projection changes modes. Old authorities stay untouched.
 for p in source.rglob('*'):os.chmod(p,0o555 if p.is_dir() else 0o444)
 os.chmod(source,0o555)
 for p in (OUT/'controls').iterdir():
  os.chmod(p,0o444)
 os.chmod(OUT/'controls',0o555)
 # The output is a NEW owned Linux volume, not a macOS0700 bind with VM uid mapping.
 run(docker+['volume','ls','--format','{{.Name}}'],'initial-volume-names',30)
 assert volume not in (OUT/'initial-volume-names-stdout.private.log').read_text().splitlines()
 run(docker+['ps','-a','--format','{{.Names}}'],'initial-container-names',30)
 assert not set([name,name+'-init',name+'-read']).intersection((OUT/'initial-container-names-stdout.private.log').read_text().splitlines())
 run(docker+['volume','create','--label',label,volume],'create-output-volume',30)
 assert (OUT/'create-output-volume-stdout.private.log').read_text().strip()==volume
 run(docker+['volume','inspect',volume],'inspect-owned-output-volume',30)
 owned=json.loads((OUT/'inspect-owned-output-volume-stdout.private.log').read_bytes());assert len(owned)==1 and owned[0]['Name']==volume and owned[0]['Labels'].get('nano.current.nano')==name
 common=['run','--label',label,'--rm','--pull=never','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges']
 init=docker+common+['--name',name+'-init','--user','0:0','--cap-add','CHOWN','--mount','type=volume,src='+volume+',dst=/outputs','--entrypoint','/bin/sh',IMAGE,'-ec','chmod 700 /outputs && chown 501:20 /outputs']
 args=docker+common+['-d','--name',name,'--cpus','2','--memory','3g','--pids-limit','128','--user','501:20','--tmpfs','/tmp:rw,nosuid,nodev,size=128m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=volume,src='+oldvol['volume']+',dst=/baseline,readonly','--mount','type=bind,src='+str(OUT/'controls')+',dst=/controls,readonly','--mount','type=volume,src='+volume+',dst=/outputs']+inputMounts+['--entrypoint','/bin/sh',IMAGE,'-ec','exec sleep 1100']
 fence();save(OUT/'original-inputs-before-prime.private.json',HELD)
 save(OUT/'owned-output-volume.private.json',{'volume':volume,'label':label,'image':IMAGE})
 try:
  run(init,'initialize-output-volume',30);run(args,'start-owned-container',30)
  prime=r"use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=[str(p) for p in sorted((OUT/'controls').iterdir())];guestNames=['/controls/'+pathlib.Path(p).name for p in names]
  run(docker+['exec','--user','501:20',name,'/usr/bin/perl','-e',prime,*guestNames],'input-bind-prime',30)
  actual=json.loads((OUT/'input-bind-prime-stdout.private.log').read_bytes());assert actual==[controlExpected[p] for p in names]
  observed=[];start=time.monotonic();previous=None;count=0;since=start
  while True:
   table={p:material(pathlib.Path(p))[0] for p in names};now=time.monotonic();fence()
   for p,row in table.items():assert row['sha256']==controlExpected[p]['sha256'] and row['st_size']==controlExpected[p]['bytes']
   observed.append({'elapsedMs':int((now-start)*1000),'table':table})
   if table==previous:count+=1
   else:count=1;since=now
   assert now-start<30
   if count>=3 and now-since>=2:break
   previous=table;time.sleep(1)
  for p,row in table.items():hold(pathlib.Path(p),row)
  save(OUT/'new-control-first-admission.private.json',{'observations':observed,'knownBytesMatched':True,'oldAuthoritiesRefreshed':False});save(OUT/'original-inputs.private.json',HELD)
  workerEvent=run(docker+['exec','--user','501:20',name,'/usr/bin/perl','/controls/worker.pl'],'docker-qualification',timeout=600,expected=None)
  reader=r"use strict;use warnings;opendir(my $d,'/outputs')or die;my @f=sort grep { /\.(?:json|private\.log|d)\z/ && -f ('/outputs/'.$_) && !-l ('/outputs/'.$_) && (-s ('/outputs/'.$_))<16777216 } readdir($d);closedir($d);exec '/usr/bin/tar','-cf','-','-C','/outputs','--',@f;die;"
  with (OUT/'logs-transport.private.tar').open('xb') as f:
   q=subprocess.run(docker+['exec','--user','501:20',name,'/usr/bin/perl','-e',reader],stdout=f,stderr=subprocess.PIPE,timeout=60)
  assert q.returncode==0
  with tarfile.open(OUT/'logs-transport.private.tar') as tf:
   for item in tf:
    assert item.isfile() and pathlib.Path(item.name).name==item.name and item.size<16777216
    with (OUT/'outputs'/item.name).open('xb') as f:f.write(tf.extractfile(item).read())
  assert workerEvent['exitCode']==0
 finally:
  for sig in (signal.SIGTERM,signal.SIGINT):signal.signal(sig,signal.SIG_IGN)
  q=subprocess.run(docker+['ps','-a','--filter','label='+label,'--format','{{.Names}}'],capture_output=True,timeout=30);assert q.returncode==0;names=q.stdout.decode().split();assert set(names)<=set([name,name+'-init',name+'-read'])
  for target in names:
   q=subprocess.run(docker+['inspect',target],check=True,capture_output=True,timeout=15);owned=json.loads(q.stdout);assert len(owned)==1 and owned[0]['Name']=='/'+target and owned[0]['Config']['Labels'].get('nano.current.nano')==name and owned[0]['Config']['Image']==IMAGE and owned[0]['Image']==IMAGE
   subprocess.run(docker+['rm','-f',target],check=True,capture_output=True,timeout=15)
  q=subprocess.run(docker+['ps','-a','--filter','label='+label,'--format','{{.ID}}'],check=True,capture_output=True,timeout=15);assert not q.stdout.strip()
  save(OUT/'cleanup-public.json',{'ownedContainerAbsent':True,'ownedOutputVolumeRetained':True,'readerWasSameOwnerReadOnlyCode':True,'imagesBuilt':0,'originalBuildRootsReadOnly':True})
 fence();result=json.loads((OUT/'outputs/result-public.json').read_bytes());assert json.loads((OUT/'docker-qualification-stdout.private.log').read_bytes())==result;assert result['passed'] is True and result['productionQualified'] is False;result['originalInputsFullNineAndShaUnchanged']=True;result['sourceRevision']=HEAD;result['linkTailDerivation']='retained-canonical-nano-core-consumer-order-with-current-Nano-target-declaration';save(OUT/'result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 p=argparse.ArgumentParser()
 first_error=None
 try:main(p.parse_args())
 except BaseException as exc:first_error=exc;raise
 finally:
  try:
   fence()
   if OUT.is_dir():save(OUT/'originals-final-fence-public.json',{'originalInputsFullNineAndShaUnchanged':True,'heldFiles':len(HELD),'mainFailurePreserved':first_error is not None})
  except BaseException as post_error:
   if OUT.is_dir() and not (OUT/'originals-final-fence-failure-public.json').exists():save(OUT/'originals-final-fence-failure-public.json',{'originalInputsFullNineAndShaUnchanged':False,'errorType':type(post_error).__name__,'firstFailurePreserved':first_error is not None})
   if first_error is None:raise
