import json,subprocess,datetime,shlex
from pathlib import Path
root=Path('/private/tmp/nano-restored-three-node-v10-export-20261006/campaign');r=json.loads((root/'resources.json').read_text())
code='''import pathlib,json,time,datetime
p=pathlib.Path('/proc')
pids=[int(x.name) for x in p.iterdir() if x.name.isdigit() and (x/'comm').read_text().strip() in ['nano','nano-p2p']]
def sample():
 result={}
 for pid in pids:
  base=p/str(pid)
  try:
   status={x.split(':',1)[0]:x.split(':',1)[1].strip() for x in (base/'status').read_text().splitlines() if ':' in x}
   tasks={}
   for t in (base/'task').iterdir():
    st=(t/'stat').read_text();after=st.rsplit(')',1)[1].split();tasks[t.name]={'comm':(t/'comm').read_text().strip(),'utime':int(after[11]),'stime':int(after[12]),'wchan':(t/'wchan').read_text().strip()}
   result[str(pid)]={'comm':(base/'comm').read_text().strip(),'io':(base/'io').read_text(),'status':{k:status[k] for k in ['VmRSS','VmHWM','Threads']},'tasks':tasks}
  except FileNotFoundError:pass
 return result
before=sample();time.sleep(2);after=sample();print(json.dumps({'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'before':before,'after':after,'pointObservationOnly':True}))
'''
cmd=['/usr/bin/ssh','-4','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(['sudo','-n','docker','exec',r['remoteContainer'],'python3','-c',code])]
s=json.loads(subprocess.check_output(cmd,text=True));path=root/('vps-thread-sample-'+datetime.datetime.now(datetime.timezone.utc).strftime('%H%M%S')+'.json');path.write_text(json.dumps(s,indent=2)+'\n');print(path)
