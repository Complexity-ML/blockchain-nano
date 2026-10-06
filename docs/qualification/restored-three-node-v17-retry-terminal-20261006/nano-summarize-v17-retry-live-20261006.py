from pathlib import Path
from collections import Counter
import json
root=Path('/private/tmp/nano-restored-three-node-v17-retry-20261006');logs={'mac-native':root/'campaign/native.log','mac-docker':Path('/private/tmp/nano-v17-retry-mac-live-20261006.log'),'vps-docker':Path('/private/tmp/nano-v17-retry-vps-live-20261006.log')};summary={}
for name,path in logs.items():
 rows=[]
 for line in path.read_text().splitlines():
  try:rows.append(json.loads(line))
  except ValueError:pass
 cats=[x for x in rows if x.get('event')=='test-capella-catalog-relay'];heads=[x for x in cats if x.get('kind')=='block'];selection=[x.get('selectionAfter') for x in heads if x.get('selectionAfter')]
 summary[name]={'catalogAttempts':len(cats),'blockAttempts':len(heads),'payloadsAdmitted':sum(x.get('payloadsAdmitted',0) for x in cats),'rejected':sum(x.get('rejected',0) for x in cats),'terminations':dict(Counter(x.get('terminationReason',x.get('reason')) for x in cats)),'blockSelections':selection,'distinctBlockSelections':len(set(selection)),'lastBlockResult':heads[-1] if heads else None}
rows=[json.loads(l) for l in (root/'campaign/events.jsonl').read_text().splitlines()];summary['lastObservation']=next((x for x in reversed(rows) if x['event']=='observation'),None);summary['partial']=True;summary['networkQualified']=False;(root/'campaign/live-trace-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
for name in logs:
 x=summary[name];print(name,'attempts',x['catalogAttempts'],'block attempts',x['blockAttempts'],'payloads admitted',x['payloadsAdmitted'],'block positions',x['distinctBlockSelections'],'terminations',x['terminations'])
