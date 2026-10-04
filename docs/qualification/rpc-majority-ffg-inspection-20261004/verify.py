import json,pathlib,hashlib,copy
p=pathlib.Path(__file__).parent
x=json.loads((p/'states.json').read_text())
keys=['previousJustified','currentJustified','finalized','justificationBits']
checks=[]
for row in x['boundaries']:
 s=row['pre'];e=s['epoch'];oldprev=s['previousJustified'];oldcurrent=s['currentJustified'];bits=(s['justificationBits']<<1)&15
 out={k:copy.deepcopy(s[k]) for k in keys};out['previousJustified']=oldcurrent
 total=int(s['totalStake'],16)
 if int(s['previousTargetStake'],16)*3>=total*2:out['currentJustified']={'epoch':e-1,'root':s['previousTargetRoot']};bits|=2
 if int(s['currentTargetStake'],16)*3>=total*2:out['currentJustified']={'epoch':e,'root':s['currentTargetRoot']};bits|=1
 if bits&14==14 and oldprev['epoch']+3==e:out['finalized']=oldprev
 if bits&6==6 and oldprev['epoch']+2==e:out['finalized']=oldprev
 if bits&7==7 and oldcurrent['epoch']+2==e:out['finalized']=oldcurrent
 if bits&3==3 and oldcurrent['epoch']+1==e:out['finalized']=oldcurrent
 out['justificationBits']=bits
 for name in ['computedPost','persistedPostBlock']:
  assert out=={k:row[name][k] for k in keys},(row['boundary'],name,out,row[name])
 checks.append({'boundarySlot':row['boundary'],'previousParticipants':len(s['previousIndices']),'currentParticipants':len(s['currentIndices']),'expected':out,'computedAndPersistedMatch':True})
base=pathlib.Path('/tmp/nano-rpc-wan-majority-d1597d9a-20261004/prepared/mac-native/data');before=json.loads(pathlib.Path('/tmp/nano-majority-vote-inspection-20261004/data-before.json').read_text());after={str(f.relative_to(base)):hashlib.sha256(f.read_bytes()).hexdigest() for f in base.rglob('*') if f.is_file()};assert before==after
result={'boundariesVerified':len(checks),'checks':checks,'inputFilesUnchanged':len(after),'historicalForkChoiceVerified':False,'productionQualified':False,'reference':'https://github.com/ethereum/consensus-specs/blob/master/specs/phase0/beacon-chain.md#justification-and-finalization'}
(p/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
