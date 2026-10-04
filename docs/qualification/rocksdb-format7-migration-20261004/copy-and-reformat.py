import pathlib,json,hashlib,fcntl,shutil,subprocess
root=pathlib.Path(__file__).parent;source=pathlib.Path('/tmp/nano-rpc-wan-majority-d1597d9a-20261004/prepared/mac-native/data');target=root/'migrated-copy';assert not target.exists()
dbs=sorted(p for p in source.iterdir() if p.is_dir() and (p/'CURRENT').is_file());assert len(dbs)==46
locks=[]
try:
 for db in dbs:
  f=(db/'LOCK').open('r+');fcntl.lockf(f,fcntl.LOCK_EX|fcntl.LOCK_NB);locks.append(f)
 def manifest():return {str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
 before=manifest();(root/'source-before.json').write_text(json.dumps(before,sort_keys=True));shutil.copytree(source,target)
 results={}
 for db in dbs:
  expected=subprocess.check_output([str(root/'reformat-native'),'read',str(db)],text=True,timeout=30).strip()
  actual=subprocess.check_output(['nice','-n','10',str(root/'reformat-native'),'reformat-copy',str(target/db.name)],text=True,timeout=60).strip()
  assert expected==actual,db.name;results[db.name]={'keys':int(actual.split()[0]),'logicalSha256':actual.split()[1]}
 assert before==manifest();(root/'native-result.json').write_text(json.dumps({'originalFilesUnchanged':len(before),'databasesMigrated':len(results),'logicalContents':results,'copiedPath':str(target),'productionQualified':False},indent=2)+'\n');print('46 copied DBs migrated; original331files and all logical hashes unchanged',flush=True)
finally:
 for f in locks:f.close()
