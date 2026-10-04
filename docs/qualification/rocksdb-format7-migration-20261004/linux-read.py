import pathlib,json,hashlib,subprocess
root=pathlib.Path('/output');base=root/'migrated-copy';expected=json.loads((root/'native-result.json').read_text());before={str(f.relative_to(base)):hashlib.sha256(f.read_bytes()).hexdigest() for f in base.rglob('*') if f.is_file()};rows={}
for name,record in expected['logicalContents'].items():
 line=subprocess.check_output([str(root/'read-linux'),'read',str(base/name)],text=True,timeout=30).strip();keys,digest=line.split();assert int(keys)==record['keys'] and digest==record['logicalSha256'],name;rows[name]=record
assert before=={str(f.relative_to(base)):hashlib.sha256(f.read_bytes()).hexdigest() for f in base.rglob('*') if f.is_file()}
print(json.dumps({'databasesReadOnLinux':len(rows),'keysAndValuesExactlyMatched':True,'migratedFilesUnchanged':len(before),'logicalContents':rows,'productionQualified':False},indent=2))
