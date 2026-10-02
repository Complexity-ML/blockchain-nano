import json,sys,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);expected={'consensus_capella_reindexed_validator_activation':360,'consensus_hash_provider_parallel':30,'consensus_hash_provider_unavailable':30}
registry=json.loads((p/'registry.json').read_text())['tests'];assert len(registry)==3 and {t['name'] for t in registry}==set(expected)
for t in registry:
 props={x['name']:x['value'] for x in t['properties']}
 assert props['TIMEOUT']==expected[t['name']]
 assert t['command'] and not props.get('DISABLED') and not props.get('SKIP_RETURN_CODE') and not props.get('SKIP_REGULAR_EXPRESSION')
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==3 and {c.attrib['name'] for c in cases}==set(expected)
assert all(c.find('skipped') is None for c in cases)
failed=[c.attrib['name'] for c in cases if c.find('failure') is not None or c.find('error') is not None]
assert (p/'programs-before.sha256').read_bytes()==(p/'programs-after.sha256').read_bytes()
assert (p/'source-before.json').read_bytes()==(p/'source-after.json').read_bytes()
assert (p/'source-compare-exit.txt').read_text().strip()=='0' and (p/'build-exit.txt').read_text().strip()=='0'
code=int((p/'ctest-exit.txt').read_text());assert int((p/'container-exit.txt').read_text())==code and (code==0)==(len(failed)==0)
print(json.dumps({'cases':3,'failed':failed,'skipped':0,'originalTimeoutsPreserved':True,'sourceAndProgramsUnchanged':True,'completeRegressionProven':False},indent=2))
