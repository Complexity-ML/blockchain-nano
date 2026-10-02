"""Control tests only: no SSH, Docker or WAN processes allowed."""
import importlib.util,tempfile,sys,json
from pathlib import Path
p=Path(__file__).with_name('launch.py')
spec=importlib.util.spec_from_file_location('wan_launch',p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as root:
 m.C=Path(root);m.OUTPUT=Path(root)/'never-created'
 def forbidden(*args,**kwargs):raise AssertionError('external action forbidden')
 m.call=forbidden
 sys.argv=['launch.py']
 try:m.main()
 except RuntimeError as e:assert 'explicit qualified execution flag' in str(e)
 else:raise AssertionError('missing execution flag accepted')
 assert not list(m.C.iterdir())
 sys.argv=['launch.py','--execute-qualified-wan']
 m.local_ready=lambda *args:True;m.remote_ready=lambda *args:False
 assert m.main()==2 and not m.OUTPUT.exists()
 attempts=list(m.C.glob('attempt-*'));assert len(attempts)==1
 assert json.loads((attempts[0]/'result.json').read_text())['newWanNodesStarted'] is False
 m.remote_ready=lambda *args:True
 calls=[]
 def first_review(args,**kwargs):
  calls.append(args)
  raise RuntimeError('terminal review refusal control')
 m.call=first_review
 try:m.main()
 except RuntimeError as e:assert str(e)=='terminal review refusal control'
 else:raise AssertionError('review refusal ignored')
 assert len(calls)==1 and calls[0][0]=='bash' and calls[0][1].endswith('rpc-terminal-review-control-20261002/run-review.sh')
 assert not m.OUTPUT.exists()
source=p.read_text()
assert 'nano-linux-sync-build' not in source and '9e9ebed3' not in source and '016ab243' not in source
assert all(key in source for key in ['NANO_TEST_BUILDER_IMAGE_ARM64','NANO_TEST_BUILDER_IMAGE_AMD64','NANO_TEST_BUILD_VOLUME_ARM64','NANO_TEST_BUILD_VOLUME_AMD64'])
print(json.dumps({'controlsPassed':4,'missingFlagRefused':True,'pendingGatesRefusedBeforeExternalAction':True,'reviewRefusalStopsBeforeWan':True,'currentArtifactsExplicit':True,'wanStarted':False},indent=2))
