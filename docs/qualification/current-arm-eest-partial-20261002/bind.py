import gzip,hashlib,json,subprocess
from pathlib import Path
source=Path('/tmp/nano-snapshot-account-view-candidate-20261002');reports=Path(__file__).resolve().parent;full=Path('/tmp/nano-full-coherent-0f53b42d-arm64-reports-20261002')
reference=json.loads(gzip.decompress(Path('/Users/boris/Dev/blockchain-nano/docs/qualification/snapshot-account-view-20261002/source-manifest.json.gz').read_bytes()))
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()=='0f53b42d3e78da85aa9a2c2f2993472747aabe8a'
assert not subprocess.check_output(['git','status','--porcelain'],cwd=source)
actual={p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in subprocess.check_output(['git','ls-files','-z'],cwd=source).decode().split('\0') if p}
assert actual==reference==json.loads((full/'source-before.json').read_text())
runner='/build/full-coherent-0f53b42d-arm64-20261002/nano-ethereum-state-runner'
sha=subprocess.check_output(['docker','exec','nano-full-coherent-0f53b42d-arm64','sha256sum',runner],text=True).split()
assert sha[1]==runner and sha[0]==json.loads((full/'programs-before.json').read_text())[runner]
fixture=json.loads((reports/'fixture-result.json').read_text());counts=json.loads((reports/'counts-result.json').read_text())
assert fixture['exactFixtureVariants']==counts['totalMatched']==5387 and counts['executionMatched']==4882 and counts['validationMatched']==505
result={'runtimeRevision':'0f53b42d3e78da85aa9a2c2f2993472747aabe8a','sourceFilesUnchangedAtPartialObservation':len(actual),'runner':runner,'runnerSha256MatchesCampaignBefore':sha[0],'partialLogSha256':hashlib.sha256((reports/'LastTest.partial.log').read_bytes()).hexdigest(),'eestFixtureAndCountAuditsPassed':True,'testSeconds':3.45,'originalTimeout':30,'fullCampaignTerminal':False,'postFullInputsVerified':False,'amdEestQualified':False,'completeForkConformance':False}
(reports/'binding-result.json').write_text(json.dumps(result,indent=2)+'\n')
