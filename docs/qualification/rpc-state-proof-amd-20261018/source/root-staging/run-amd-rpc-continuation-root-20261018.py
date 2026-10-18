"""Run only the admitted retained AMD continuation, never restart on observation timeout."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
E=Path(__file__).parent.resolve()
O=E/'amd-rpc-continuation-dispatch-root-20261018-01'
stage=json.loads((E/'amd-rpc-continuation-staging-root-20261018-01/result-public.json').read_text())
HEAD='4388f77b9e9c1b3b94ba5da6f3e486eaa13a3f5c'
PIN={'sha256':'e6a37b5069a6786dea733bc3c0136e59fae63d2be214a34027b13d55ecb3b9b1','bytes':1281491}
assert stage['passed'] is True and stage['actualHead']==HEAD and stage['controlPin']==PIN and stage['cppExecuted'] is False
mf=(E/'amd-rpc-continuation-source-v2-20261018/manifest.json').read_bytes()
assert (hashlib.sha256(mf).hexdigest(),len(mf))==('60ba0cb1bbf94234ff74a0783dcf3cefe8cdd81e7a2de94f71e09fc8f82e6e7b',4167)
assert subprocess.check_output(['/Library/Developer/CommandLineTools/usr/bin/git','-C','/Users/boris/Dev/blockchain-nano','rev-parse','HEAD']).decode().strip()==HEAD
assert subprocess.check_output(['/Library/Developer/CommandLineTools/usr/bin/git','-C','/Users/boris/Dev/blockchain-nano','status','--porcelain'])==b''
argv=['/usr/bin/ssh','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','-o','ServerAliveInterval=30','-o','ServerAliveCountMax=3','ubuntu@145.239.199.166','/usr/bin/python3','/home/ubuntu/rpc-candidate-amd-continuation-tools-20261018-01/host.py','/home/ubuntu/rpc-candidate-amd-continuation-control-20261018-01/control.private.json',PIN['sha256'],str(PIN['bytes'])]
os.umask(0o077);O.mkdir(mode=0o700,exist_ok=False)
start=time.time()
(O/'dispatch-public.json').write_text(json.dumps({'format':'nano-amd-retained-continuation-dispatch-v1','argv':argv,'startedAtUnix':start,'hostBudgetSeconds':1380,'cleanupBudgetSeconds':60,'localObservationTimeoutSeconds':1800,'privateControlTransferred':False,'automaticRestart':False,'imageBuild':False,'sourceRevision':'c6cdf780c68eab8b06306d76257bda9028928ea3','productionQualified':False},indent=2)+'\n')
with (O/'host.stdout.log').open('xb') as stdout,(O/'host.stderr.log').open('xb') as stderr:
 p=subprocess.Popen(argv,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr)
 print(json.dumps({'state':'running','sshPid':p.pid,'logDirectory':str(O),'automaticRestart':False}),flush=True)
 try:rc=p.wait(timeout=1800)
 except subprocess.TimeoutExpired:
  (O/'observation-timeout-public.json').write_text(json.dumps({'state':'observation timeout; actual SSH still live','sshPid':p.pid,'automaticRestart':False})+'\n')
  print(json.dumps({'state':'observation-timeout','sshPid':p.pid,'automaticRestart':False}),flush=True)
  rc=p.wait()
result={'format':'nano-amd-retained-continuation-dispatch-exit-v1','exitCode':rc,'elapsedSeconds':round(time.time()-start,3),'actualProcessTerminal':True,'automaticRestart':False,'productionQualified':False}
(O/'dispatch-exit-public.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result),flush=True)
for name in ('host.stdout.log','host.stderr.log'):
 data=(O/name).read_bytes()
 print(json.dumps({'log':name,'bytes':len(data),'tail':data[-12000:].decode('utf-8','replace')}),flush=True)
sys.exit(rc if 0<=rc<=255 else 1)
