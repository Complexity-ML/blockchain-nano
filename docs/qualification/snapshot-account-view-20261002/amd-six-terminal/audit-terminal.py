import json,sys,hashlib,gzip,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);revision='0f53b42d3e78da85aa9a2c2f2993472747aabe8a';build='/build/snapshot-accounts-0f53b42d-amd64-20261002/'
expected={
 'consensus_capella_signed_transition':(120,['nano-capella-transition-tests']),
 'consensus_capella_reindexed_validator_activation':(360,['nano-capella-transition-tests','--deposit-reorg-activation']),
 'consensus_hash_provider_parallel':(30,['nano-consensus-hash-provider-tests']),
 'consensus_hash_provider_unavailable':(30,['nano-consensus-hash-provider-tests','--provider-unavailable']),
 'consensus_capella_snapshot_account_primitives':(60,['nano-capella-snapshot-account-tests']),
 'consensus_capella_snapshot_cached_rows':(60,['nano-capella-snapshot-json-tests'])}
assert (p/'source-revision.txt').read_text().strip()==revision
assert (p/'build-exit.txt').read_text().strip()=='0'
registry=json.loads((p/'registry.json').read_text())['tests'];assert len(registry)==6 and {t['name'] for t in registry}==set(expected)
for t in registry:
 props={x['name']:x['value'] for x in t['properties']};deadline,args=expected[t['name']]
 assert props['TIMEOUT']==deadline and t['command']==[build+args[0],*args[1:]]
 assert not props.get('DISABLED') and not props.get('SKIP_RETURN_CODE') and not props.get('SKIP_REGULAR_EXPRESSION')
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==6 and {c.attrib['name'] for c in cases}==set(expected)
assert all(c.find('skipped') is None for c in cases)
failed=[c.attrib['name'] for c in cases if c.find('failure') is not None or c.find('error') is not None]
assert (p/'programs-before.sha256').read_bytes()==(p/'programs-after.sha256').read_bytes()
programs={line.split()[1]:line.split()[0] for line in (p/'programs-before.sha256').read_text().splitlines()}
assert len(programs)==4 and set(programs)=={build+args[0] for _,args in expected.values()}
before=json.loads((p/'source-before.json').read_text());after=json.loads((p/'source-after.json').read_text());assert before==after and len(before)==15315
expected_source=json.loads(gzip.decompress(Path(sys.argv[2]).read_bytes()));assert before==expected_source
assert (p/'source-compare-exit.txt').read_text().strip()=='0'
code=int((p/'ctest-exit.txt').read_text());assert int((p/'container-exit.txt').read_text())==code and (code==0)==(len(failed)==0)
print(json.dumps({'runtimeRevision':revision,'cases':6,'failed':failed,'skipped':0,'sourceFiles':len(before),'compiledPrograms':len(programs),'originalTimeoutsPreserved':True,'sourceAndProgramsUnchanged':True,'durations':{c.attrib['name']:float(c.attrib['time']) for c in cases},'completeRegressionProven':False,'wholeNodeProductionProven':False},indent=2))
