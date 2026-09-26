# Offline diagnostic for the archived synthetic Bellatrix campaign; not a consensus verifier.
import json,hashlib,gzip,sys
from pathlib import Path
path=Path(sys.argv[1]); raw=gzip.decompress(path.read_bytes()); p=json.loads(raw)['store']; limits=list(map(lambda x:int(x,16),p['profile'][0]['stateLimits'])); h=176+64*limits[0]; a=h+96+32*limits[1]+8*limits[2]
blocks={x[0]:x for x in p['blocks']}; root=p['finalized'][1]; ancestors=set()
while root in blocks and root not in ancestors:
 ancestors.add(root);root=blocks[root][1][2]
rows=[]
for root,record in blocks.items():
 b=bytes.fromhex(record[2][0][2:]); n=lambda at,w=8:int.from_bytes(b[at:at+w],'little'); slot=n(40);epoch=slot//8
 if not 40<=slot<=88:continue
 v,bal,prev,cur,scores=[n(i,4) for i in [h+88,h+92,a,a+4,a+129]]; count=(bal-v)//121
 assert (bal-v)%121==0 and cur-prev==scores-cur==count and (prev-bal)==8*count
 total=pt=ct=0
 for i in range(count):
  off=v+121*i;eff=n(off+80);active=n(off+97)<=epoch<n(off+105);previous=n(off+97)<=epoch-1<n(off+105)
  if active:total+=eff
  if not b[off+88]:
   if previous and b[prev+i]&2:pt+=eff
   if active and b[cur+i]&2:ct+=eff
 rows.append(dict(slot=slot,root=root,ancestorOfFinalized11=root in ancestors,validators=count,previousTargetWeight=pt,currentTargetWeight=ct,totalActiveWeight=total,previousQuorum=3*pt>=2*total,currentQuorum=3*ct>=2*total,justificationBits=b[a+8],previousJustified=n(a+9),currentJustified=n(a+49),finalized=n(a+89)))
print(json.dumps({'inputSha256':hashlib.sha256(raw).hexdigest(),'limits':limits,'rows':sorted(rows,key=lambda r:(r['slot'],r['root']))},indent=2))
