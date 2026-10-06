from pathlib import Path
import subprocess,json,collections,datetime
root=Path('/private/tmp/nano-restored-three-node-v13-parent-cache-20261006/campaign');res=json.loads((root/'resources.json').read_text());out=root/'live-observations';out.mkdir(exist_ok=True)
samples={}
for name,cmd in [('mac-native',None),('mac-docker',['docker','logs',res['localContainer']]),('vps-docker',['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166','sudo -n docker logs '+res['remoteContainer']])]:
 if cmd:
  r=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=30);assert r.returncode==0;rtext=r.stdout
 else:rtext=(root/'native.log').read_text()
 events=[]
 for line in rtext.splitlines():
  try:events.append(json.loads(line))
  except ValueError:pass
 cats=[e for e in events if e.get('event')=='test-capella-catalog-relay'];kinds=sorted({e['kind'] for e in cats})
 samples[name]={'eventLines':len(events),'catalogues':{k:{'attempts':sum(e['kind']==k for e in cats),'payloadsAdmitted':sum(e.get('payloadsAdmitted',0) for e in cats if e['kind']==k),'rejected':sum(e.get('rejected',0) for e in cats if e['kind']==k),'reasons':dict(collections.Counter(str(e.get('reason')) for e in cats if e['kind']==k))} for k in kinds},'blockAttempts':[e for e in cats if e['kind']=='block'],'dispatchTransitions':[e for e in events if e.get('event')=='test-core-dispatch-transition']}
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ');payload={'observedAt':stamp,'scope':'point-in-time logs; attempts not unique objects; not terminal result','nodes':samples};(out/(stamp+'.json')).write_text(json.dumps(payload,indent=2)+'\n');print(json.dumps({n:s['catalogues'].get('block') for n,s in samples.items()}))
