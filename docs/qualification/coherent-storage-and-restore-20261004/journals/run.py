import pathlib,json,hashlib,subprocess,shlex
R=pathlib.Path('/tmp/nano-migrated-journal-audit-20261004');M=pathlib.Path('/tmp/nano-rocksdb-migration-proof-20261004')
SOURCE=pathlib.Path('/tmp/nano-rpc-wan-majority-d1597d9a-20261004/prepared/mac-native/data');COPY=M/'migrated-copy'
RECEIPT=pathlib.Path('/Users/boris/Dev/blockchain-nano/docs/qualification/rpc-majority-vote-inspection-20261004/receipt.json')
CONFIG=pathlib.Path('/tmp/nano-majority-vote-inspection-20261004/node-native.json')
GENESIS=SOURCE.parent/'genesis.json'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def tree(root):return {str(p.relative_to(root)):sha(p) for p in sorted(root.rglob('*')) if p.is_file()}
def save(n,x):(R/n).write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
receipt=json.loads(RECEIPT.read_text());records=[]
for line in pathlib.Path('/tmp/nano-rpc-wan-majority-d1597d9a-20261004/native.log').read_text().splitlines():
 try:r=json.loads(line)
 except ValueError:continue
 if r.get('event')=='capella-runtime-stopped':records.append(r)
assert records==[receipt], 'independent Git receipt differs from originally stopped runtime receipt'
assert subprocess.check_output(['git','-C','/tmp/nano-production-coherent-20261004','rev-parse','HEAD'],text=True).strip()=='efbeb9bca3d42e41edb9cb81afba98c0b953a90f'
assert not subprocess.check_output(['git','-C','/tmp/nano-production-coherent-20261004','status','--porcelain'])
inputs=json.loads(pathlib.Path('/tmp/nano-coherent-native-reports-20261004/inputs-after-tests.json').read_text());assert all(sha(pathlib.Path(p))==h for p,h in inputs.items())
source_before=tree(SOURCE);copy_before=tree(COPY)
assert source_before==json.loads((M/'source-before.json').read_text()) and copy_before==json.loads((M/'migrated-before-linux.json').read_text())
save('fixture-before.json',{'original':source_before,'migrated':copy_before})
pins={str(p):sha(p) for p in (RECEIPT,CONFIG,GENESIS,R/'inspect-native',R/'inspect.cpp')};save('pins.json',pins)
for label,path in [('original',SOURCE),('migrated',COPY)]:
 args=['/usr/bin/sandbox-exec','-p','(version 1)(allow default)(deny network*)(deny file-write*)',str(R/'inspect-native'),str(GENESIS),str(CONFIG),str(path),str(RECEIPT)]
 save(label+'-command.json',args)
 with (R/(label+'.json')).open('w') as out,(R/(label+'.stderr')).open('w') as err:r=subprocess.run(args,stdout=out,stderr=err,timeout=120)
 (R/(label+'-exit.txt')).write_text(str(r.returncode)+'\n');assert r.returncode==0, label+' inspector failed; see retained stderr'
 a=json.loads((R/(label+'.json')).read_text());assert a['signerCount']==22 and a['negativeBindingsRejected']==66 and a['coreExternalCommitmentMatched'] and a['coreWitnessMatchesCommittedSnapshot']
assert json.loads((R/'original.json').read_text())==json.loads((R/'migrated.json').read_text())
assert tree(SOURCE)==source_before and tree(COPY)==copy_before
assert all(sha(pathlib.Path(p))==h for p,h in inputs.items()) and all(sha(pathlib.Path(p))==h for p,h in pins.items())
save('fixture-after.json',{'original':tree(SOURCE),'migrated':tree(COPY)})
save('result.json',dict(exit=0,originalFilesUnchanged=len(source_before),migratedFilesUnchanged=len(copy_before),signers=22,negativeBindingsPerFixture=66,originalMigratedJournalStatesExactlyEqual=True,coreWitnessMatchedExternalToken=True,nativeRuntimeInputsUnchanged=len(inputs),signaturesRequested=0,dutiesResumed=False,linuxAuditPerformed=False,productionQualified=False))
print('Original and migrated signer/witness audits passed with66 negative bindings each')
