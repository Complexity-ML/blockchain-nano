from pathlib import Path
import subprocess,json,shlex,sys
arch=sys.argv[1];r=Path('/private/tmp/nano-network-status-coherent-20261006');remote='/tmp/nano-network-status-coherent-20261006';ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166']
if arch=='amd64':
 subprocess.run(ssh+['mkdir -p '+remote],check=True)
 files=[r/n for n in ['source.tar','source.sha256','source-manifest.json','build.sh','runtime.sh','regression.sh','all.sh']]
 for f in files:
  with f.open('rb') as inp:subprocess.run(ssh+['cat > '+shlex.quote(remote+'/'+f.name)],stdin=inp,check=True)
 subprocess.run(ssh+['tar -xf '+remote+'/source.tar -C '+remote],check=True)
 path=remote;repo='/home/ubuntu/blockchain-nano-git';vol='nano-clean-rpc-build-20261002';image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';cmd=['sudo','-n','docker']
else:
 path=str(r);repo='/Users/boris/Dev/blockchain-nano';vol='nano-clean-arm64-build-20261002';image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';cmd=['docker']
cmd+=['run','--rm','--network','none','--ulimit','nofile=65536:65536','--name','nano-network-status-coherent-'+arch+'-20261006','-v',repo+':/work:ro','-v',path+':/drivers','-v',path+':/overlay:ro','-v',vol+':/build']
for src,dst in [('src','src'),('include','include'),('network','network'),('tests/test_capella_transition.cpp','tests/test_capella_transition.cpp'),('tests/capella/paused_capture.cpp','tests/capella/paused_capture.cpp')]:cmd+=['-v',path+'/source/'+src+':/work/'+dst+':ro']
cmd += [image,'bash','/drivers/all.sh',arch]
(r/(arch+'-command.json')).write_text(json.dumps(cmd,indent=2))
actual=ssh+[shlex.join(cmd)] if arch=='amd64' else cmd
with (r/(arch+'-controller.log')).open('w') as log:
 p=subprocess.Popen(actual,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 for line in p.stdout:log.write(line);log.flush();print(line,end='',flush=True)
 code=p.wait()
(r/(arch+'-terminal-exit.json')).write_text(json.dumps({'exitCode':code}));sys.exit(code)
