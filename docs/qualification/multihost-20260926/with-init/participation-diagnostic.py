import json,hashlib
from pathlib import Path
source=Path('/private/tmp/nano-multihost-checkpoint.json');j=json.loads(source.read_text());blocks=j['store']['blocks'];byroot={b[0]:b for b in blocks};tip=max(blocks,key=lambda b:int(b[1][0],16));path=[]
while tip:
 path.append(tip);tip=byroot.get(tip[1][2])
rows=[]
for b in reversed(path):
 data=bytes.fromhex(b[2][0][2:]);n=lambda at,w=8:int.from_bytes(data[at:at+w],'little')
 h=176+64*64;p=h+96+32*64+8*64
 v=n(h+88,4);bal=n(h+92,4);prev=n(p,4);cur=n(p+4,4);scores=n(p+129,4);count=(bal-v)//121
 assert count==64 and cur-prev==count and scores-cur==count
 flags=lambda start:[i for i in range(count) if data[start+i]&2]
 rows.append({'slot':n(40),'root':b[0],'justified':n(p+49),'finalized':n(p+89),'previousTargetCount':len(flags(prev)),'currentTargetCount':len(flags(cur)),'previousTargetIndices':flags(prev),'currentTargetIndices':flags(cur)})
result={'sourceSha256':hashlib.sha256(source.read_bytes()).hexdigest(),'scope':'Read-only inspection of highest-slot branch in retained native checkpoint, fixed 64-validator fixture; not an independent SSZ or consensus verifier. Target flag counts, not weighted quorum proof.','rows':rows}
Path('/private/tmp/nano-participation-diagnostic.json').write_text(json.dumps(result,indent=2)+'\n')
for r in rows:
 if r['slot']>=24:print({k:v for k,v in r.items() if k not in ['root','previousTargetIndices','currentTargetIndices']})
