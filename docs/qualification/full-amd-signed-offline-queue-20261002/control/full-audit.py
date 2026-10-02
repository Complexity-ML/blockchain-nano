import json,sys,gzip,math,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);reference=json.loads(gzip.decompress(Path(sys.argv[2]).read_bytes()))
revision='0f53b42d3e78da85aa9a2c2f2993472747aabe8a'
assert (p/'source-revision.txt').read_text().strip()==revision
assert (p/'build-exit.txt').read_text().strip()=='0'
before=json.loads((p/'source-before.json').read_text());after=json.loads((p/'source-after.json').read_text())
assert before==after==reference and len(before)==15315
assert (p/'source-compare-exit.txt').read_text().strip()=='0'
registry=json.loads((p/'registry.json').read_text())['tests'];names=[t['name'] for t in registry]
assert len(names)==len(set(names))==427
programs=json.loads((p/'programs-before.json').read_text());assert programs==json.loads((p/'programs-after.json').read_text())
assert (p/'link-inputs-before.sha256').read_bytes()==(p/'link-inputs-after.sha256').read_bytes()
for t in registry:
 prop={v['name']:v['value'] for v in t['properties']};assert len(prop)==len(t['properties'])
 assert not any(x in prop for x in ['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION'])
 assert isinstance(prop.get('TIMEOUT'),(int,float)) and math.isfinite(prop['TIMEOUT']) and prop['TIMEOUT']>0
 assert t['command'] and t['command'][0] in programs
 # Hash manifest must bind every command/helper, not merely test names.
 for argument in t['command'][1:]:
  if str(argument).startswith('/') and Path(argument).name in ['nano','nano-altair-probe','nano-finality-crash-fixture']:assert argument in programs
suite=E.parse(p/'results.xml').getroot();assert suite.tag=='testsuite'
cases=suite.findall('testcase');actual=[c.attrib['name'] for c in cases]
assert len(cases)==len(set(actual))==427 and set(actual)==set(names) and int(suite.attrib['tests'])==427
assert all(c.attrib.get('status') in ['run','fail'] and c.find('skipped') is None for c in cases)
assert int(suite.attrib.get('skipped',0))==int(suite.attrib.get('disabled',0))==0
failed=[c.attrib['name'] for c in cases if c.find('failure') is not None or c.find('error') is not None]
assert int(suite.attrib.get('failures',0))+int(suite.attrib.get('errors',0))==len(failed)
assert all(c.attrib.get('status')!='fail' or c.attrib['name'] in failed for c in cases)
code=int((p/'ctest-exit.txt').read_text());assert int((p/'container-exit.txt').read_text())==code and (code==0)==(not failed)
print(json.dumps({'runtimeRevision':revision,'registered':427,'reported':427,'failed':failed,'skipped':0,'completeSelection':True,'sourceFiles':len(before),'programs':len(programs),'sourceProgramsAndLinkInputsUnchanged':True,'eestFixtureResultsIndependentlyAudited':False,'forkConformanceProven':False,'nativeOrWanProductionProven':False},indent=2))
if failed:raise SystemExit(1)
