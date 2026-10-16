#!/usr/bin/env python3
"""Root only: one current RPC TU/test; reuse qualified ARM Core, six library files and image."""
import pathlib,types,hashlib,json,os,tarfile,subprocess,time,signal,uuid,sys,resource,stat
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');HEAD='583c58952efde5ccbc033750a06d5a6f7ae056bc';REV='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
O=E/'pruning-rpc-arm-root-qualification-20261016-01';A=E/'current-arm-helpers-root-attempt02-20261016';P=E/'qualify-complete-nano-arm-root-attempt02-20261016.py'
b=P.read_bytes();assert hashlib.sha256(b).hexdigest()=='a73f0267359a4ac155b5a63a005385930b558a3408343622e8909931ffe92442';m=types.ModuleType('root_rpc_arm');m.__file__=str(P);exec(compile(b,str(P),'exec'),m.__dict__);m.OUT=O
IMAGE=m.IMAGE;CREATED=False;NAME=None;LABEL=None;VOLUME=None
TREES={'include':'73572ff29b1dea911e79292d65bf1381ffb86018','src':'203a093354f3906e1c7449a5be8a8a3d47c32bb5','network':'8c56a9d154e25ecdf0f72d25cc403eb152c3f328','third_party':'9ff7872a2174571f645aa5d61b037c947af339c6','tests/support':'a3978f97a17ab00da25512b1687247dfcaeaa50d','tests/capella':'0ff1c69d8eb4c27e722c3ff77370d5a3a6c211aa','tests/multihost':'8577c917bec892be4ffac8d36d02eea72cff22c0'}
SELECTED=[*TREES,'CMakeLists.txt','tests/test_rpc_simulation_block_id.cpp'];TU={'sha256':'b8e7625d0dc3b34c2156a5e62180630442edc49cef922bf703bbb9e5a54fe491','bytes':26990};CORE={'sha256':'e2127db4aa30c39fc0891eb22f008410898717ce75c7aff8d46a25d62345499d','bytes':41053616}
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def material(p):
 p=pathlib.Path(p);assert p.is_absolute() and p.resolve()==p;fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  st=os.fstat(fd);assert stat.S_ISREG(st.st_mode) and st.st_nlink==1 and 0<=st.st_size<=536870912;row={k:getattr(st,k)for k in m.F};parts=[];total=0
  while True:
   z=os.read(fd,min(1048576,536870913-total))
   if not z:break
   total+=len(z);assert total<=st.st_size;parts.append(z)
  assert total==st.st_size and row=={k:getattr(os.fstat(fd),k)for k in m.F}=={k:getattr(p.lstat(),k)for k in m.F};b=b''.join(parts);return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)
m.material=material
ENV={'PATH':'/usr/bin:/bin','HOME':'/Users/boris','LANG':'C','LC_ALL':'C','GIT_OPTIONAL_LOCKS':'0','GIT_NO_REPLACE_OBJECTS':'1','GIT_NO_LAZY_FETCH':'1','GIT_TERMINAL_PROMPT':'0'}
def git(*args):return subprocess.check_output(['/usr/bin/git','-C',str(R),*args],env=ENV,timeout=30)
def js(p,known=None):return json.loads(m.hold(p,known))
def stable(paths,known,label):
 start=time.monotonic();prev=None;count=0;since=start;observations=[]
 while True:
  table={str(p):material(p)[0]for p in paths};now=time.monotonic()
  for p,z in table.items():assert {'sha256':z['sha256'],'bytes':z['st_size']}==known[p]
  observations.append({'elapsedMs':int((now-start)*1000),'table':table});m.fence()
  if table==prev:count+=1
  else:count=1;since=now
  assert now-start<30
  if count>=3 and now-since>=2:break
  prev=table;time.sleep(1)
 for p,z in table.items():m.hold(pathlib.Path(p),z)
 m.save(O/(label+'.private.json'),{'observations':observations,'knownBytesMatched':True,'oldAuthoritiesRefreshed':False});return table

