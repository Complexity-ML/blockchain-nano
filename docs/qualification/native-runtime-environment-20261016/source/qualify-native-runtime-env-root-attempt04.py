#!/usr/bin/env python3
"""Root-only isolated public-source Node checks; no Nano/Docker/network execution."""
import argparse,hashlib,io,json,os,pathlib,re,stat,subprocess,tarfile,time
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');P=E/'native-runtime-loader-env-source-proposal-20261016';O=E/'native-runtime-env-root-qualification-attempt04-20261016'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');held={}
def material(p):
 assert p.is_absolute() and p.resolve()==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);assert stat.S_ISREG(a.st_mode) and a.st_nlink==1;row={k:getattr(a,k) for k in F};h=hashlib.sha256();parts=[]
  while True:
   b=os.read(fd,1048576)
   if not b:break
   h.update(b);parts.append(b)
  assert row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(p.lstat(),k) for k in F}
  return dict(row,sha256=h.hexdigest()),b''.join(parts)
 finally:os.close(fd)
def hold(p,pin=None):
 row,b=material(p)
 if pin:
  if 'st_dev' in pin:assert row==pin
  else:assert row['sha256']==pin['sha256'] and row['st_size']==pin['bytes']
 assert str(p) not in held or held[str(p)]==row;held[str(p)]=row;return b
def fence():
 for p,row in held.items():assert material(pathlib.Path(p))[0]==row
def save(p,x):
 with p.open('x') as f:json.dump(x,f,sort_keys=True);f.write('\n')
def main(a):
 os.umask(0o077);assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip()=='f5b1510996003168f0d3c05fdeaf9a3d215cc8ae';assert subprocess.check_output(['git','status','--porcelain'],cwd=R)==b''
 O.mkdir(mode=0o700);source=O/'source';source.mkdir()
 m=json.loads(hold(P/'manifest.json',{'sha256':a.sha256,'bytes':a.bytes}))
 for x in m['baselines']:hold(R/x['path'],x)
 for x in m['payloads']:hold(P/x['path'],x)
 expected={};raw=subprocess.check_output(['git','archive','HEAD','tests/multihost','scripts/operations','tests/capella/isolated-bootstrap-plan.mjs','tests/capella/finality-observation.mjs','tests/capella/isolated-fixture-domain.mjs','tests/capella/isolated-runtime-observation.mjs','tests/capella/restored-finality-progress.mjs','tests/capella/restored-http-observation.mjs','tests/assertoor/stability.mjs','docs/qualification/local-supervisor-linux-20261015/source/known-files.pl','docs/qualification/local-supervisor-linux-20261015/source/copy-bootstrap-pairs.pl'],cwd=R)
 with tarfile.open(fileobj=io.BytesIO(raw)) as tf:
  for item in tf:
   assert item.isfile() or item.isdir();rel=pathlib.Path(item.name);assert not rel.is_absolute() and '..' not in rel.parts
   p=source/rel
   if item.isdir():p.mkdir(parents=True,exist_ok=True)
   else:p.parent.mkdir(parents=True,exist_ok=True);b=tf.extractfile(item).read();p.write_bytes(b);expected[str(p)]={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
 for x in m['payloads']:
  if not x['path'].startswith('tests/multihost/'):continue
  p=source/x['path'];b=hold(P/x['path'],x);p.write_bytes(b);expected[str(p)]={'sha256':x['sha256'],'bytes':x['bytes']}
  observations=[];start=time.monotonic();previous=None;consecutive=0;since=start
 while True:
  table={name:material(pathlib.Path(name))[0] for name in expected};now=time.monotonic()
  for name,row in table.items():assert row['sha256']==expected[name]['sha256'] and row['st_size']==expected[name]['bytes']
  observations.append({'elapsedMs':int((now-start)*1000),'files':table})
  if table==previous:consecutive+=1
  else:consecutive=1;since=now
  assert now-start<30
  if consecutive>=3 and now-since>=2:break
  previous=table;time.sleep(1)
 for name,row in table.items():hold(pathlib.Path(name),row)
 old=json.loads((E/'full-payout-total-root-qualification-attempt02-20261016/inputs-admitted.private.json').read_bytes())['fixedFiles'];nodepath=next(k for k in old if k.endswith('/node'));node=pathlib.Path(nodepath);hold(node,old[nodepath])
 fence();save(O/'inputs-admitted.private.json',{'files':held,'newSourceObservations':observations,'oldAuthoritiesRefreshed':False})
 argv=[str(node),'--test','--test-reporter=tap','tests/multihost/lifecycle-driver-guard.test.mjs'];save(O/'argv.public.json',argv)
 began=time.monotonic()
 with (O/'node-test.log').open('xb') as f:r=subprocess.run(argv,cwd=source,stdout=f,stderr=subprocess.STDOUT,timeout=90)
 elapsed=int((time.monotonic()-began)*1000);assert r.returncode==0;fence();text=(O/'node-test.log').read_text()
 counts={key:int(re.search(r'^# '+key+r' (\d+)$',text,re.M).group(1)) for key in ['tests','pass','fail','cancelled','skipped','todo']}
 assert counts['tests']==1 and counts['pass']==counts['tests'] and not any(counts[k] for k in ['fail','cancelled','skipped','todo'])
 result={'format':'nano-native-runtime-env-root-qualification-v1','passed':True,'elapsedMs':elapsed,'counts':counts,'newPureEnvironmentGroups':0,'reusedExactPureEnvironmentGroups':3,'reusedPureResultSha256':'4ffc70360e4cbbadb59a3e945cfa51227581fca69d1643cffcec73d362637792','existingGuardRerunReason':'launcher and helper source bindings changed','oldAuthoritiesRefreshed':False,'nanoStarted':False,'dockerInvoked':False,'networkStarted':False,'keysRead':False,'runtimeEnvironmentFullyEqualToPreviousConsumer':False,'fullLoaderQualified':False,'productionQualified':False}
 save(O/'result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--sha256',required=True);p.add_argument('--bytes',type=int,required=True);main(p.parse_args())
