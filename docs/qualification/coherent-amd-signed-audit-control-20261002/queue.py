"""Wait for immutable terminal gates; run one signed offline audit, no rebuild."""
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
control=Path(__file__).resolve().parent
state_dir=Path('/home/ubuntu/nano-coherent-amd-signed-controller-state-20261002')
full=Path('/home/ubuntu/nano-coherent-network-amd64-reports-20261002')
network=Path('/home/ubuntu/nano-network-supplement-amd64-reports-20261002')

def stage(directory,names):
    values=[(directory/n).read_text().strip() if (directory/n).exists() else '' for n in names]
    if any(v and v!='0' for v in values):return 'refused'
    return 'ready' if all(v=='0' for v in values) else 'pending'

def live(pid,marker):
    try:
        os.kill(pid,0)
        return marker.encode() in Path('/proc/'+str(pid)+'/cmdline').read_bytes()
    except (OSError,PermissionError):return False

def main():
    if not __debug__:raise RuntimeError('Assertions must remain enabled')
    state_dir.mkdir()
    state=state_dir/'queue-state.json'
    manifest={str(p.relative_to(control)):hashlib.sha256(p.read_bytes()).hexdigest() for p in control.rglob('*') if p.is_file() and '__pycache__' not in str(p)}
    (state_dir/'controller-manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2))
    def record(**kw):state.write_text(json.dumps({'timeUtc':time.time(),'pid':os.getpid(),**kw},indent=2))
    deadline=time.monotonic()+86400
    while True:
        f=stage(full,['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt'])
        n=stage(network,['ctest-exit.txt','container-exit.txt','audit-exit.txt'])
        if 'refused' in (f,n):
            record(phase='terminal-gate-refused',fullStage=f,networkStage=n,signedCasesStarted=False);return 2
        if (f,n)==('ready','ready'):break
        primary_live=live(946683,'run-vps.sh');network_live=live(948352,'network-supplement-queue.py')
        record(phase='waiting-terminal-campaigns',primaryLive=primary_live,networkLive=network_live,fullStage=f,networkStage=n,signedCasesStarted=False)
        if (f=='pending' and not primary_live) or (n=='pending' and not network_live):
            record(phase='owner-stopped-with-incomplete-evidence',primaryLive=primary_live,networkLive=network_live,signedCasesStarted=False);return 2
        if time.monotonic()>=deadline:
            record(phase='observation-expired',signedCasesStarted=False,existingCampaignsUntouched=True);return 3
        time.sleep(15)
    record(phase='terminal-review-and-signed-audit',signedCasesStartedProven=False)
    with (state_dir/'audit-driver.log').open('w') as out:
        code=subprocess.run([sys.executable,str(control/'run.py')],stdout=out,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}).returncode
    (state_dir/'controller-exit.txt').write_text(str(code)+'\n')
    record(phase='terminal',exit=code,newWanNodesStarted=False)
    return code
if __name__=='__main__':
    try:code=main()
    except Exception as error:
        if state_dir.exists():(state_dir/'controller-error.txt').write_text(str(error)+'\n')
        print(str(error),file=sys.stderr);code=1
    raise SystemExit(code)
