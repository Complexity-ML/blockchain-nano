"""Selected compiled software audit on VPS; no runtime, SDK walk, or key read."""
from pathlib import Path
import os, stat, json, hashlib, re, datetime, calendar

F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
ROOT = Path('/home/ubuntu/current-amd-rpc-runtime-relink-20261017-01')
held = {}

def material(path):
    path = Path(path)
    assert path.is_absolute() and path.resolve() == path
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        s = os.fstat(fd)
        assert stat.S_ISREG(s.st_mode) and s.st_nlink == 1 and 0 <= s.st_size <= 67108864
        row = {k:getattr(s,k) for k in F}
        parts, total = [], 0
        while True:
            b = os.read(fd,1048576)
            if not b: break
            total += len(b)
            assert total <= s.st_size
            parts.append(b)
        body = b''.join(parts)
        assert len(body) == s.st_size and row == {k:getattr(os.fstat(fd),k) for k in F} == {k:getattr(path.lstat(),k) for k in F}
        return dict(row,sha256=hashlib.sha256(body).hexdigest()), body
    finally:
        os.close(fd)

def load(path,sha,size):
    row, body = material(path)
    assert row['sha256'] == sha and len(body) == size
    held[str(path)] = row
    return json.loads(body)

def ns(text):
    m = re.fullmatch(r'(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})[.](\d{9}) ([+-])(\d{2})(\d{2})', text)
    assert m is not None
    y,mo,d,h,mi,s = map(int,m.groups()[:6])
    assert int(m[9]) <= 23 and int(m[10]) <= 59
    zone = (int(m[9])*60+int(m[10]))*60
    if m[8] == '-': zone = -zone
    return (calendar.timegm(datetime.datetime(y,mo,d,h,mi,s).timetuple())-zone)*1000000000+int(m[7])

def guest_row(row):
    assert set(row) == {'material','bytes','sha256'}
    r = row['material']
    assert isinstance(r,list) and len(r) == 9 and all(isinstance(x,str) for x in r)
    assert all(re.fullmatch('[0-9]+',r[i]) for i in (0,1,3,4,5,6)) and re.fullmatch('[0-9a-f]+',r[2])
    values = [int(r[0]),int(r[1]),int(r[2],16),int(r[3]),int(r[4]),int(r[5]),int(r[6]),ns(r[7]),ns(r[8])]
    assert values[3] == 1 and values[4] == values[5] == 1000 and values[6] == row['bytes'] and stat.S_ISREG(values[2])
    return dict(zip(F,values),sha256=row['sha256'])

def select(path,expected):
    row,body = material(path)
    assert row == expected
    held[str(path)] = row
    return {'path':str(path),'sha256':row['sha256'],'bytes':len(body)}

assert os.getuid() == os.getgid() == 1000
binding = load(ROOT/'outputs/software-admission.private.json','0a55dfa0136e43a41cd4533ee13ebddaad82fa38e890b5607bd2fd861be43de5',243309)
assert binding['format'] == 'nano-current-linux-runtime-relink-software-binding-v1' and binding['owner'] == 'vps-docker'
assert binding['sourceRevisionDeclared'] == 'd26e4744416d033dad5a2519cc2ab8899b838faf' and binding['oldAuthoritiesRefreshed'] is False
core = select(ROOT/'baseline/libnano_core.a',guest_row(binding['coreArchive']))
assert core['sha256'] == '2678b628528295dd7d729e8917572d3076a1068399e58b12f97fa3849bb497bd' and core['bytes'] == 40805844
roles = {'nano':'nano','bootstrapAudit':'nano-capella-isolated-bootstrap-audit','stoppedCapture':'nano-capella-isolated-stopped-capture','lifecycleTool':'nano-capella-deposit-audit','lifecycleSubmit':'nano-capella-lifecycle-submit'}
programs = {}
assert len(binding['programs']) == len(roles)
for p in binding['programs']:
    role = p['role']
    assert role in roles and role not in programs and p['target'] == roles[role] and p['softwarePath'] == '/outputs/'+roles[role]
    programs[role] = select(ROOT/'outputs'/roles[role],guest_row(p['software']))
legacy = load(Path('/home/ubuntu/current-amd-final-runtime-20261016-01/runtime-bundle-admission.private.json'),'5c5bb5928a00fe4d5fafef2028c03c450aa25c61855b595fb0fe3c2c9e5226ab',8589)
selected = [(p,row) for p,row in legacy['files'].items() if p.endswith('/nano-p2p')]
assert len(selected) == 1
path,row = selected[0]
assert path == '/home/ubuntu/current-amd-final-runtime-20261016-01/network-build/nano-p2p'
programs['p2p'] = select(Path(path),row)
assert programs['p2p']['sha256'] == 'b87645b74fb4b3139d4d9d9f43443e2afaff6b773375941a7fb624f113d88795' and programs['p2p']['bytes'] == 18964752
for path,row in held.items(): assert material(Path(path))[0] == row
print(json.dumps({'format':'nano-current-amd-rpc-software-selected-fence-v1','passed':True,'coreArchive':core,'programs':programs,'originalNineAndShaMatched':True,'selectedSoftwareFiles':7,'descriptorFiles':2,'oldAuthoritiesRefreshed':False,'privateInputsTransferred':False,'programsExecuted':0,'sdkFilesRead':0,'newKeys':0,'runtimeStarted':False,'productionQualified':False},sort_keys=True))
