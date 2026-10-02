#!/usr/bin/env python3
"""Fresh coherent full regression, reused image/volume, no RPC instrumentation."""
import os,pathlib,shutil,subprocess
SOURCE=pathlib.Path('/home/ubuntu/nano-rpc-headroom-margin-candidate-20261002')
REV='d1597d9a6987d0839ed2f0c7ded268aead35d071'
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
VOLUME='nano-clean-rpc-build-20261002'
SUBDIR='rpc-full-d1597d9a-amd64'
REPORTS='/home/ubuntu/nano-rpc-full-amd64-reports-20261002'
CONTROL=pathlib.Path('/home/ubuntu/nano-rpc-full-amd64-control-20261002')
D=['sudo','-n','docker']
def out(args):return subprocess.check_output(args,text=True).strip()
assert out(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
assert not out(['git','-C',str(SOURCE),'status','--porcelain'])
assert out(D+['image','inspect',IMAGE,'--format','{{.Id}}'])==IMAGE
assert VOLUME in out(D+['volume','ls','-q']).splitlines()
assert not out(D+['ps','-q'])
assert not pathlib.Path(REPORTS).exists()
assert shutil.disk_usage('/home/ubuntu').free>8*1024**3
CONTROL.mkdir(exist_ok=False);(CONTROL/'bin').mkdir()
# Host-owned auditor runs as ubuntu; only Docker CLI receives sudo.
wrapper=CONTROL/'bin/docker';wrapper.write_text('#!/bin/sh\nexec sudo -n /usr/bin/docker "$@"\n');wrapper.chmod(0o750)
env=os.environ.copy();env['PATH']=str(CONTROL/'bin')+':'+env['PATH']
env.update(NANO_BUILDER_IMAGE=IMAGE,NANO_BUILD_VOLUME=VOLUME,NANO_BUILD_SUBDIR=SUBDIR)
env.pop('NANO_RPC_TIMING_DIR',None)
command=['bash','scripts/ci/full-docker.sh',REPORTS]
(CONTROL/'command.txt').write_text(' '.join(command)+'\n'+REV+'\n'+IMAGE+'\n'+SUBDIR+'\n')
with (CONTROL/'controller.log').open('wb') as log:
 try:code=subprocess.run(command,cwd=SOURCE,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=14400).returncode
 except subprocess.TimeoutExpired:
  for suffix in ['prepare','test']:
   name='nano-full-'+SUBDIR+'-'+suffix
   if out(D+['ps','-q','--filter','name=^/'+name+'$']):subprocess.run(D+['stop','-t','5',name],stdout=log,stderr=subprocess.STDOUT,timeout=30)
  code=124
(CONTROL/'controller-exit.txt').write_text(str(code)+'\n')
print('Full regression controller terminal:',code,flush=True)
