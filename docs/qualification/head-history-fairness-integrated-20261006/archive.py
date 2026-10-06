from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-history-first-20261006');out=repo/'docs/qualification/head-history-fairness-integrated-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def metadata(source,target):
 target.mkdir(exist_ok=False)
 for f in source.iterdir():
  if f.is_file() and f.suffix in ['.txt','.log','.json','.sha256','.cpp','.sh','.py']:shutil.copy2(f,target/f.name)
for n,h in read(root/'source-manifest.json').items():assert sha(root/'network'/n)==h and sha(repo/'network'/n)==h,n
previous=repo/'docs/qualification/head-catalog-fallback-integrated-20261006/network'
for f in (root/'baseline-network').rglob('*'):
 if f.is_file():assert f.read_bytes()==(previous/f.relative_to(root/'baseline-network')).read_bytes()
assert sha(root/'fairness-v1-control-source.cpp')==read(root/'source-v1-manifest.json')['test_capella_head_history_fairness.cpp']
receipts=read(root/'terminal-exits.json');assert all(v['exitCode']==0 for v in receipts.values())
assert (root/'baseline-exit.txt').read_text().strip()=='1' and 'HISTORY STARVATION: unconfirmed head consumed both historical opportunities' in (root/'baseline-run.log').read_text()
for case in ['capella_head_catalog_relay','capella_head_history_fairness','capella_relay_schedule']:
 prefix='arm-final-' if case=='capella_head_history_fairness' else 'arm-'
 assert (root/(prefix+case+'-exit.txt')).read_text().strip()=='0'
 assert (root/'amd-test-proof'/('amd-'+case+'-exit.txt')).read_text().strip()=='0'
 assert read(root/'native'/(case+'-run-exit.json'))['exitCode']==0
 logs=[(root/(prefix+case+'-run.log')).read_text().strip(),(root/'amd-test-proof'/('amd-'+case+'-run.log')).read_text().strip(),(root/'native'/(case+'-run.log')).read_text().strip()];assert len(set(logs))==1,case
for arch in ['arm64','amd64']:
 p=root/(arch+'-runtime-proof');assert (p/'controller-exit.txt').read_text().strip()=='0';assert (p/'usage-exit.txt').read_text().strip()=='1';assert 'Usage: nano-p2p' in (p/'usage.log').read_text()
assert read(root/'native/runtime-usage-exit.json')['exitCode']==1
assert (root/'cmake-targets-controller-exit.txt').read_text().strip()=='0';assert '100% tests passed, 0 tests failed out of 3' in (root/'cmake-targets-ctest.log').read_text()
out.mkdir(exist_ok=False);shutil.copytree(root/'network',out/'network');shutil.copytree(root/'baseline-network',out/'baseline-network')
metadata(root/'native',out/'native');metadata(root/'arm64-runtime-proof',out/'arm64-runtime');metadata(root/'amd64-runtime-proof',out/'amd64-runtime');metadata(root/'amd-test-proof',out/'amd64-tests');metadata(root,out/'control-and-commands')
for name in ['nano-edit-history-first-20261006.py','nano-build-history-first-native-20261006.py','nano-launch-history-first-amd-20261006.py','nano-collect-history-first-20261006.py','nano-history-first-arm-controller-20261006.log','nano-history-first-arm-final-controller-20261006.log','nano-history-first-native-controller-20261006.log','nano-history-first-arm-runtime-controller-20261006.log','nano-history-first-amd-controller-20261006.log','nano-history-first-collection-controller-20261006.log','nano-history-first-cmake-controller-20261006.log']:shutil.copy2(Path('/private/tmp')/name,out/name)
(out/'README.md').write_text('''# Alternance tête/historique — qualification ciblée

Base d97f8cb2, cœur 5787915e inchangé. Le réseau alterne, par pair/session, un tour de blocs priorisant la tête et un tour servant directement le catalogue historique sous le même délai, quota de dépendances, limite d’entrées et budget réseau. Les opportunités des votes et autres catégories restent intercalées. Réinitialisation de l’ordre avec une nouvelle session distante. Aucun changement des signatures, quorum, EVM, stockage ou témoins.

Le tour historique doit ensuite lire le statut authentifié du cœur et vérifier l’ACK réel du pair pour la tête sélectionnée. Une livraison historique, un indice de statut ou une réponse tardive ne devient jamais un ACK de tête. En cas de statut absent/tardif, les admissions et le résultat de fermeture du catalogue restent visibles ; le tour échoue. Les quotas et annulations s’appliquent au vrai chemin CapellaBlockDelivery.

Contre-exemple conservé : avec l’ancien relais 7248698d et le même premier scénario, deux têtes non confirmées consomment deux délais sans jamais servir la branche indépendante. Compilation réussie, sortie 1 attendue avec le marqueur HISTORY STARVATION. Source contrôle v1 figée conservée ; deux contrôles additionnels de quota/annulation ont ensuite été ajoutés à la source finale, sans modifier le premier scénario.

Mac natif, Docker ARM64 et Docker AMD64 : sorties identiques des trois unités pertinentes (19 priorité de tête, 10 alternance/ACK/délais/annulation/quota, 5 ordonnancement). Les autres consommateurs non modifiés ne sont pas rejoués. Configuration CMake, compilation réelle des trois cibles et CTest ARM : 3/3, sortie 0. Les trois exécutables nano-p2p sont liés et identifiés ; usage sans arguments : sortie 1 attendue. Binaires parents Docker inchangés. Images, volumes et dépendances existants réutilisés, sans rebuild d’image ni transfert de base/clé inter-hôtes.

Ces contrôles utilisent un transport modélisé. Ils prouvent le contre-exemple et les invariants ciblés, pas la résolution complète de WAN A, la reprise/finalité des trois propriétaires ou la production. Le test v14 FAILED et les graphes encore divergents restent conservés. Prochaine étape : restauration sur engagements/journaux post-v14 exacts avec les nouveaux binaires, critère original de trois nouvelles finalités communes en 900 s. Pression/coût/pruning, partitions, rotations/cycle validateurs, EVM/profil actif, bridge/lancement, production et revue indépendante restent ouverts.
''')
note='\n\n2026-10-06 — Alternance tête/historique qualifiée dans son périmètre : contre-exemple ancien relais conservé (tête non confirmée bloque deux tours historiques), correction et mêmes 19/10/5 scénarios sur Mac natif et Docker ARM/AMD. ACK de tête réel obligatoire, statut absent/tardif et erreurs restent des échecs, quota/annulation partagés ; catégories intercalées et état par session. Trois cibles CMake compilées et CTest ARM 3/3. Nouveaux binaires P2P identifiés, cœur 5787915e inchangé, images/volumes réutilisés. Aucun résultat WAN/finalité nouvelle revendiqué : prochaine reprise post-v14 selon le critère original. Échecs historiques, autres écarts et revue indépendante restent ouverts. [Preuves](qualification/head-history-fairness-integrated-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
shutil.copy2('/private/tmp/nano-archive-history-first-20261006.py',out/'archive.py');m={str(f.relative_to(out)):sha(f) for f in sorted(out.rglob('*')) if f.is_file() and f.name!='manifest.json'};(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');print('Archived',len(m),'verified proof files')
