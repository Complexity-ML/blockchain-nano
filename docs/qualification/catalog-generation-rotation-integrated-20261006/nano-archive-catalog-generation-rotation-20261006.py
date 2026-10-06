from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-catalog-generation-rotation-docker-20261006'
for arch in ['arm64','amd64']:
 assert json.loads((root/(arch+'-terminal-exit.json')).read_text())['exitCode']==0
 assert (root/(arch+'-proof/controller-exit.txt')).read_text().strip()=='0'
 assert '100% tests passed, 0 tests failed out of 5' in (root/(arch+'-proof/sdk-ctest.log')).read_text()
for p in (tmp/'nano-catalog-generation-rotation-native-sdk-20261006').glob('*-exit.json'):
 if p.name=='capella_head_history_fairness-run-exit.json':assert json.loads(p.read_text())['exitCode']==1
 else:assert json.loads(p.read_text())['exitCode']==0
for p in (tmp/'nano-catalog-generation-rotation-native-sdk-20261006/fairness-cmake-definitions').glob('*-exit.json'):assert json.loads(p.read_text())['exitCode']==0
out=repo/'docs/qualification/catalog-generation-rotation-integrated-20261006';out.mkdir(exist_ok=False)
for name,target in [('nano-catalog-generation-rotation-counterexample-proof-20261006','counterexample'),('nano-catalog-generation-rotation-full-draft-20261006','isolated-selection'),('nano-catalog-generation-rotation-native-sdk-20261006','native-sdk'),('nano-catalog-generation-rotation-native-network-20261006','native-network')]:
 dest=out/target;dest.mkdir()
 for f in (tmp/name).rglob('*'):
  if f.is_file() and f.suffix in ['.json','.log']:
   q=dest/f.relative_to(tmp/name);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,q)
for arch in ['arm64','amd64']:
 shutil.copytree(root/(arch+'-proof'),out/arch)
 for name in ['command.json','controller.log','terminal-exit.json']:shutil.copy2(root/(arch+'-'+name),out/(arch+'-'+name))
for name in ['source-manifest.json','source.sha256','core-reuse-audit.json','sdk.sh']:shutil.copy2(root/name,out/name)
for name in ['nano-catalog-generation-rotation-counterexample-20261006.cpp','nano-launch-catalog-generation-rotation-20261006.py','nano-qualify-catalog-generation-rotation-draft-20261006.py','nano-qualify-catalog-generation-rotation-native-network-20261006.py','nano-archive-catalog-generation-rotation-20261006.py']:shutil.copy2(tmp/name,out/name)
text="""# Catalog selection across retirement namespace rotation

Baseline adapter counterexample expected exit 1, isolated correction exit 0. Inventory and peer remain unchanged while bounded request retirement rotates its namespace; clearing the selection on that nonce rotation can repeatedly revisit refused roots and starve a later valid root. Preserve the selection boundary for the peer/session lifetime, while acquiring fresh owned snapshots with the current namespace. Explicit session reset still clears peer selections. Authentication, acquire generation fencing, budgets, seek validation, cancellation and ACK criteria remain intact.

Integrated native, Docker ARM64 and Docker AMD64: five affected suites (11 selection, 46 catalog, 19 head, 7 termination, 10 history fairness scenarios) and P2P compilation. Native helper initially omitted NANO_HEAD_HISTORY_FIRST and failed; preserve that failure and the corrected run matching CMake. Docker builds use the existing images and build volumes, without image rebuild. 343 prior frozen sources unchanged; only two network files changed, allowing scoped reuse of core/capture source qualification.

Mocked adapter results establish this defect and its correction, not the complete WAN failure cause or network convergence. v17 retry remains FAILED, zero new common finality, and all historical failures are retained. A new three-owner finality campaign with audited post-stop commitments remains required. No production readiness claim.
"""
(out/'README.md').write_text(text)
note='\n\n2026-10-06 — Catalogue : contre-exemple de starvation par rotation du namespace de requête reproduit sur le relais antérieur (sortie attendue 1), correctif intégré qualifié en natif et Docker ARM64/AMD64 : 5 suites affectées, 11/46/19/7/10 scénarios et compilation P2P. Conservation du curseur de sélection pendant la session ; nouveaux snapshots acquis avec le namespace courant et réinitialisation explicite au changement de session. Échec du lanceur natif sans la définition CMake conservé, relance corrigée réussie. Images/volumes réutilisés, cœur et capture inchangés. Cause WAN complète et progression de finalité toujours non qualifiées ; v17 reste FAILED. [Preuves](qualification/catalog-generation-rotation-integrated-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
manifest={str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print('Archived',len(manifest),'proof files',out)
