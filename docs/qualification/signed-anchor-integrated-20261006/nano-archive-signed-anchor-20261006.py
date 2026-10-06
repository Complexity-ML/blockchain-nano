from pathlib import Path
import hashlib,json,shutil
repo=Path('/Users/boris/Dev/blockchain-nano'); tmp=Path('/private/tmp'); root=tmp/'nano-signed-anchor-docker-20261006'; out=repo/'docs/qualification/signed-anchor-integrated-20261006'
def read(p): return json.loads(p.read_text())
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
manifest=read(root/'source-manifest.json')
for name,h in manifest.items(): assert digest(repo/name)==h,name
native=tmp/'nano-signed-anchor-native-20261006'
for p in native.glob('*-exit.json'):
 expected=1 if p.name=='runtime-usage-exit.json' else 0
 assert read(p)['exitCode']==expected,(p,read(p))
for arch in ['arm64','amd64']:
 assert read(root/(arch+'-terminal-exit.json'))['exitCode']==0
 assert (root/(arch+'-proof/sdk-controller-exit.txt')).read_text().strip()=='0'
 for name in ['regression-controller-exit.txt','test-wire-exit.txt']: assert (root/(arch+'-wire-proof')/name).read_text().strip()=='0'
assert read(tmp/'nano-signed-anchor-counterexample-proof-20261006/baseline-run-exit.json')['exitCode']==1
assert read(tmp/'nano-signed-anchor-counterexample-proof-20261006/draft-run-exit.json')['exitCode']==0
out.mkdir(exist_ok=False)
def copyproof(source,dest):
 dest.mkdir(parents=True,exist_ok=True)
 for p in source.rglob('*'):
  if p.is_file() and p.suffix in ['.json','.log','.txt','.sha256']:
   q=dest/p.relative_to(source);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
copyproof(native,out/'native')
for arch in ['arm64','amd64']:
 copyproof(root/(arch+'-proof'),out/(arch+'-sdk'))
 copyproof(root/(arch+'-wire-proof'),out/(arch+'-wire'))
for stem in ['counterexample-proof','regression-draft-proof','security-draft-proof']:
 copyproof(tmp/('nano-signed-anchor-'+stem+'-20261006'),out/stem)
for p in root.iterdir():
 if p.is_file() and p.suffix in ['.json','.log','.sha256','.sh']: shutil.copy2(p,out/p.name)
for name in ['nano-signed-anchor-counterexample-20261006.cpp','nano-qualify-signed-anchor-counterexample-20261006.py','nano-qualify-signed-anchor-native-20261006.py','nano-launch-signed-anchor-20261006.py','nano-collect-signed-anchor-20261006.py','nano-archive-signed-anchor-20261006.py']:
 shutil.copy2(tmp/name,out/name)
(out/'source').mkdir()
for name in ['network/CMakeLists.txt','network/capella_block_delivery.hpp','network/capella_export_cache.hpp','network/test_capella_block_delivery.cpp','network/test_capella_signed_anchor.cpp']:
 shutil.copy2(repo/name,out/'source'/Path(name).name)
(out/'README.md').write_text('''# Ancre signée : correctif intégré et qualification ciblée

Le parcours des ancêtres pouvait dépasser sa limite avant d'envoyer une ancre signée déjà détenue par le destinataire. Le contre-exemple synthétique échoue sur la version précédente (sortie 1) et réussit avec le correctif (sortie 0). Il ne constitue pas une preuve de la cause WAN complète.

Le relais présente maintenant l'ancre locale signée au destinataire par son admission normale. Seul un ACK exact accepté établit la connaissance distante. Un refus explicite autorise la préparation bornée des parents ; un ACK perdu, tardif ou mal formé n'est jamais assimilé à une admission. L'ancre peut être présentée de nouveau après les ACK des parents, mais sans retry d'un résultat ambigu. Le cache conserve une métadonnée locale immuable, sans attester l'état du pair. Profondeur, mémoire, quantum, annulation et délai restent bornés.

Neuf suites SDK ciblées passent en natif, Docker ARM64 et Docker AMD64 : livraison des blocs (28 cas), ancre signée (11 nouveaux cas), attestations, rattrapage borné, reprise/cache, catalogue, sélection, terminaison et équité historique. Le runtime P2P compile sur les trois architectures. Le scénario --wire du cœur réel passe aussi sur les trois : neuf blocs, dix-sept messages, deux reprises SIGKILL et dix-huit annulations. Il indique explicitement finalityScenario=false et networkActivated=false : aucune qualification WAN ou finalité commune n'en découle.

Contrôleurs originaux : natif 94615, ARM64 86191, AMD64 33176, tous terminaux avec sortie 0. Collecte AMD64 51448 sortie 0. Les images et volumes existants ont été réutilisés sans rebuild d'image. L'audit de dépendances permet de réutiliser le cœur d8d5b46c : zéro consommateur du cœur parmi 136 unités ; la fixture réelle consommatrice du header a été recompilée. 346 sources figées sont vérifiées contre le dépôt. L'échec initial de comparaison du baseline réseau et sa correction sont conservés.

Les échecs de finalité v18 et antérieurs restent ouverts et conservés. Prochaine preuve requise : reprise entière et trois nouvelles finalités communes sur les trois propriétaires privés, avec leurs engagements et témoins indépendants. Production, release sans instrumentation de faute, portabilité physique, bridge, lancement et revue indépendante ne sont pas clos.
''')
note='\n\n2026-10-06 — Ancre locale signée : contre-exemple de famine reproduit puis correctif intégré. Admission distante obligatoire ; refus explicite permet le parcours borné des parents, résultat ambigu sans retry automatique. Neuf suites SDK et scénario réel --wire qualifiés en natif/ARM64/AMD64, contrôleurs 94615/86191/33176 sortis 0. Images/volumes réutilisés ; cœur inchangé, fixture consommatrice recompilée. Cause WAN globale et finalité commune non prouvées ; échecs historiques et toutes les exigences restantes conservés. [Qualification ciblée](qualification/signed-anchor-integrated-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):digest(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n')
print('Archived',out)
