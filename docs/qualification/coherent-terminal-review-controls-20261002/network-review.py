import sys,json,re,hashlib,xml.etree.ElementTree as E
from pathlib import Path
source,reports,prefix=Path(sys.argv[1]),Path(sys.argv[2]),sys.argv[3]
cmake=(source/'network/CMakeLists.txt').read_text()
expected=dict(re.findall(r'add_test\(NAME (\S+) COMMAND (\S+)\)',cmake));assert len(expected)==21
registry=json.loads((reports/'registry.json').read_text());tests=registry['tests'];assert len(tests)==len(expected)
assert len({t['name'] for t in tests})==len(tests);assert {t['name'] for t in tests}==set(expected)
limits=dict(re.findall(r'set_tests_properties\((\S+) PROPERTIES TIMEOUT (\d+)',cmake))
for t in tests:
 assert t['command']==[prefix+'/'+expected[t['name']]]
 timeout=[p['value'] for p in t['properties'] if p['name']=='TIMEOUT']
 assert timeout==([float(limits[t['name']])] if t['name'] in limits else [])
command=json.loads((reports/'command.json').read_text());assert command[-2:]==['bash','/reports/run.sh']
assert '--timeout 120' in (reports/'run.sh').read_text()
assert (reports/'programs-before.sha256').read_bytes()==(reports/'programs-after.sha256').read_bytes()
hashes={}
for row in (reports/'programs-before.sha256').read_text().splitlines():
 h,p=row.split(None,1);assert p not in hashes and re.fullmatch('[0-9a-f]{64}',h);hashes[p]=h
assert all(t['command'][0] in hashes for t in tests)
assert all((reports/n).read_text().strip()=='0' for n in ['ctest-exit.txt','container-exit.txt','audit-exit.txt'])
root=E.parse(reports/'results.xml').getroot();cases=root.findall('testcase');assert root.tag=='testsuite';assert int(root.get('tests'))==21
assert {c.get('name') for c in cases}==set(expected) and len(cases)==21
assert all(c.get('status')=='run' and not c.findall('failure') and not c.findall('error') and not c.findall('skipped') for c in cases)
print(json.dumps({'registered':21,'reported':21,'sourceNamedSelectionMatched':True,'originalExplicitTimeoutsPreserved':True,'bootstrapFallbackSeconds':120,'programsUnchanged':True,'productionQualified':False},indent=2))
