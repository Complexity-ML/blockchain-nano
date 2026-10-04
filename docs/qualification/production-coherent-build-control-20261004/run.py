#!/usr/bin/env python3
import os,pathlib,shutil,subprocess,sys,json
ARCH=sys.argv[1]
assert ARCH in ('arm64','amd64')
REV='efbeb9bca3d42e41edb9cb81afba98c0b953a90f'
SOURCE=pathlib.Path('/tmp/nano-production-coherent-20261004' if ARCH=='arm64' else '/home/ubuntu/nano-production-coherent-20261004')
IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
VOLUMES={'arm64':'nano-clean-arm64-build-20261002','amd64':'nano-clean-rpc-build-20261002'}
BASE=pathlib.Path('/tmp' if ARCH=='arm64' else '/home/ubuntu')
REPORTS=BASE/('nano-coherent-full-'+ARCH+'-reports-20261004')
CONTROL=BASE/('nano-coherent-full-'+ARCH+'-control-20261004')
SUBDIR='coherent-efbeb9bc-'+ARCH
D=['docker'] if ARCH=='arm64' else ['sudo','-n','docker']
def out(args):return subprocess.check_output(args,text=True).strip()
assert out(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
assert not out(['git','-C',str(SOURCE),'status','--porcelain'])
assert out(D+['image','inspect',IMAGES[ARCH],'--format','{{.Id}}'])==IMAGES[ARCH]
assert VOLUMES[ARCH] in out(D+['volume','ls','-q']).splitlines()
assert not out(D+['ps','-q']), 'Refuse overlap with running containers'
assert not REPORTS.exists()
assert shutil.disk_usage(BASE).free>8*1024**3
CONTROL.mkdir(exist_ok=False)
env=os.environ.copy()
if ARCH=='amd64':
 (CONTROL/'bin').mkdir()
 wrapper=CONTROL/'bin/docker';wrapper.write_text('#!/bin/sh\nexec sudo -n /usr/bin/docker "$@"\n');wrapper.chmod(0o750)
 env['PATH']=str(CONTROL/'bin')+':'+env['PATH']
env.update(NANO_BUILDER_IMAGE=IMAGES[ARCH],NANO_BUILD_VOLUME=VOLUMES[ARCH],NANO_BUILD_SUBDIR=SUBDIR)
env.pop('NANO_RPC_TIMING_DIR',None)
command=['bash','scripts/ci/full-docker.sh',str(REPORTS)]
(CONTROL/'scope.json').write_text(json.dumps(dict(revision=REV,arch=ARCH,image=IMAGES[ARCH],volume=VOLUMES[ARCH],subdir=SUBDIR,source=str(SOURCE),reports=str(REPORTS),imageRebuild=False,freshBuild=True,qualificationPending=True),indent=2)+'\n')
with (CONTROL/'controller.log').open('wb') as log:
 try:code=subprocess.run(command,cwd=SOURCE,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=14400).returncode
 except subprocess.TimeoutExpired:
  for suffix in ['prepare','test']:
   name='nano-full-'+SUBDIR+'-'+suffix
   if out(D+['ps','-q','--filter','name=^/'+name+'$']):subprocess.run(D+['stop','-t','5',name],stdout=log,stderr=subprocess.STDOUT,timeout=30)
  code=124
(CONTROL/'controller-exit.txt').write_text(str(code)+'\n')
print('Coherent full '+ARCH+' controller terminal:',code,flush=True)
sys.exit(code)
