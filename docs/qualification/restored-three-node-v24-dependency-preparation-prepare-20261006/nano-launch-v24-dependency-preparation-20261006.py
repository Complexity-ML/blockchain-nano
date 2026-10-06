from pathlib import Path
import subprocess,json,os,datetime,sys
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-restored-three-node-v24-dependency-preparation-20261006');rev=sys.argv[1];log=Path('/private/tmp/nano-restored-three-node-v24-controller-20261006.log')
env=os.environ.copy();env['PATH']='/private/tmp/nano-v9-poststop-ipv4-tools/bin:'+env['PATH'];env['NANO_TEST_RESTORED_PREPARATION']=str(repo/'docs/qualification/restored-three-node-v24-dependency-preparation-prepare-20261006');env['NANO_TEST_VPS_LOCAL_SOURCE']='/tmp/nano-restored-three-node-vps-muwhm1u4'
args=['node','tests/capella/restored-three-node-run.mjs','ubuntu@145.239.199.166',rev,str(root)]
with log.open('w') as f:
 p=subprocess.Popen(args,cwd=repo,env=env,stdout=f,stderr=subprocess.STDOUT)
 Path('/private/tmp/nano-v24-controller-process-20261006.json').write_text(json.dumps({'pid':p.pid,'revision':rev,'root':str(root),'controllerLog':str(log),'deadlineSeconds':900,'targetDistinctAdvances':3},indent=2)+'\n')
 code=p.wait()
(root/'campaign/terminal-exit.json').write_text(json.dumps({'exitCode':code,'observedTerminalAt':datetime.datetime.now(datetime.timezone.utc).isoformat()},indent=2)+'\n')
print('Original v24 controller terminal',code)
raise SystemExit(code)
