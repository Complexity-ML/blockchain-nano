"""SOURCE_ONLY: Root executes once; prepare two NEW case copies and one ARM control."""
import hashlib,json,os,pathlib,re,signal,stat,subprocess,sys,time
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
D=E/'real-journal-rollback-arm-source-proposal-20261018-03'
OLD=E/'physical-cohort-arm-control-root-20261018-05/control.private.json'
OLD_PIN={'sha256':'3a1697c45c0fde5257963392715268013767a8ff53cba9ce68b3f6e5d0f97a3a','bytes':186873}
TEMPLATE_PIN={'sha256': '0117e8a7c4baf764d3d555f7f96413df2b7e66bfb7ce45bf54c28135c000eeba', 'bytes': 2194}
HOST_PIN={'sha256': 'c9f0ab2c6295905379d06354b9f224cb09c48a09c86264611657499de55eb0f1', 'bytes': 33108}
WORKER_PIN={'sha256': 'e88d6f69acb7a0a3bb688bd1c5482bd919e914a193a3398362a338c2c4ae5e6d', 'bytes': 20025}
CPP=E/'real-journal-rollback-source-proposal-20261018-03/beacon_journal_physical_rollback.cpp'
CPP_PIN={'sha256':'2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd','bytes':28801}
HELPER=pathlib.Path('/Users/boris/Dev/blockchain-nano/tools/storage-maintenance/bootstrap-readonly-replica.hpp')
HELPER_PIN={'sha256':'d3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125','bytes':20319}
INVENTORY=E/'physical-cohort-volume-export-root-20261018-01/export-inventory-public.json'
INVENTORY_PIN={'sha256':'19d3c604fb202602bc9d5a33e92aa89bb059034f229b419fd505973a88e38bae','bytes':18623}
CASE=E/'real-journal-rollback-arm-case-root-20261018-03'
CONTROL_DIR=E/'real-journal-rollback-arm-control-root-20261018-03'
OUTPUT=E/'real-journal-rollback-arm-root-20261018-03'
HEAD='ffe8cd99de3d3aaff2c0c634b871953660da12f9'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
FIELDS={'format','ready','actualHead','oldControl','sourceAuthority','caseRoot','outputRoot','hostPin','worker','volume','fixtureInventory','timeoutSeconds','recipe'}
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def decode(raw):
 def unique(items):
  d={}
  for k,v in items:need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=unique,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def material(path):
 p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p,'physical absolute input');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and 0<a.st_size<=67108864,'bounded regular input');chunks=[];n=0
  while b:=os.read(fd,65536):n+=len(b);need(n<=a.st_size,'input grew');chunks.append(b)
  raw=b''.join(chunks);need(n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'input full9/path changed');return dict(row,sha256=pin(raw)['sha256']),raw
 finally:os.close(fd)
