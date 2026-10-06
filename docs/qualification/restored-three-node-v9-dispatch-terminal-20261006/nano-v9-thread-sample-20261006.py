from pathlib import Path
import json,subprocess,shlex,datetime
root=Path('/private/tmp/nano-restored-three-node-v9-dispatch-20261006/campaign');r=json.loads((root/'resources.json').read_text())
code='''import json,pathlib,subprocess,time
name=NAME
rows=subprocess.check_output(['docker','top',name,'-eo','pid,comm'],text=True).splitlines()[1:]
pids=[int(x.split()[0]) for x in rows]
def sample():
 out={}
 for pid in pids:
  p=pathlib.Path('/proc')/str(pid);threads={}
  for t in (p/'task').iterdir():
   fields=(t/'stat').read_text().rsplit(')',1)[1].split();threads[t.name]={'state':fields[0],'userTicks':int(fields[11]),'systemTicks':int(fields[12]),'wchan':(t/'wchan').read_text().strip()}
  io={k:int(v) for k,v in [x.split(':') for x in (p/'io').read_text().splitlines()]}
  memory=[x for x in (p/'status').read_text().splitlines() if x.startswith(('Name:','VmRSS:','VmHWM:','Threads:'))]
  out[str(pid)]={'threads':threads,'io':io,'memory':memory}
 return out
first=sample();time.sleep(2);second=sample()
cgroups={}
for pid in pids:
 paths=(pathlib.Path('/proc')/str(pid)/'cgroup').read_text().splitlines()
 for row in paths:
  version,controllers,relative=row.split(':',2)
  if version=='0':
   base=pathlib.Path('/sys/fs/cgroup')/relative.lstrip('/');values={}
   for name in ['memory.current','memory.peak','memory.max','memory.events','cpu.stat','cpu.max']:
    f=base/name
    if f.exists():values[name]=f.read_text().strip()
   cgroups[str(pid)]={'path':relative,'values':values,'memoryPeakIsKernelCgroupHighWater':True}
print(json.dumps({'first':first,'second':second,'intervalSeconds':2,'pointSampleNotPeaks':True,'cgroups':cgroups}))
'''.replace('NAME',repr(r['remoteContainer']))
result=subprocess.check_output(['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(['sudo','-n','python3','-c',code])],text=True)
p=root/('vps-thread-sample-'+datetime.datetime.now(datetime.timezone.utc).strftime('%H%M%S')+'.json');p.write_text(result)
d=json.loads(result);s={}
for pid,v in d['second'].items():
 old=d['first'][pid];s[pid]={'memory':v['memory'],'diskWrittenBytes':v['io']['write_bytes']-old['io']['write_bytes'],'activeThreads':{t:{'cpuTicksDelta':x['userTicks']+x['systemTicks']-old['threads'].get(t,{}).get('userTicks',0)-old['threads'].get(t,{}).get('systemTicks',0),'wchan':x['wchan']} for t,x in v['threads'].items()}}
print(json.dumps({'snapshot':str(p),'intervalSeconds':2,'nodes':s}))
