import pathlib,json,hashlib,subprocess
root=pathlib.Path(__file__).resolve().parent
fixtures={'original':pathlib.Path('/tmp/nano-rpc-wan-majority-d1597d9a-20261004/prepared/mac-native/data'),'migrated':pathlib.Path('/tmp/nano-rocksdb-migration-proof-20261004/migrated-copy')}
def manifest(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
for name,p in fixtures.items():
 before=manifest(p);(root/(name+'-before.json')).write_text(json.dumps(before,sort_keys=True)+'\n')
 cmd=['/usr/bin/sandbox-exec','-p','(version 1)(allow default)(deny network*)(deny file-write*)','/tmp/nano-storage-profile-20261004',str(p)]
 with (root/(name+'.json')).open('w') as out,(root/(name+'.stderr')).open('w') as err:r=subprocess.run(cmd,stdout=out,stderr=err)
 (root/(name+'-exit.txt')).write_text(str(r.returncode)+'\n');assert r.returncode==0
 after=manifest(p);assert before==after;(root/(name+'-after.json')).write_text(json.dumps(after,sort_keys=True)+'\n')
 print(name,'unchanged files',len(after))
empty=root/'empty-node';empty.mkdir()
negative=subprocess.run(['/usr/bin/sandbox-exec','-p','(version 1)(allow default)(deny network*)(deny file-write*)','/tmp/nano-storage-profile-20261004',str(empty)],capture_output=True,text=True)
(root/'negative-no-node.json').write_text(json.dumps({'exit':negative.returncode,'stderr':negative.stderr,'stdout':negative.stdout},indent=2)+'\n');assert negative.returncode==1 and 'No Nano databases found' in negative.stderr
(root/'result.json').write_text(json.dumps({'exit':0,'fixturesUnchanged':True,'negativeEmptyDirectoryRefused':True,'pruningImplemented':False,'binarySHA256':hashlib.sha256(pathlib.Path('/tmp/nano-storage-profile-20261004').read_bytes()).hexdigest()},indent=2)+'\n')
