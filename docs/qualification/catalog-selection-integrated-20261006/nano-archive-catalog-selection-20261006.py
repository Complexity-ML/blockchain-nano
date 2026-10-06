from pathlib import Path
import json,hashlib,shutil,subprocess
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-catalog-selection-docker-20261006');dest=repo/'docs/qualification/catalog-selection-integrated-20261006'
m=json.loads((root/'source-manifest.json').read_text());assert all(hashlib.sha256((repo/p).read_bytes()).hexdigest()==h for p,h in m.items())
for arch in ['arm64','amd64']:
 assert json.loads((root/(arch+'-terminal-exit.json')).read_text())['exitCode']==0
 p=root/(arch+'-proof')
 for f in ['controller-exit.txt','runtime-controller-exit.txt','regression-controller-exit.txt','sdk-controller-exit.txt']:assert (p/f).read_text().strip()=='0'
 assert '100% tests passed, 0 tests failed out of 5' in (p/'sdk-ctest.log').read_text()
summary=json.loads((root/'native-proof/summary.json').read_text());assert summary['nativeCoreExit']==0 and summary['nativeNetworkExit']==0
assert not dest.exists();dest.mkdir(parents=True)
for name in ['native-proof','arm64-proof','amd64-proof','source']:shutil.copytree(root/name,dest/('sources' if name=='source' else name.removesuffix('-proof')))
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sha256','.sh']:shutil.copy2(f,dest/f.name)
for name in ['nano-launch-catalog-selection-20261006.py','nano-prepare-catalog-selection-docker-20261006.py','nano-collect-catalog-selection-docker-20261006.py','nano-archive-catalog-selection-20261006.py']:shutil.copy2(Path('/private/tmp')/name,dest/name)
local=dest/'vps-local-preparation';local.mkdir()
for name in ['nano-vps-local-launch-prepare.py','nano-test-vps-local-derive-20261006.py','nano-test-vps-local-derive-corrected-20261006.py','nano-check-vps-local-derive-live-20261006.py','nano-test-vps-local-derive-final-20261006.py','nano-check-vps-local-derive-final-live-20261006.py']:shutil.copy2(Path('/private/tmp')/name,local/name)
proof=Path('/private/tmp/nano-v17-local-prepare-diagnostic-final-20261006')
for name in ['summary.json','vps-readonly-derive.log','vps-readonly-derive-exit.json']:shutil.copy2(proof/name,local/name)
assert json.loads((proof/'vps-readonly-derive-exit.json').read_text())['exitCode']==0
shutil.copy2(repo/'tests/capella/restored-three-node-run.mjs',local/'restored-three-node-run.mjs')
shutil.copy2(repo/'tests/capella/vps-local-launch-prepare.py',local/'vps-local-launch-prepare.py')
(local/'scope.json').write_text(json.dumps({'derivationCases':6,'actualVpsReadonlyDerivationMatches':True,'initialFixtureFailure':'old plan had pre-runtime startupCatchup; corrected source is actual stopped v16 configuration','controllerSyntaxChecked':True,'controllerNewBranchEndToEndQualified':False,'signersStarted':False},indent=2)+'\n')
summary.update({'dockerPending':False,'arm64OriginalSession':73015,'amd64OriginalSession':20532,'arm64Exit':0,'amd64Exit':0,'dockerSdkSuites':5,'imagesReused':True,'databasesOrKeysTransferred':False})
(dest/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(dest/'README.md').write_text("""# Sélection du catalogue entre tentatives — qualification ciblée

Base `19b722bd`, 345 sources figées dans `sources/`. Le contre-exemple du préfixe répété demeure dans `../catalog-prefix-starvation-baseline-20261006/` ; la campagne v16 reste un échec de finalité WAN.

La sélection par pair et catégorie reprend après le dernier élément tenté. Elle ne constitue ni admission ni ACK ; refus et réponses incertaines sont réessayés après bouclage. Une nouvelle session ou génération remet la sélection à zéro. Le seek recherche dans le snapshot stable sans assouplir la validation d'appartenance du curseur de page ; authentification privée, propriété, budgets et annulation restent contrôlés. Fin d'un suffixe ne signifie pas parcours complet.

Qualification native et Docker ARM64/AMD64 : 136 unités de cœur rescannées, 18 dépendantes recompilées depuis l'archive précédente ; scénarios runtime configuré, reprise automatique, HTTP, wire. Huit vérifications de bornes seek et trois propriété/limites, neuf engagements indépendants de reprise, deux réouvertures SIGKILL dans wire. SDK : sélection 9 cas nouveaux, catalogue 46, tête 19, diagnostic 7, historique 10. Tests SDK modélisés ; ils ne prouvent pas la finalité WAN. Binaire nano-p2p compilé sur les trois environnements.

Échecs des lanceurs natifs conservés : mauvais nom de fichier, définition CMake NANO_HEAD_HISTORY_FIRST omise, répertoire de lien absent. La définition CMake rétablie fait passer les dix cas historiques ; aucune source corrigée pour cacher ces échecs. Les images et volumes sont réutilisés, sans build d'image ni transfert de base/configuration/clés entre hôtes.

Pas de qualification release sans instrumentation, de finalité commune à trois nœuds, de charge générale, ni de production par ce lot. La reprise v17 devra conserver les engagements post-v16 et le critère original de trois finalités nouvelles en 900 secondes. Revue indépendante et tous autres critères du goal restent ouverts.
""")
manifest={str(p.relative_to(dest)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dest.rglob('*')) if p.is_file()};(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');assert all(hashlib.sha256((dest/p).read_bytes()).hexdigest()==h for p,h in manifest.items())
paragraph='\n\n2026-10-06 — Sélection du catalogue entre tentatives : qualification native, Docker ARM64 et AMD64, sources figées et sorties terminales vérifiées. Seek borné/authentifié dans snapshot ; positions de sélection distinctes des ACK, bouclage/retry et reset session/génération. Neuf nouveaux cas SDK ; suites catalogue/tête/diagnostic/historique et quatre scénarios cœur passent. Le contre-exemple initial et les échecs de lanceurs sont conservés. Aucune image reconstruite, base/configuration/clé transférée. Preuves catalog-selection-integrated-20261006/. Finalité commune post-restauration et causalité WAN restent ouvertes ; release sans instrumentation et production non qualifiées.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+paragraph)
print(json.dumps({'proofFiles':len(manifest),'hashesVerified':True,'networkQualified':False}))
