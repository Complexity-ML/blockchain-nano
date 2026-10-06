from pathlib import Path
import subprocess,json,time
p=Path('/private/tmp/nano-dispatch-recovery-20261006')
for mode in ['http-baseline','http-candidate']:
 start=time.monotonic();r=subprocess.run([str(p/mode)],capture_output=True,text=True,timeout=25)
 (p/(mode+'-loopback-run.log')).write_text(r.stdout+r.stderr);(p/(mode+'-loopback-exit.json')).write_text(json.dumps({'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start})+'\n');print(mode,r.returncode,r.stdout.strip(),r.stderr.strip(),flush=True)
