import gzip,hashlib,json,sys
from pathlib import Path
PREFIX='/work/tests/fixtures/ethereum-official/'
EMPTY_LOGS='0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347'
def inventory(source,reference):
 root=source/'tests/fixtures/ethereum-official';expected={};files=sorted(root.rglob('*.json'))
 for f in files:
  relative=str(f.relative_to(source));assert hashlib.sha256(f.read_bytes()).hexdigest()==reference[relative],f
  for name,test in json.loads(f.read_text()).items():
   assert all(k in test for k in ['pre','env','post'])
   for fork,variants in test['post'].items():
    for i,value in enumerate(variants):
     key=(str(f.relative_to(root)),name,fork,i);assert key not in expected
     assert 'txbytes' in value and 'hash' in value and 'logs' in value
     expected[key]=value
 assert len(files)==317 and len(expected)==5387
 return expected

def ctest_rows(path):
 active=found=ended=False;rows=[];summary=None
 with path.open() as f:
  for line in f:
   import re
   if re.fullmatch(r'\d+/\d+ Testing: ethereum_official_regressions\n?',line):
    assert not found;found=active=True;continue
   if not active:continue
   if line.startswith('Test time ='):ended=True;active=False;continue
   assert not re.match(r'\d+/\d+ Testing:',line),'Unterminated EEST output'
   if not line.startswith('{'):continue
   row=json.loads(line)
   if 'summary' in row:assert summary is None;summary=row['summary']
   else:assert summary is None;rows.append(row)
 assert found and ended and summary is not None
 return rows,summary

def audit(rows,summary,expected):
 seen=set();execution=validation=0
 for row in rows:
  assert row['file'].startswith(PREFIX)
  key=(row['file'][len(PREFIX):],row['case'],row['fork'],row['variant'])
  assert key in expected and key not in seen;seen.add(key);post=expected[key]
  assert row['status']=='matched'
  assert row['stateRoot']['expected']==post['hash']==row['stateRoot']['actual']
  exception=post.get('expectException','')
  if exception:
   validation+=1;v=row['validation']
   assert v['expected']==exception and v['actual'] and v['actual'] in exception.split('|')
   assert v['transactionExecuted'] is False and v['stateUnchanged'] is True
   assert post['logs']==EMPTY_LOGS
  else:
   execution+=1;assert 'validation' not in row
   assert row['logsHash']['expected']==post['logs']==row['logsHash']['actual']
   assert row['transactionHashMatches'] is True
   receipt=post.get('receipt',{})
   if 'transactionHash' in receipt:assert row['transactionHash']==receipt['transactionHash']
   if 'rlp' in receipt:assert row['receiptMatches'] is True
   if 'cumulativeGasUsed' in receipt:
    assert row['gasUsedMatches'] is True and row['expectedGasUsed']==receipt['cumulativeGasUsed']
    def number(x):return x if isinstance(x,int) else int(x,16) if x.startswith('0x') else int(x)
    assert number(row['gasUsed'])==number(receipt['cumulativeGasUsed'])
 assert seen==set(expected),'Missing or substituted fixture variant'
 assert (execution,validation)==(4882,505)
 assert summary['matched']==5387 and summary['files']==317 and summary['validationMatched']==505
 assert summary.get('executionMatched',execution)==execution
 assert all(summary[k]==0 for k in ['mismatched','unsupported','errors'])
 return {'exactFixtureVariants':5387,'fixtureFiles':317,'execution':execution,'validation':validation,'identitySetAndRootsLogsExpectedErrorsAndAvailableGasBoundToFixture':True,'transactionHashAndReceiptMatchBooleansIndependentlyReexecuted':False,'completeForkConformance':False}

if __name__=='__main__':
 source=Path(sys.argv[1]);reference=json.loads(gzip.decompress(Path(sys.argv[2]).read_bytes()))
 expected=inventory(source,reference);rows,summary=ctest_rows(Path(sys.argv[3]))
 print(json.dumps(audit(rows,summary,expected),indent=2))
