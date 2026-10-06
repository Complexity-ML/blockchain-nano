from pathlib import Path
import json,subprocess,shlex,datetime
root=Path('/private/tmp/nano-restored-three-node-v25-timeout-preparation-20261006/campaign');resources=json.loads((root/'resources.json').read_text());name=resources['remoteContainer']
code='''import subprocess,json,collections
p=subprocess.run(['sudo','-n','docker','logs','--tail','1200',NAME],capture_output=True,text=True)
assert p.returncode==0
rows=[]
for line in (p.stdout+p.stderr).splitlines():
 try:d=json.loads(line)
 except ValueError:continue
 if d.get('event') in ['test-capella-catalog-relay','test-capella-attestation-delivery','test-consensus-stream-completion','test-consensus-send-completion','test-core-recovery-stage','altair-ipc-ready','test-core-dispatch-transition','test-core-rpc-failure']:
  rows.append({k:v for k,v in d.items() if k in ['event','kind','headPriority','reason','stage','outcome','dependencyReason','elapsedMs','payloadsAdmitted','accepted','success','terminationReason','failureStage','phase','pending','highWater','timeMs','method','classification','httpStatus','cause','rpcCode']})
counts=collections.Counter((r.get('event'),r.get('kind'),r.get('headPriority'),r.get('reason'),r.get('dependencyReason'),r.get('outcome'),r.get('accepted'),r.get('phase'),r.get('cause'),r.get('httpStatus')) for r in rows)
print(json.dumps({'scope':'VPS last 1200 log lines, public fields filtered on VPS before transfer; not complete campaign','events':len(rows),'categories':[{'classification':list(k),'count':v} for k,v in counts.items()],'recentPublicRows':rows[-12:]}))
'''.replace('NAME',repr(name))
ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166'];d=json.loads(subprocess.check_output(ssh+[shlex.join(['python3','-c',code])],text=True));d['collectedAt']=datetime.datetime.now(datetime.timezone.utc).isoformat();Path('/private/tmp/nano-v25-live-vps-rpc-20261006.json').write_text(json.dumps(d,indent=2)+'\n')
print(json.dumps({'events':d['events'],'rpcFailures':[r for r in d['categories'] if r['classification'][0]=='test-core-rpc-failure'],'publicFieldsOnly':True},indent=2))
