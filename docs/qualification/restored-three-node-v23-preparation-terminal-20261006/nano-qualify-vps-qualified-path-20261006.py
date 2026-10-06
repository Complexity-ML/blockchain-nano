from pathlib import Path
import importlib.util,json,copy
repo=Path('/Users/boris/Dev/blockchain-nano');p=repo/'tests/capella/vps-local-launch-prepare.py';spec=importlib.util.spec_from_file_location('vps_prepare',p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
valid=['/build/catalog-selection-amd64-20261006/network/nano-p2p','/build/dependency-preparation-sdk-amd64-20261006/network/nano-p2p','/build/future-qualified-lot-amd64-20270101/network/nano-p2p']
invalid=['/build/catalog-selection-arm64-20261006/network/nano-p2p','/build/x-amd64-20261006/../network/nano-p2p','/build/../x-amd64-20261006/network/nano-p2p','/build/x-amd64-20261006/network/nano-p2p/extra','/tmp/x-amd64-20261006/network/nano-p2p','/build/x-amd64-2026100x/network/nano-p2p','/build/x-amd64-20261006/network/nano-p2p;bad',None]
for x in valid:assert m.qualified_p2p_binary(x)
for x in invalid:assert not m.qualified_p2p_binary(x)
x=json.loads((repo/'docs/qualification/restored-three-node-v23-dependency-preparation-prepare-20261006/input.json').read_text());configuration=x['vpsConfig'];capsule=x['vpsCapsule'];before=m.canonical_hash(configuration)
results=[m.derive(configuration,capsule,12345,12346,binary) for binary in valid]
assert m.canonical_hash(configuration)==before
for result,binary in zip(results,valid):
 assert result['p2p_binary']==binary and result['capella']['recoveryCommitment']==capsule['restart']
 assert {k['index']:k['restart'] for k in result['capella']['validator']['keys']}=={k['index']:k['restart'] for k in capsule['validatorKeys']}
assert len(set(m.canonical_hash(x) for x in results))==3
bad=copy.deepcopy(capsule);bad['primaryWitnessAndSignersVerified']=False
try:m.derive(configuration,bad,12345,12346,valid[1]);raise RuntimeError('unaudited capsule accepted')
except AssertionError:pass
print(json.dumps({'pathCases':len(valid)+len(invalid),'auditedDerivations':len(results),'unauditedCapsuleRejected':True,'inputUnchanged':True,'pathBindsCanonicalHash':True,'controllerDigestVerificationUnchanged':True,'blockchainRuntimeChanged':False,'networkActivated':False}))
