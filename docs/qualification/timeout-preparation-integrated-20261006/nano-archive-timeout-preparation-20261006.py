from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-timeout-preparation-docker-20261006';native=tmp/'nano-timeout-preparation-native-20261006';out=repo/'docs/qualification/timeout-preparation-integrated-20261006';draft=tmp/'nano-v24-timeout-continuation-draft-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
m=read(root/'source-manifest.json')
for n,h in m.items():assert sha(repo/n)==h,n
assert read(root/'native-terminal-exit.json')['exitCode']==0
for p in native.glob('*-exit.json'):assert read(p)['exitCode']==(1 if p.name=='runtime-usage-exit.json' else 0),p
assert read(native/'source-immutability.json')['hashesUnchanged']
link=read(native/'wire-link-command.json');assert str(native/'candidate.o') in link and '/private/tmp/nano-network-status-coherent-native-20261006/test.o' not in link
assert sha(Path('/private/tmp/nano-network-status-coherent-native-20261006/libnano_core.a'))=='4eb5122e27b9f4ffaa78c5bd63ddbc9e2e6f7f01dff9a45b1d88a69529915d99'
for arch in ['arm64','amd64']:
 assert read(root/(arch+'-terminal-exit.json'))['exitCode']==0
 assert (root/(arch+'-proof/sdk-controller-exit.txt')).read_text().strip()=='0'
 assert '100% tests passed, 0 tests failed out of 23' in (root/(arch+'-proof/sdk-ctest.log')).read_text()
 for name in ['regression-controller-exit.txt','test-wire-exit.txt']:assert (root/(arch+'-wire-proof')/name).read_text().strip()=='0'
 for phase in ['before','after']:
  s=(root/(arch+'-source-'+phase+'.log')).read_text();assert s.count(': OK')==len(m) and 'FAILED' not in s
 expected=(repo/f'docs/qualification/network-status-coherent-20261006/{arch}/runtime-programs.sha256').read_text().splitlines()[-1].split()[0];assert (root/(arch+'-wire-proof/test-programs.sha256')).read_text().splitlines()[-1].split()[0]==expected
for scenario in ['slow','slow-duplicate']:assert read(draft/('native-review-proof/baseline-'+scenario+'-exit.json'))['exitCode']==1
assert read(draft/'native-review-proof/source-immutability.json')['hashesUnchanged']
assert read(draft/'initial-pilot-not-qualifying.json')['exitCode']==1
out.mkdir(exist_ok=False)
def proofs(p,d):
 d.mkdir(parents=True,exist_ok=True)
 for f in p.rglob('*'):
  if f.is_file() and f.suffix in ['.log','.json','.txt','.sha256','.py','.sh']:
   q=d/f.relative_to(p);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,q)
proofs(native,out/'native')
for arch in ['arm64','amd64']:
 proofs(root/(arch+'-proof'),out/(arch+'-sdk'));proofs(root/(arch+'-wire-proof'),out/(arch+'-wire'))
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sh','.sha256']:shutil.copy2(f,out/f.name)
for name in ['nano-integrate-timeout-preparation-20261006.py','nano-qualify-timeout-preparation-native-20261006.py','nano-launch-timeout-preparation-20261006.py','nano-collect-timeout-preparation-20261006.py','nano-archive-timeout-preparation-20261006.py','nano-run-v24-timeout-draft-20261006.py','nano-run-v24-timeout-review-20261006.py']:shutil.copy2(tmp/name,out/name)
for name in ['native-proof','native-review-proof']:proofs(draft/name,out/('isolated-'+name))
shutil.copy2(draft/'initial-pilot-not-qualifying.json',out/'initial-pilot-not-qualifying.json')
for name in ['baseline-review-source','draft-review-source']:shutil.copytree(draft/name,out/name,ignore=shutil.ignore_patterns('*.o','*.pyc','__pycache__'))
(out/'source').mkdir()
for name in ['CMakeLists.txt','capella_block_delivery.hpp','capella_head_catalog_relay.hpp','test_capella_dependency_preparation.cpp']:shutil.copy2(repo/'network'/name,out/'source'/name)
(out/'README.md').write_text('''# Reprise du parcours de dépendances après délai borné

Parent d80e15a8, cœur d285db64 inchangé. Le relais conservait son curseur seulement au plafond de profondeur : si les exports authentifiés prenaient plus de temps que le délai total, il repartait de la même cible au tour suivant. Le contre-exemple isolé avec exports retardés échoue avec le code parent : 20 tours, 60 exports, zéro envoi et zéro ACK de cible. Le correctif atteint les neuf ACK en 15 tours et 39 exports avec les mêmes limites. Deux modes nouveaux : exports lents, exports lents dupliqués ; callbacks d'export reçus après completion ignorés.

Le dernier parent extrait d'un export local authentifié devient un curseur de préparation au délai total. Il ne vaut jamais ACK distant. Cette frontière est effacée avant toute émission de payload afin de ne pas transformer une transmission ambiguë en reprise implicite. Profondeur, octets, quantum, deadline, quorum et fencing inchangés ; succès conditionné aux ACK effectivement obtenus. La correction ne prétend pas expliquer entièrement l'échec WAN V24 ni la régression de branche observée.

Qualification ciblée : natif, Docker ARM64, Docker AMD64 ; quatorze suites affectées et neuf variantes de préparation, soit 23 tests CTest sur chaque Docker. Runtime nano-p2p compilé sur les trois architectures. Fixture wire compilée et réellement liée au cœur actuel inchangé ; commandes vérifiées pour l'objet natif. Wire couvre neuf blocs, dix-sept messages, deux SIGKILL, annulations, votes et opérations signées dans son périmètre. finalityScenario=false et networkActivated=false : finalité WAN non qualifiée.

Sources figées (349 fichiers), hashes avant/après et bibliothèques cœur vérifiés. Images/volumes existants réutilisés ; sorties distinctes gardant les anciens programmes. Le premier pilote isolé n'injectait pas le retard à cause d'un remplacement de texte sans correspondance : il est conservé et exclu. Seule la preuve native-review contient le retard effectivement injecté et les deux contre-exemples attendus.

Toutes les preuves historiques, les exigences de production non démontrées, la revue indépendante et les critères bridge/fair launch demeurent ouverts. Aucun lancement public, fonds ou vente. Effet sur la reprise réelle à trois nœuds à démontrer séparément.
''')
note='\n\n2026-10-06 — Reprise du parcours de dépendances après délai intégrée : parent local authentifié conservé comme curseur, jamais comme ACK ; effacement avant émission, mêmes limites et fencing. Contre-exemples exports lents/dupliqués : parent 20 tours sans envoi, correctif 15 tours/9 ACK. Natif/ARM64/AMD64 : 23 cas ciblés, runtime et wire liés au cœur d285db64 inchangé, hashes vérifiés. Premier pilote sans retard exclu et conservé. [Preuves](qualification/timeout-preparation-integrated-20261006/README.md). Effet WAN V24 et régression de branche non qualifiés ; autres critères et revue indépendante ouverts.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived scoped timeout preparation qualification')
