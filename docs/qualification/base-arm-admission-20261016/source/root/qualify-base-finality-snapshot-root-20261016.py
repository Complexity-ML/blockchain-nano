#!/usr/bin/env python3
import hashlib,json,pathlib,subprocess,sys
E=pathlib.Path(__file__).resolve().parent
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
P=E/'bridge-production-delta-source-20261016'
O=E/'base-finality-snapshot-qualification-root-20261016-01'
NODE='/opt/homebrew/bin/node'
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def save(name,value):(O/name).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def run(argv,name,cwd):
 q=subprocess.run(argv,cwd=cwd,stdin=subprocess.DEVNULL,capture_output=True,timeout=45)
 (O/(name+'-stdout.log')).write_bytes(q.stdout);(O/(name+'-stderr.log')).write_bytes(q.stderr)
 save(name+'-command.json',dict(argv=argv,cwd=str(cwd),exitCode=q.returncode))
 return q
manifest_raw=(P/'manifest.json').read_bytes()
assert pin(manifest_raw)=={'sha256':'6296b6d535260487b966b02d6066ad8d171b606359657f74312ce9b88de0bb68','bytes':2033}
mf=json.loads(manifest_raw)
for row in mf['payloads']:assert pin((P/row['path']).read_bytes())=={k:row[k] for k in ('sha256','bytes')}
mode=sys.argv[1]
if mode=='baseline':
 O.mkdir(mode=0o700);B=O/'baseline';B.mkdir()
 for row in mf['sourceOverlays']:
  old=(R/row['path']).read_bytes();assert pin(old)==row['before']
  (B/pathlib.Path(row['path']).name).write_bytes((P/row['sourcePath']).read_bytes() if row['path'].endswith('.test.mjs') else old)
 version=run([NODE,'--version'],'node-version',B);assert version.returncode==0
 q=run([NODE,'--test','--test-reporter=tap','--test-name-pattern=^advanced finalized','finality.test.mjs'],'baseline-new-regressions',B)
 text=q.stdout.decode();assert q.returncode==1
 assert text.count('error: \'Missing expected rejection.\'')==3,text
 assert '# fail 4' in text,text
 save('baseline-public.json',dict(format='nano-base-finality-snapshot-causal-baseline-v1',passed=True,expectedBaselineExit=1,missingExpectedRejections=3,newSubtestsFailed=3,parentGroupFailed=True,oldSource=mf['sourceOverlays'][0]['before'],newTest=mf['sourceOverlays'][1]['after'],oldTestsExecuted=False,networkAccessUsed=False,productionQualified=False))
 print('Baseline reproduced: three missing refusals; parent group failed as expected.')
elif mode=='baseline-readback':
 B=O/'baseline';q=json.loads((O/'baseline-new-regressions-command.json').read_bytes())
 assert q['exitCode']==1
 text=(O/'baseline-new-regressions-stdout.log').read_text()
 assert text.count('AssertionError [ERR_ASSERTION]: Missing expected rejection.')==3 and 'ℹ fail 4' in text
 for row in mf['sourceOverlays']:
  assert pin((B/pathlib.Path(row['path']).name).read_bytes())==(row['after'] if row['path'].endswith('.test.mjs') else row['before'])
 save('baseline-public.json',dict(format='nano-base-finality-snapshot-causal-baseline-v1',passed=True,expectedBaselineExit=1,missingExpectedRejections=3,newSubtestsFailed=3,parentGroupFailed=True,oldSource=mf['sourceOverlays'][0]['before'],newTest=mf['sourceOverlays'][1]['after'],oldTestsExecuted=False,networkAccessUsed=False,wrapperInitialFailure='TAP-specific parser expected despite native Node default spec reporter; output read back without rerun',baselineReplayed=False,productionQualified=False))
 print('Existing baseline output verified: three missing refusals; no test replay.')
elif mode=='apply-and-qualify':
 assert json.loads((O/'baseline-public.json').read_bytes())['passed'] is True
 for row in mf['sourceOverlays']:assert pin((R/row['path']).read_bytes())==row['before']
 for row in mf['sourceOverlays']:(R/row['path']).write_bytes((P/row['sourcePath']).read_bytes())
 q=run([NODE,'--test','--test-reporter=tap','tools/bridge-source/finality.test.mjs'],'candidate-finality-file',R)
 assert q.returncode==0,q.stdout.decode()+q.stderr.decode()
 for row in mf['sourceOverlays']:assert pin((R/row['path']).read_bytes())==row['after']
 text=q.stdout.decode();counts={}
 for line in text.splitlines():
  for k in ('tests','suites','pass','fail','cancelled','skipped','todo'):
   if line.startswith('# '+k+' '):counts[k]=int(line.split()[-1])
 assert counts['fail']==0 and counts['skipped']==0 and counts['pass']==counts['tests']
 result=dict(format='nano-base-finality-response-snapshot-qualification-v1',passed=True,baselineNewRefusalsMissing=3,candidateNewRefusals=3,counts=counts,sourceFiles=mf['sourceOverlays'],nodeVersion=(O/'node-version-stdout.log').read_text().strip(),executionEnvironment='native macOS Node; existing C++ Docker dependencies image declares no Node package; no image rebuild or package download',rpcFixturesOnly=True,networkAccessUsed=False,cppCommands=0,dockerBuilds=0,eligibleForMint=False,cryptographicFinalityVerified=False,bridgeImplemented=False,runtimeDutiesResumed=False,productionQualified=False)
 save('result-public.json',result);print(json.dumps(result,sort_keys=True))
else:raise AssertionError('unknown mode')