def main():
 global CREATED,NAME,LABEL,VOLUME
 assert len(sys.argv)==1 and os.getuid()==501 and os.getgid()==20
 assert git('rev-parse','HEAD').decode().strip()==HEAD and git('status','--porcelain')==b''
 for path,oid in TREES.items():assert git('rev-parse',HEAD+':'+path).decode().strip()==oid==git('rev-parse',REV+':'+path).decode().strip()
 wb=m.hold(E/'pruning-rpc-linux-root-source02-20261016/worker.pl',{'sha256':'61c3f64d8b0b8cc8754a515789f9bade225ff8a3aca67fa5f17e81ca38ed8eef','bytes':36051})
 oldresult=js(A/'result-public.json',{'sha256':'5bf63c1289a9b83bc3a9988d171c1071614111754324a5296a9599336178bfcf','bytes':7375})
 binding=js(A/'outputs/software-admission.private.json',{'sha256':oldresult['softwareAdmissionSha256'],'bytes':oldresult['softwareAdmissionBytes']})
 pr=binding['guestMaterials']['/controls/recipes.json'];recipes=js(A/'controls/recipes.json',{'sha256':pr['sha256'],'bytes':pr['bytes']});template=next(x for x in recipes['programs']if x['role']=='lifecycleSubmit');assert not any('NANO_'in x for x in template['compileFlags'])
 origin=E/'fresh-arm-bootstrap-import-current-core-root-20261016';descriptor=js(origin/'selected-descriptor-admission.private.json',{'sha256':'9a2caf6a466b7dd103c4100d29ed7bbe9df50f475bfd340d340ccb961ef1248a','bytes':1920});closure=js(origin/'selected-original-closure.private.json',descriptor['materials']['originalClosure']);host=closure['files'];old={g:z for g,z in binding['guestMaterials'].items()if g.startswith('/old/')};assert len(old)==6;mounts=[];oldexpected={}
 for guest,z in old.items():
  rows=[(p,x)for p,x in host.items()if x.get('sha256')==z['sha256']and x.get('st_size')==z['bytes']];assert len(rows)==1;p,x=rows[0];body=m.hold(pathlib.Path(p),x);assert pin(body)=={'sha256':z['sha256'],'bytes':z['bytes']};mounts+=['--mount','type=bind,src='+p+',dst='+guest+',readonly'];oldexpected[guest]=pin(body)
 core=A/'runtime-programs/libnano_core.a';ad=js(A/'new-runtime-bundle-first-admission.private.json',{'sha256':'c73a03b0adb9aac1e51799c621fe98ec1f3d034396b95a626ad68346ad3417d5','bytes':12657});assert pin(m.hold(core,ad['observations'][-1]['table'][str(core)]))==CORE
 # Only a selected executable row from Root's separately admitted fresh tool control.
 cp=js(E/'campaign-resource-guard-root-control-20261016/probe-only-config.private.json',{'sha256':'fed2abc6d9fd4107be44b4a40e767c05745365515bbc3df35b81cfaa8d4d9209','bytes':2490})['tools']['docker'];row={k:int(v)for k,v in zip(m.F,cp['material'])};row['sha256']=cp['sha256'];m.hold(pathlib.Path(cp['path']),row);docker=[cp['path'],'--host','unix:///Users/boris/.docker/run/docker.sock']
 O.mkdir(mode=0o700);CREATED=True
 for leaf in ['source','controls','outputs']:(O/leaf).mkdir(mode=0o700)
 source=O/'source';expected=dict(oldexpected);wanted={}
 for entry in git('ls-tree','-r','-z',HEAD,'--',*SELECTED).split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t');mode,kind,oid=meta.decode().split();key=path.decode();assert mode in ['100644','100755']and kind=='blob';wanted[key]=oid
 def limit():resource.setrlimit(resource.RLIMIT_FSIZE,(536870912,536870912))
 with(O/'source.private.tar').open('xb')as f:
  q=subprocess.run(['/usr/bin/git','-C',str(R),'archive',HEAD,*SELECTED],stdout=f,stderr=subprocess.PIPE,env=ENV,timeout=60,preexec_fn=limit);assert q.returncode==0 and not q.stderr
 seen=set();empty=set()
 with tarfile.open(O/'source.private.tar','r:')as tf:
  for z in tf:
   rel=pathlib.PurePosixPath(z.name);assert not rel.is_absolute()and'..'not in rel.parts and(z.isdir()or z.isfile());p=source/str(rel)
   if z.isdir():p.mkdir(parents=True,exist_ok=True);continue
   assert str(rel)in wanted and str(rel)not in seen and z.size<=33554432;body=tf.extractfile(z).read(z.size+1);assert len(body)==z.size and hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()==wanted[str(rel)];seen.add(str(rel));p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(body)
   if body:expected['/work/'+str(rel)]=pin(body)
   else:empty.add('/work/'+str(rel))
 assert seen==set(wanted)and empty=={f'/work/third_party/mcl/{v}/.emptydir'for v in ['bin','lib','obj']};assert expected['/work/tests/test_rpc_simulation_block_id.cpp']==TU
 cmake_raw=(source/'CMakeLists.txt').read_bytes();assert pin(cmake_raw)=={'sha256':'f13f5ade8f691aa50c60a28d427548af35c829b1c07db57e4fe4717618c7fd2f','bytes':175191}
 add=b'add_test(NAME rpc_pruning_retention_boundaries COMMAND nano-rpc-simulation-block-id-tests --pruning-boundary-self-test)\nset_tests_properties(rpc_pruning_retention_boundaries PROPERTIES TIMEOUT 45 LABELS "qualification_rpc_pruning_retention")\n';assert cmake_raw.count(add)==1 and cmake_raw.replace(add,b'')==git('show','92b5edf385682b51517ea3a6e008a99831e7016c:CMakeLists.txt')
 cmake=(source/'CMakeLists.txt').read_text();assert 'add_executable(nano-rpc-simulation-block-id-tests tests/test_rpc_simulation_block_id.cpp)'in cmake and 'target_link_libraries(nano-rpc-simulation-block-id-tests PRIVATE nano_core)'in cmake
 program=dict(template,role='rpcSimulationTest',source='/work/tests/test_rpc_simulation_block_id.cpp',reuse=None)
 recipe={'format':'nano-current-linux-rpc-test-recipes-v1','owner':'mac-docker','uid':501,'gid':20,'imageId':IMAGE,'elfMachine':183,'sourceRevision':HEAD,'sourceTreeEntries':[{'path':p,'oid':v}for p,v in TREES.items()],'programs':[program]}
 bodies={'worker.pl':wb,'recipes.json':(json.dumps(recipe,sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'core-pin.json':(json.dumps(CORE,sort_keys=True)+'\n').encode()}
 for leaf,b in bodies.items():(O/'controls'/leaf).write_bytes(b)
 for p in sorted(source.rglob('*'),key=lambda p:len(p.parts),reverse=True):os.chmod(p,0o555 if p.is_dir()else 0o444)
 os.chmod(source,0o555)
 for p in(O/'controls').iterdir():os.chmod(p,0o444)
 os.chmod(O/'controls',0o555)
 NAME='nano-rpc-pruning-arm-'+uuid.uuid4().hex;LABEL='nano.rpc.current='+NAME;VOLUME=NAME+'-outputs'
 def dk(label,args,seconds=30,expected=0):return m.run(docker+args,label,seconds,expected)
 def read(label):return(O/(label+'-stdout.private.log')).read_bytes()
 dk('image-inspect',['image','inspect',IMAGE,'--format','{{json .}}']);image=json.loads(read('image-inspect'));assert image['Id']==IMAGE and image['Architecture']=='arm64'and image['Os']=='linux'
 dk('initial-name',['ps','-a','--filter','name=^/'+NAME+'(-init)?$','--format','{{.ID}}']);assert read('initial-name').strip()==b''
 dk('initial-volume',['volume','ls','--format','{{.Name}}']);assert VOLUME not in read('initial-volume').decode().splitlines()
 dk('create-volume',['volume','create','--label',LABEL,VOLUME]);assert read('create-volume').decode().strip()==VOLUME
 m.save(O/'owned-output-volume.private.json',{'volume':VOLUME,'label':LABEL,'image':IMAGE})
 first=None
 try:
  dk('inspect-owned-volume',['volume','inspect',VOLUME]);v=json.loads(read('inspect-owned-volume'));assert len(v)==1 and v[0]['Name']==VOLUME and v[0]['Labels'].get('nano.rpc.current')==NAME
  common=['--pull=never','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges']
  dk('init-owned-volume',['run','--rm','--name',NAME+'-init','--label',LABEL,*common,'--user','0:0','--cap-add','CHOWN','--mount','type=volume,src='+VOLUME+',dst=/outputs,volume-nocopy','--entrypoint','/bin/sh',IMAGE,'-ec','chmod 700 /outputs && chown 501:20 /outputs'])
  dk('start-owned-container',['run','-d','--rm','--name',NAME,'--label',LABEL,*common,'--cpus','2','--memory','3g','--pids-limit','128','--user','501:20','--tmpfs','/tmp:rw,nosuid,nodev,size=128m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(O/'controls')+',dst=/controls,readonly','--mount','type=volume,src='+VOLUME+',dst=/outputs,volume-nocopy',*mounts,'--workdir','/outputs','--entrypoint','/bin/sh',IMAGE,'-ec','exec sleep 900'])
  modeprobe=r"use Fcntl qw(:mode);my @s=lstat('/outputs');die unless @s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==501&&$s[5]==20;opendir(my $d,'/outputs')or die;my @f=grep{$_ ne '.'&&$_ ne '..'}readdir($d);die if @f;print qq(owned-empty-output-501-20-0700\n);"
  dk('output-permissions-before-worker',['exec','--user','501:20',NAME,'/usr/bin/perl','-e',modeprobe]);assert read('output-permissions-before-worker')==b'owned-empty-output-501-20-0700\n'
  prime=r"use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=sorted(bodies);dk('prime-controls',['exec','--user','501:20',NAME,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-e',prime,*['/controls/'+n for n in names]]);assert json.loads(read('prime-controls'))==[pin(bodies[n])for n in names]
  stable([O/'controls'/n for n in names],{str(O/'controls'/n):pin(bodies[n])for n in names},'new-controls-first-admission');m.fence();m.save(O/'selected-host-inputs-before-worker.private.json',m.HELD)
  dk('worker-syntax',['exec','--user','501:20',NAME,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-c','/controls/worker.pl'])
  ev=dk('rpc-current-worker',['exec','--user','501:20',NAME,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','/controls/worker.pl','--rpc-pruning-current'],610,None)
  assert ev['exitCode']==0, 'RPC worker failed; raw stdout/stderr and exit receipt retained'
  reader=r"use strict;use warnings;opendir(my $d,'/outputs')or die;my @f=sort grep {/\A[A-Za-z0-9_.-]+\z/&&-f('/outputs/'.$_)&&!-l('/outputs/'.$_)}readdir($d);closedir($d);die if @f>128;my $sum=0;for(@f){my $s=-s('/outputs/'.$_);die unless $s<67108864;$sum+=$s;}die if $sum>268435456;exec '/usr/bin/tar','-cf','-','-C','/outputs','--',@f;die;"
  with(O/'artifacts.private.tar').open('xb')as f:
   q=subprocess.run(docker+['exec','--user','501:20',NAME,'/usr/bin/perl','-e',reader],stdout=f,stderr=subprocess.PIPE,timeout=60,preexec_fn=limit);assert q.returncode==0 and not q.stderr
  with tarfile.open(O/'artifacts.private.tar','r:')as tf:
   seen=set()
   for z in tf:
    assert z.isfile()and pathlib.Path(z.name).name==z.name and z.name not in seen and z.size<67108864;seen.add(z.name);body=tf.extractfile(z).read(z.size+1);assert len(body)==z.size;(O/'outputs'/z.name).write_bytes(body)
  assert ev['exitCode']==0 and not(O/'rpc-current-worker-stderr.private.log').read_bytes();result=json.loads((O/'outputs/result-public.json').read_bytes());assert json.loads(read('rpc-current-worker'))==result
  assert result['passed']and result['owner']=='mac-docker'and result['sourceRevisionDeclared']==HEAD and result['rpcTargetedRuns']==1
  assert all(result[k]==1 for k in ['independentMCommands','compiledTranslationUnits','linkCommands','loaderCommands']);assert result['rpcTargetedSummary']['checks']==112 and result['rpcTargetedSummary']['expectedRefusals']==20 and result['ownedSyntheticTmpNamespaceAbsentAfterTest']
  assert pin((O/'outputs/software-admission.private.json').read_bytes())=={'sha256':result['softwareAdmissionSha256'],'bytes':result['softwareAdmissionBytes']}
  exe=O/'outputs/nano-rpc-simulation-block-id-tests';assert pin(exe.read_bytes())=={'sha256':result['softwareSha256'],'bytes':result['softwareBytes']};os.chmod(exe,0o500);stable([exe],{str(exe):pin(exe.read_bytes())},'new-test-copy-first-admission')
 except BaseException as ex:first=ex;raise
 finally:
  try:
   for sig in[signal.SIGTERM,signal.SIGINT]:signal.signal(sig,signal.SIG_IGN)
   q=subprocess.run(docker+['ps','-a','--filter','label='+LABEL,'--format','{{.Names}}'],capture_output=True,timeout=15);assert q.returncode==0;names=q.stdout.decode().split();assert set(names)<={NAME,NAME+'-init'}
   for n in names:
    a=json.loads(subprocess.check_output(docker+['inspect',n],timeout=15));assert len(a)==1 and a[0]['Name']=='/'+n and a[0]['Config']['Labels'].get('nano.rpc.current')==NAME and a[0]['Image']==IMAGE;subprocess.run(docker+['rm','-f',n],capture_output=True,timeout=15,check=True)
   q=subprocess.run(docker+['ps','-a','--filter','label='+LABEL,'--format','{{.ID}}'],capture_output=True,timeout=15);assert q.returncode==0 and not q.stdout.strip();m.save(O/'cleanup-public.json',{'ownContainersAbsent':True,'outputVolumeRetained':True,'imagesBuilt':0})
  except BaseException:
   m.save(O/'cleanup-failure-public.json',{'passed':False,'firstFailurePreserved':first is not None})
   if first is None:raise
 m.fence();assert git('rev-parse','HEAD').decode().strip()==HEAD and git('status','--porcelain')==b''
 result.update(rootTestOnlyCmakeDeltaVerified=True,rootTestTranslationUnitRevision=HEAD,rootPerlSyntaxPassed=True,rootGitHead=HEAD,rootSelectedSourceRevision=REV,rootGitProjectionBlobsVerified=len(wanted),rootSevenTreesVerified=True,rootImageIdentityVerified=True,rootSixRetainedLibraryRowsFullNineAndShaUnchanged=True,rootCoreFullNineAndShaUnchanged=True,rootOwnedContainersAbsent=True,rootNoOldAuthorityRefreshed=True,rootWholeCmakeConfigureQualified=False,productionQualified=False);m.save(O/'result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 failed=False
 try:main()
 except BaseException:failed=True;raise
 finally:
  try:
   m.fence()
   if CREATED:m.save(O/'selected-inputs-final-fence-public.json',{'passed':True,'heldSelectedFiles':len(m.HELD),'firstFailurePreserved':failed,'oldAuthoritiesRefreshed':False})
  except BaseException:
   if CREATED:m.save(O/'selected-inputs-final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':failed})
   if not failed:raise
