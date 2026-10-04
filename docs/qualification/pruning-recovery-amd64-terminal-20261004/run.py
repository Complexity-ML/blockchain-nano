import pathlib,subprocess,json,hashlib,shlex,os,time
ROOT=pathlib.Path(__file__).resolve().parent
SOURCE=pathlib.Path('/home/ubuntu/nano-finalized-retention-recovery-20261004')
REV='94535766652e1d5eff53a7ce4f1ef3652e387262'
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
VOLUME='nano-clean-rpc-build-20261002';BASE='/build/finalized-retention-4734f285-amd64';OUT='/build/pruning-recovery-94535766-amd64';NAME='nano-pruning-recovery-amd64-20261004';OWNER=str(os.getuid())+':'+str(os.getgid())
def save(n,v):(ROOT/n).write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def call(args,timeout=60):
 p=subprocess.run(args,text=True,capture_output=True,timeout=timeout)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr[-1000:]))
 return p.stdout.strip()
def docker(args):return call(['sudo','-n','docker']+args)
def read_base(path):return docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--mount',f'type=volume,src={VOLUME},dst=/build,readonly',IMAGE,'cat',BASE+'/'+path])
def source_manifest():
 names=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0')
 return {n.decode():hashlib.sha256((SOURCE/n.decode()).read_bytes()).hexdigest() for n in names if n}
def original_programs():return docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--mount',f'type=volume,src={VOLUME},dst=/build,readonly',IMAGE,'sha256sum',BASE+'/nano-capella-transition-tests',BASE+'/nano-execution-checkpoint-tests',BASE+'/nano-execution-checkpoint-sync-tests',BASE+'/nano-storage-read-only-tests'])
try:
 assert call(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
 assert not call(['git','-C',str(SOURCE),'status','--porcelain'])
 assert json.loads(pathlib.Path('/home/ubuntu/nano-finalized-retention-amd64-control-20261004/state.json').read_text())['exit']==0
 assert not docker(['ps','-q']),'No overlapping Docker job'
 docker(['volume','inspect',VOLUME]);assert docker(['image','inspect',IMAGE,'--format','{{.Id}}'])==IMAGE
 before=source_manifest();save('sources-before.json',before);original=original_programs();(ROOT/'original-programs-before.txt').write_text(original+'\n')
 docker(['run','--rm','--network','none','--read-only','--user','0:0','--cap-drop','ALL','--cap-add','CHOWN','--mount',f'type=volume,src={VOLUME},dst=/build',IMAGE,'sh','-c','set -eu; test ! -e "$1"; mkdir "$1"; chown "$2" "$1"','sh',OUT,OWNER])
 results=[]
 for kind,target,file,mode in [('storage','nano-execution-checkpoint-tests','test_execution_checkpoint.cpp','--pruning-restore'),('capella','nano-capella-transition-tests','test_capella_transition.cpp','--historical-pruning-restore')]:
  flags=read_base('CMakeFiles/'+target+'.dir/flags.make');link_text=read_base('CMakeFiles/'+target+'.dir/link.txt');(ROOT/(kind+'-flags.make')).write_text(flags+'\n');(ROOT/(kind+'-link.txt')).write_text(link_text+'\n');values={l.split(' = ',1)[0]:l.split(' = ',1)[1] for l in flags.splitlines() if ' = ' in l};obj=OUT+'/'+file+'.o';binary=OUT+'/'+target
  compile=['clang++']+shlex.split(values['CXX_DEFINES'])+shlex.split(values['CXX_INCLUDES'])+shlex.split(values['CXX_FLAGS'])+['-c','/work/tests/'+file,'-o',obj]
  link=shlex.split(link_text);old='CMakeFiles/'+target+'.dir/tests/'+file+'.o';assert old in link;link[link.index(old)]=obj;link[link.index('-o')+1]=binary
  save(kind+'-commands.json',{'compile':compile,'link':link,'test':[binary,mode],'testTimeoutSeconds':180 if kind=='capella' else 120})
  script='set -u\ncd '+BASE+'\nmkdir -p '+OUT+'/fixture-scratch\nexport TMPDIR='+OUT+'/fixture-scratch\n'
  for phase,args in [('compile',compile),('link',link),('test',['timeout','180' if kind=='capella' else '120',binary,mode])]:
   script+=shlex.join(args)+' > /report/'+kind+'-'+phase+'.log 2>&1\nstatus=$?\nprintf "%s\\n" "$status" > /report/'+kind+'-'+phase+'-exit.txt\nif [ "$status" -ne 0 ]; then exit "$status"; fi\n'
  script+='sha256sum '+shlex.quote(binary)+' > /report/'+kind+'-program.txt\n';(ROOT/(kind+'-run.sh')).write_text(script)
  save('state.json',{'phase':kind,'revision':REV,'imageBuild':False,'productionQualified':False})
  args=['sudo','-n','docker','run','--rm','--name',NAME,'--user',OWNER,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount',f'type=volume,src={VOLUME},dst=/build','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={ROOT},dst=/report',IMAGE,'bash','/report/'+kind+'-run.sh']
  with (ROOT/(kind+'-controller.log')).open('w') as log:
   try:p=subprocess.run(args,text=True,stdout=log,stderr=subprocess.STDOUT,timeout=3600)
   except subprocess.TimeoutExpired:
    docker(['stop','-t','5',NAME]);raise
  (ROOT/(kind+'-controller-exit.txt')).write_text(str(p.returncode)+'\n')
  if p.returncode:raise RuntimeError(kind+' failed; keep logs')
  events=[json.loads(l) for l in (ROOT/(kind+'-test.log')).read_text().splitlines() if l.startswith('{')]
  wanted='historical-state-pruning-offline-restore' if kind=='storage' else 'capella-pruned-consensus-physical-restore';result=next(e for e in events if e.get('scenario')==wanted)
  if kind=='capella':assert result['journalsAdvanced']==result['signingPins']==64 and result['afterFinalizedEpoch']>result['beforeFinalizedEpoch']
  assert result['sourceBackupUnchanged'];results.append(result)
 after=source_manifest();save('sources-after.json',after);assert before==after
 original_after=original_programs();(ROOT/'original-programs-after.txt').write_text(original_after+'\n');assert original_after==original
 save('result.json',{'revision':REV,'architecture':'amd64','scenarios':results,'sourcesUnchanged':True,'originalProgramsUnchanged':True,'imageBuild':False,'productionQualified':False});save('state.json',{'phase':'terminal','exit':0,'revision':REV})
except Exception as e:
 save('failure.json',{'error':str(e),'productionQualified':False});save('state.json',{'phase':'terminal','exit':1,'revision':REV});raise
