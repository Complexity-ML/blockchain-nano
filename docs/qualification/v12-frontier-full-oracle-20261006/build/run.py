from pathlib import Path
import json,hashlib,subprocess,collections
base=Path('/private/tmp');old=base/'nano-v12-frontier-oracle-20261006';audit=base/'nano-v12-vote-cache-post-stop-native-diagnostic-20261006';campaign=base/'nano-restored-three-node-v12-vote-cache-20261006/campaign';out=base/'nano-v12-frontier-oracle-result-20261006';out.mkdir(exist_ok=True)
terminal=json.loads((campaign/'terminal-exit.json').read_text());assert terminal['originalSession']==70646 and terminal['exitCode']==1
req=json.loads((old/'request.json').read_text());req['genesis']=str(campaign.parent/'config/genesis.json');maps={}
def hashes(d):return {str(p.relative_to(d)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(d.rglob('*')) if p.is_file()}
for source in req['sources']:
 name=source['name'];assert json.loads((audit/(name+'-exit.json')).read_text())['exitCode']==0
 cfg=audit/(name+'-config.json');data=campaign.parent/name/'data';expected=json.loads((audit/(name+'-before.json')).read_text());assert hashes(data)==expected
 maps[name]=expected;source.update(config=str(cfg),database=str(data/'chain'),checkpoint=json.loads((audit/(name+'-capture.json')).read_text())['restart'])
(out/'request.json').write_text(json.dumps(req,indent=2)+'\n');(out/'data-before.json').write_text(json.dumps(maps,indent=2)+'\n')
cmd=[str(old/'diagnose'),str(out/'request.json')];(out/'command.json').write_text(json.dumps(cmd,indent=2)+'\n')
with (out/'diagnostic.json').open('w') as f,(out/'error.log').open('w') as e:r=subprocess.run(cmd,stdout=f,stderr=e)
(out/'exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0
for source in req['sources']:assert hashes(Path(source['database']).parent)==maps[source['name']]
d=json.loads((out/'diagnostic.json').read_text());summary={'classes':dict(collections.Counter(x['classification'] for x in d['rows'])),'rows':len(d['rows']),'binarySHA256':hashlib.sha256((old/'diagnose').read_bytes()).hexdigest(),'sourceSHA256':hashlib.sha256((old/'diagnose.cpp').read_bytes()).hexdigest(),'hashesUnchanged':True,'scope':'16 newest signed votes per Mac source on opposite stopped v12 Mac store','activeNetworkCauseProven':False,'productionQualified':False}
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