def creation_anchor(row):return {k:row[k]for k in (*F[:8],'sha256')}
def syncdir(p):
 fd=os.open(p,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(fd)
 finally:os.close(fd)
def write_new(p,raw):
 need(p.parent.resolve(strict=True)==p.parent and not os.path.lexists(p),'physical parent/new path only');fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb')as f:f.write(raw);f.flush();os.fchmod(f.fileno(),0o400);os.fsync(f.fileno());a=os.fstat(f.fileno());born={k:getattr(a,k)for k in F}
 syncdir(p.parent);row,actual=material(p);need(actual==raw and all(row[k]==born[k]for k in F if k!='st_ctime_ns')and stat.S_IMODE(row['st_mode'])==0o400 and row['st_nlink']==1,'new bytes/creation eight/mode');return creation_anchor(row)

def main():
 held={};created={};first=None;secondary=[];events=[];result=None;control_dir_created=False
 def hold(p,wanted=None,original=None):
  p=pathlib.Path(p);row,raw=material(p)
  if wanted is not None:need(pin(raw)==wanted,'known pin changed: '+str(p))
  if original is not None:need(row==original,'original row changed: '+str(p))
  need(str(p)not in held or held[str(p)]==row,'no held authority refresh');held.setdefault(str(p),row);return raw
 def fail(exc,stage):
  nonlocal first
  item={'stage':stage,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None:first=item
  else:secondary.append(item)
 def git_command(git,repo,args):
  p=None;err=None;joined=False;began=time.monotonic();stdout=b'';stderr=b''
  try:
   p=subprocess.Popen([git,'--no-optional-locks','-c','core.fsmonitor=false','-C',repo,*args],stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'PATH':'/usr/bin:/bin','HOME':str(pathlib.Path.home()),'LC_ALL':'C','LANG':'C','GIT_OPTIONAL_LOCKS':'0'});need(p.pid>1 and p.pid!=28722,'safe owned Git child');stdout,stderr=p.communicate(timeout=15);need(len(stdout)<=1048576 and len(stderr)<=1048576 and p.returncode==0 and stderr==b'','bounded Git0 empty stderr')
  except BaseException as exc:err=exc
  finally:
   if p is not None and p.pid>1 and p.pid!=28722:
    try:
     try:os.killpg(p.pid,signal.SIGTERM)
     except ProcessLookupError:pass
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=3)
     try:os.killpg(p.pid,0)
     except ProcessLookupError:joined=True
     need(joined,'owned Git group remains')
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':'git-join','message':str(exc)})
    for stream in(p.stdout,p.stderr):
     try:stream.close()
     except BaseException as exc:
      if err is None:err=exc
      else:secondary.append({'stage':'git-close','message':str(exc)})
   events.append({'args':args,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-began)*1000)})
  if err is not None:raise err
  return stdout
 try:
  need(len(sys.argv)==1,'fixed prepare-root.py takes no arguments');need(E.resolve(strict=True)==E and E.is_dir(),'physical E parent');need(all(not os.path.lexists(p)for p in(CASE,CONTROL_DIR,OUTPUT)),'case/control/output namespaces all absent before mutation')
  template=decode(hold(D/'control.template.json',TEMPLATE_PIN));need(type(template)is dict and set(template)==FIELDS and template['format']=='nano-real-journal-rollback-arm-control-v1'and template['ready']is False,'exact unusable thirteen-field template');hold(D/'host.py',HOST_PIN);hold(D/'worker.py',WORKER_PIN)
  old=decode(hold(OLD,OLD_PIN));need(old['format']=='nano-physical-cohort-arm-control-v1'and old['ready']is True,'fixed old control');need(template['oldControl']=={'file':str(OLD),'pin':OLD_PIN}and template['hostPin']==HOST_PIN and template['worker']['pin']==WORKER_PIN,'template joins frozen sources/control')
  git=old['gitExecutable'];repo=old['repoRoot'];original=old['originalFiles'][git];need(set(original)==set(F)|{'sha256'},'genuine old Git full9+SHA');hold(git,original=original);need(git_command(git,repo,['rev-parse','HEAD']).decode().strip()==HEAD,'expected current HEAD');need(git_command(git,repo,['status','--porcelain'])==b'','clean current source checkout')
  cpp=hold(CPP,CPP_PIN);helper=hold(HELPER,HELPER_PIN);inventory=decode(hold(INVENTORY,INVENTORY_PIN));need(inventory['format']=='nano-public-cohort-export-inventory-v1'and inventory['entryCount']==69 and len(inventory['entries'])==69,'known old portable69 inventory')
  recipe={k:list(old[k])for k in('compilerFlags','linkPrefix','linkTail')};need(all(type(old[k])is list and recipe[k]==old[k]and recipe[k]and all(type(x)is str and x and '\0'not in x for x in recipe[k])for k in recipe),'exact old three-array recipe');need(recipe['compilerFlags'].count('-I/case/tests')==1 and '-DNANO_TEST_SOURCE="/work"'in recipe['compilerFlags'],'old case/test-source flags already present; do not append or remap')
  c=template;c.update(ready=True,actualHead=HEAD,caseRoot={'root':str(CASE),'pins':{'tests/qualification/beacon_journal_physical_rollback.cpp':CPP_PIN,'tools/storage-maintenance/bootstrap-readonly-replica.hpp':HELPER_PIN}},outputRoot=str(OUTPUT),worker={'file':str(D/'worker.py'),'pin':WORKER_PIN},fixtureInventory={'file':str(INVENTORY),'pin':INVENTORY_PIN},recipe=recipe);need(set(c)==FIELDS and c['recipe']=={k:old[k]for k in recipe},'exact thirteen fields and byte-identical recipe values')
  # All writes below are new paths. No old input is chmodded, copied elsewhere or re-admitted.
  CASE.mkdir(mode=0o700);syncdir(E)
  for relative,raw in(('tests/qualification/beacon_journal_physical_rollback.cpp',cpp),('tools/storage-maintenance/bootstrap-readonly-replica.hpp',helper)):
   p=CASE
   for component in pathlib.PurePosixPath(relative).parts[:-1]:
    q=p/component
    if not q.exists():q.mkdir(mode=0o700);syncdir(p)
    need(q.resolve(strict=True)==q and q.is_dir(),'new physical case parent');p=q
   target=CASE/relative;created[str(target)]=write_new(target,raw)
  for path,row in held.items():need(material(path)[0]==row,'all held original/source inputs unchanged before control')
  for path,row in created.items():need(creation_anchor(material(path)[0])==row,'new unadmitted case creation eight+SHA before control')
  CONTROL_DIR.mkdir(mode=0o700);control_dir_created=True;syncdir(E);control=CONTROL_DIR/'control.private.json';raw=(json.dumps(c,sort_keys=True,indent=2,allow_nan=False)+'\n').encode();created[str(control)]=write_new(control,raw)
  result={'format':'nano-real-journal-rollback-arm-preparation-v1','passed':True,'control':{'file':str(control),'pin':pin(raw)},'newCaseFiles':2,'createdNotAdmitted':True,'recipeByteEqualOld':True,'actualHead':HEAD,'hostPin':HOST_PIN,'workerPin':WORKER_PIN,'cppPin':CPP_PIN,'inputAuthoritiesRefreshed':False,'hostInvoked':False,'DockerInvoked':False,'CppInvoked':False,'CoreRebuilt':False,'productionQualified':False}
 except BaseException as exc:fail(exc,'preparation')
 finally:
  for table,stage in((held,'input-final-fence'),(created,'new-file-final-fence')):
   for path,row in table.items():
    try:
     actual=material(path)[0];need((creation_anchor(actual)if table is created else actual)==row,'old full9+SHA or new creation eight+SHA changed')
    except BaseException as exc:fail(exc,stage+':'+path)
  if result is None:result={'format':'nano-real-journal-rollback-arm-preparation-v1','passed':False,'hostInvoked':False,'DockerInvoked':False,'CppInvoked':False,'productionQualified':False}
  result.update(passed=first is None,firstFailure=first,secondaryFailures=secondary,heldInputFiles=len(held),newFiles=len(created),processEvents=events)
  if control_dir_created:
   try:write_new(CONTROL_DIR/'preparation-result-public.json',(json.dumps(result,sort_keys=True,indent=2,allow_nan=False)+'\n').encode())
   except BaseException as exc:fail(exc,'result-publication');result.update(passed=False,firstFailure=first,secondaryFailures=secondary)
 print(json.dumps(result,sort_keys=True,allow_nan=False));return 0 if result['passed']else 1
if __name__=='__main__':raise SystemExit(main())
