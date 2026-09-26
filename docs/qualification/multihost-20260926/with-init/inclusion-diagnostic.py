import json,collections
from pathlib import Path
j=json.load(open('/private/tmp/nano-multihost-checkpoint.json'));seen=set();rows=[]
def block_slot(item):
 b=bytes.fromhex(item[1][2:]);m=int.from_bytes(b[:4],'little');return int.from_bytes(b[m:m+8],'little')
for root,hx in sorted(j['signedBlocks'],key=block_slot):
 b=bytes.fromhex(hx[2:]);u=lambda at,w=4:int.from_bytes(b[at:at+w],'little');msg=u(0);slot=u(msg,8);proposer=u(msg+8,8);body=msg+u(msg+80);start=body+u(body+208);end=body+u(body+212)
 if start==end:continue
 count=u(start)//4
 for i in range(count):
  lo=start+u(start+4*i);hi=start+u(start+4*(i+1)) if i+1<count else end
  vote=b[lo:hi];n=lambda a:int.from_bytes(vote[a:a+8],'little')
  key=vote.hex()
  if key in seen:continue
  seen.add(key);rows.append({'blockSlot':slot,'proposer':proposer,'host':['native','mac-docker','vps'][proposer%3],'voteSlot':n(4),'sourceEpoch':n(52),'targetEpoch':n(92),'targetRoot':'0x'+vote[100:132].hex(),'delaySlots':slot-n(4)})
rows.sort(key=lambda r:(r['blockSlot'],r['voteSlot']))
Path('/private/tmp/nano-inclusion.json').write_text(json.dumps(rows,indent=2)+'\n')
for e in range(2,8):
 a=[r for r in rows if r['targetEpoch']==e];print(e,'votes',len(a),'delay',dict(sorted(collections.Counter(r['delaySlots'] for r in a).items())),'sources',dict(collections.Counter(r['sourceEpoch'] for r in a)),'targets',len(set(r['targetRoot'] for r in a)))
