from pathlib import Path
import subprocess,json,collections,datetime
root=Path('/private/tmp/nano-restored-three-node-v9-dispatch-20261006/campaign');r=json.loads((root/'resources.json').read_text());raw={}
raw['mac-native']=(root/'native.log').read_text()
raw['mac-docker']=subprocess.check_output(['docker','logs',r['localContainer']],stderr=subprocess.STDOUT,text=True)
raw['vps-docker']=subprocess.check_output(['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166','sudo docker logs '+r['remoteContainer']+' 2>&1'],text=True)
out={'time':datetime.datetime.now(datetime.timezone.utc).isoformat(),'pointObservationOnly':True,'productionQualified':False,'nodes':{}}
for n,s in raw.items():
 events=[]
 for l in s.splitlines():
  try:events.append(json.loads(l))
  except (ValueError,TypeError):pass
 transitions=[e for e in events if e.get('event')=='test-core-dispatch-transition'];completion=collections.Counter(e.get('reason') for e in events if e.get('event')=='test-consensus-stream-completion')
 out['nodes'][n]={'sendCounts':dict(collections.Counter((e.get('outcome','?')+'|'+e.get('kind','?')) for e in events if e.get('event')=='test-consensus-send-completion')),'sendTail':[e for e in events if e.get('event')=='test-consensus-send-completion'][-4:],'eventCounts':dict(collections.Counter(e.get('event') for e in events)),'catalogResults':[e for e in events if e.get('event')=='test-capella-catalog-relay'][-12:],'dispatchTransitions':transitions,'streamCompletionReasons':dict(completion),'budget':next((e for e in reversed(events) if e.get('event')=='test-network-budget'),None)}
for name,key,remote in [(r['localContainer'],'mac-docker',False),(r['remoteContainer'],'vps-docker',True)]:
 args=['docker','stats','--no-stream','--format','{{json .}}',name]
 if remote:args=['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166','sudo '+' '.join(["'"+a.replace("'","'\\''")+"'" for a in args])]
 out['nodes'][key]['resources']=json.loads(subprocess.check_output(args,text=True))
out['nodes']['mac-native']['resources']=subprocess.check_output(['ps','-p',str(r['nativePid']),'-o','pid=,rss=,%cpu=,etime='],text=True).strip()
p=root/('live-snapshot-'+datetime.datetime.now(datetime.timezone.utc).strftime('%H%M%S')+'.json');p.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({'snapshot':str(p),'transitions':{n:d['dispatchTransitions'] for n,d in out['nodes'].items()},'streamReasons':{n:d['streamCompletionReasons'] for n,d in out['nodes'].items()},'sendCounts':{n:d['sendCounts'] for n,d in out['nodes'].items()},'sendTail':{n:d['sendTail'] for n,d in out['nodes'].items()}}))
