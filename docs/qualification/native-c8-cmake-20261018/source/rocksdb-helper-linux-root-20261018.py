#!/usr/bin/env python3
import hashlib,json,os,subprocess,sys
from pathlib import Path
E=Path(__file__).resolve().parent
O=E/'rocksdb-helper-linux-root-20261018-01'
C=Path('/private/tmp/nano-rocksdb-helper-linux-case-root-20261018-01')
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
D=['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'capella-sync-adverse-docker-client-root-20261018-03'),'--host','unix:///Users/boris/.docker/run/docker.sock']
O.mkdir(mode=0o700);C.mkdir(mode=0o700);(C/'tests/support').mkdir(parents=True,mode=0o700)
(O/'out').mkdir(mode=0o700);(O/'out/tmp').mkdir(mode=0o700)
sources=[('probe.cpp',E/'rocksdb-test-helper-probe-source-proposal-20261018-01/source/tests/qualification/rocksdb_test_helper_probe.cpp','a9b86ebbdc0567ed4d42587644f7dd7a53e990aba36247ca4e2d96b253028edd'),
 ('tests/support/rocksdb.hpp',E/'rocksdb-test-helper-source-proposal-20261018-01/source/tests/support/rocksdb.hpp','a128ab10f79c3207412dc0a8a224ee699d865139b1b0cc9fdd0da8de5aa96376')]
for name,p,want in sources:
 b=p.read_bytes();assert hashlib.sha256(b).hexdigest()==want
 with (C/name).open('xb') as f:f.write(b)
 (C/name).chmod(0o400)
oracle=json.loads((E/'rocksdb-test-helper-probe-source-proposal-20261018-01/oracle.json').read_text())
report={'format':'nano-rocksdb-helper-linux-root-v1','image':IMAGE,'architecture':'arm64','status':'RUNNING',
        'productionQualified':False,'imageBuilds':0,'coreBuilds':0,'phases':[]}
def run(name,args):
 result=subprocess.run(D+args,input=b'',capture_output=True,timeout=90)
 for suffix,body in [('stdout',result.stdout),('stderr',result.stderr)]:
  with (O/(name+'.'+suffix+'.log')).open('xb') as f:f.write(body)
 report['phases'].append({'name':name,'argv':D+args,'exitCode':result.returncode})
 assert result.returncode==0,name+' failed'
 return result.stdout
sandbox=['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges',
         '--user','501:20','--memory','512m','--cpus','1','--pids-limit','128','--env','TMPDIR=/out/tmp',
         '--mount','type=bind,src='+str(C)+',dst=/case,readonly',
         '--mount','type=bind,src='+str(O/'out')+',dst=/out']
try:
 initial=json.loads(run('image-before',['image','inspect',IMAGE,'--format','{{json .}}']))
 assert initial['Id']==IMAGE and initial['Os']=='linux' and initial['Architecture']=='arm64'
 run('provider',sandbox+['--entrypoint','/usr/bin/dpkg-query',IMAGE,'-W','librocksdb-dev','librocksdb8.9'])
 run('compile',sandbox+['--entrypoint','/usr/bin/clang++',IMAGE,'-std=c++20','-O2','-MMD','-MF','/out/helper-probe.d','-I/case','/case/probe.cpp','-lrocksdb','-o','/out/helper-probe'])
 raw=run('probe',sandbox+['--entrypoint','/out/helper-probe',IMAGE,'/out/synthetic-rocksdb-helper-linux01'])
 record=json.loads(raw);assert set(record)==set(oracle['fixed'])|set(oracle['integers'])
 for k,v in oracle['fixed'].items():assert type(record[k])==type(v) and record[k]==v,k
 assert type(record['elapsedMs']) is int and 0<=record['elapsedMs']<=29999
 assert (O/'probe.stderr.log').read_bytes()==b''
 run('loader',sandbox+['--entrypoint','/usr/bin/ldd',IMAGE,'/out/helper-probe'])
 assert b'not found' not in (O/'loader.stdout.log').read_bytes()
 deps=(O/'out/helper-probe.d').read_text();assert '/case/tests/support/rocksdb.hpp' in deps
 for name,_,want in sources:assert hashlib.sha256((C/name).read_bytes()).hexdigest()==want
 final=json.loads(run('image-after',['image','inspect',IMAGE,'--format','{{json .}}']));assert initial==final
 assert not (O/'out/synthetic-rocksdb-helper-linux01').exists()
 report['record']=record;report['status']='PASS'
except BaseException as e:report['status']='FAILED';report['error']=str(e)
finally:
 with (O/'result-public.json').open('x') as f:json.dump(report,f,indent=2,sort_keys=True);f.write('\n')
 print(json.dumps({'status':report['status'],'report':str(O/'result-public.json'),'error':report.get('error')}))
sys.exit(0 if report['status']=='PASS' else 1)
