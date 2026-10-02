import copy,gzip,hashlib,importlib.util,json
from pathlib import Path
control=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('fixture_audit',control/'eest-fixture-audit.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
source=Path('/tmp/nano-snapshot-account-view-candidate-20261002');ref=Path('/Users/boris/Dev/blockchain-nano/docs/qualification/snapshot-account-view-20261002/source-manifest.json.gz');log=Path('/tmp/nano-full-2d2764de-arm64-reports-20261002/LastTest.log')
before=hashlib.sha256(log.read_bytes()).hexdigest()
expected=module.inventory(source,json.loads(gzip.decompress(ref.read_bytes())));rows,summary=module.ctest_rows(log)
results=[]
for name in ['missing-variant','duplicate-variant','foreign-variant','self-consistent-wrong-root','wrong-validation-reason','executed-validation-transaction','wrong-gas-with-true-match']:
 changed=copy.deepcopy(rows)
 if name=='missing-variant':changed.pop()
 if name=='duplicate-variant':changed[-1]=copy.deepcopy(changed[0])
 if name=='foreign-variant':changed[0]['variant']=999999
 if name=='self-consistent-wrong-root':changed[0]['stateRoot']={'actual':'0x'+'00'*32,'expected':'0x'+'00'*32}
 if name=='wrong-validation-reason':next(r for r in changed if 'validation' in r)['validation']['actual']='TransactionException.FOREIGN'
 if name=='executed-validation-transaction':next(r for r in changed if 'validation' in r)['validation']['transactionExecuted']=True
 if name=='wrong-gas-with-true-match':next(r for r in changed if 'gasUsedMatches' in r)['gasUsed']='0xffffffffffffffff'
 try:module.audit(changed,summary,expected)
 except (AssertionError,KeyError):results.append({'mutation':name,'rejected':True})
 else:raise RuntimeError('Mutation accepted: '+name)
assert hashlib.sha256(log.read_bytes()).hexdigest()==before
print(json.dumps({'positiveHistoricalReportPassed':True,'negativeControls':results,'historicalReportSha256Unchanged':before,'nativeEvmExecutedAgain':False,'currentFullRunQualification':False},indent=2))
