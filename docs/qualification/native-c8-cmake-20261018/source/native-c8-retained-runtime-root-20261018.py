#!/usr/bin/env python3
import hashlib,json,os,signal,stat,subprocess,sys,time
from pathlib import Path
E=Path(__file__).resolve().parent
P=E/'native-c8-cmake-root-20261018-03'
O=E/'native-c8-runtime-root-20261018-04'
B=P/'build'
T=Path('/private/tmp/nano-native-c8-runtime-20261018-04')
CMAKE='/opt/homebrew/Cellar/cmake/4.4.3/bin/cmake'
def pin(p):
 p=Path(p);a=p.lstat();body=p.read_bytes();z=p.lstat()
 def row(s):return [s.st_dev,s.st_ino,s.st_mode,s.st_nlink,s.st_uid,s.st_gid,s.st_size,s.st_mtime_ns,s.st_ctime_ns]
 assert stat.S_ISREG(a.st_mode) and a.st_nlink==1 and row(a)==row(z),str(p)
 return {'file':str(p),'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest(),'nine':row(a)}
def check(rows):
 for row in rows:assert pin(row['file'])==row,row['file']
def dump(p,obj):
 with p.open('x') as f:json.dump(obj,f,indent=2,sort_keys=True);f.write('\n');f.flush();os.fsync(f.fileno())
O.mkdir(mode=0o700);T.mkdir(mode=0o700)
report={'format':'nano-native-c8-retained-runtime-v1','status':'RUNNING','phases':[],
        'additionalBuildCommands':0,'priorFailedBatchPreserved':True,'priorPassingCasesReplayed':0,
        'productionQualified':False,'oldDataOpened':False,'runtimeDutiesResumed':False,'networkStarted':False}
guards=[]
def run(name,args,timeout):
 check(guards);started=time.monotonic();log=O/(name+'.log')
 print(json.dumps({'phase':name,'status':'START','log':str(log)}),flush=True)
 with log.open('xb') as f:
  proc=subprocess.Popen(args,cwd=B,env={'PATH':'/usr/bin:/bin','LANG':'C','LC_ALL':'C','TMPDIR':str(T)},
                        stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
  try:code=proc.wait(timeout=timeout)
  except BaseException:
   assert proc.pid>1 and proc.pid!=28722;os.killpg(proc.pid,signal.SIGTERM)
   try:proc.wait(timeout=5)
   except subprocess.TimeoutExpired:os.killpg(proc.pid,signal.SIGKILL);proc.wait(timeout=5)
   raise
  f.flush();os.fsync(f.fileno())
 report['phases'].append({'name':name,'argv':args,'exitCode':code,'elapsedSeconds':round(time.monotonic()-started,3),'log':pin(log)})
 check(guards);assert code==0,name+' failed; see '+str(log)
 print(json.dumps({'phase':name,'exitCode':code}),flush=True)
 return log.read_text()
try:
 prior=json.loads((P/'result-public.json').read_text())
 assert prior['status']=='FAILED' and prior['phases'][0]['exitCode']==0 and prior['phases'][1]['exitCode']==0
 assert prior['phases'][3]['name']=='ctest' and prior['phases'][3]['exitCode']==8
 report['priorResult']=pin(P/'result-public.json')
 guards=json.loads((P/'inputs-private.json').read_text());check(guards)
 products=[pin(B/name) for name in ['nano','nano-cli-readonly-tests','nano-cli-readonly-probe',
           'nano-capella-bootstrap-startup-tests','nano-storage-maintenance',
           'nano-bootstrap-archive-maintenance','nano-capella-duty-readiness','libnano_core.a']]
 for _ in range(3):time.sleep(1);check(products)
 guards+=products;dump(O/'retained-inputs-private.json',guards)
 report['retainedProducts']=products
 api=json.loads(run('readonly-api',[str(B/'nano-cli-readonly-tests'),'--self-test'],120))
 assert api['result']=='PASS' and [api[k] for k in ['groups','expectedRefusals','actualWriteRefusals','positiveReadonlyOpens']]==[6,11,2,5]
 report['api']=api
 bootstrap=run('bootstrap-startup',[str(B/'nano-capella-bootstrap-startup-tests'),'--self-test'],70)
 report['bootstrapRecords']=[json.loads(line) for line in bootstrap.splitlines() if line.startswith('{')]
 assert len(report['bootstrapRecords'])==7 and all(x.get('result')=='PASS' for x in report['bootstrapRecords'])
 nano=products[0]
 probe=json.loads(run('cli-probe',[str(B/'nano-cli-readonly-probe'),str(B/'nano'),nano['sha256'],str(nano['bytes']),str(T/'synthetic-cli-readonly-native-c8')],210))
 assert probe['result']=='PASS' and [probe[k] for k in ['actualCliCommands','successfulCliExits','expectedCliRefusals','readonlySourceChecks']]==[18,8,10,15]
 report['probe']=probe
 I=O/'install';run('install',[CMAKE,'--install',str(B),'--prefix',str(I)],120)
 notices=I/'share/nano/licenses';rows=[]
 for line in (notices/'notices.sha256').read_text().splitlines():
  expected,name=line.split('  ');actual=pin(notices/name);assert actual['sha256']==expected;rows.append(actual)
 assert len(rows)==11;report['installedNotices']=rows
 binaries=[I/'bin'/x for x in ['nano','nano-storage-maintenance','nano-bootstrap-archive-maintenance','nano-capella-duty-readiness']]
 report['installedBinaries']=[pin(x) for x in binaries]
 run('installed-load-commands',['/usr/bin/otool','-L',*[str(x) for x in binaries]],30)
 run('installed-help',[str(I/'bin/nano'),'--help'],30)
 check(guards);report['status']='PASS'
except BaseException as error:report['status']='FAILED';report['error']=str(error)
finally:
 dump(O/'result-public.json',report)
 print(json.dumps({'status':report['status'],'report':str(O/'result-public.json'),'phases':len(report['phases']),'error':report.get('error')}),flush=True)
sys.exit(0 if report['status']=='PASS' else 1)
