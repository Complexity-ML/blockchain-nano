from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-dependency-preparation-docker-20261006';native=tmp/'nano-dependency-preparation-native-20261006';out=repo/'docs/qualification/dependency-preparation-integrated-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
m=read(root/'source-manifest.json')
for n,h in m.items():assert sha(repo/n)==h,n
assert read(root/'native-terminal-exit.json')['exitCode']==0
for p in native.glob('*-exit.json'):
 if p.name=='original-terminal-exit.json':continue
 assert read(p)['exitCode']==(1 if p.name=='runtime-usage-exit.json' else 0),p
assert read(native/'old-wire-link-not-qualifying.json')['qualificationAllowed']==False
assert read(native/'wire-repair/wire-exit.json')['exitCode']==0
assert read(native/'wire-repair/provenance.json')['coreSHA256']=='4eb5122e27b9f4ffaa78c5bd63ddbc9e2e6f7f01dff9a45b1d88a69529915d99'
assert read(native/'source-immutability.json')['hashesUnchanged']
for arch in ['arm64','amd64']:
 assert read(root/(arch+'-terminal-exit.json'))['exitCode']==0
 assert (root/(arch+'-proof/sdk-controller-exit.txt')).read_text().strip()=='0'
 assert '100% tests passed, 0 tests failed out of 21' in (root/(arch+'-proof/sdk-ctest.log')).read_text()
 for name in ['regression-controller-exit.txt','test-wire-exit.txt']:assert (root/(arch+'-wire-proof')/name).read_text().strip()=='0'
 for phase in ['before','after']:
  s=(root/(arch+'-source-'+phase+'.log')).read_text();assert s.count(': OK')==len(m) and 'FAILED' not in s
 expected=(repo/f'docs/qualification/network-status-coherent-20261006/{arch}/runtime-programs.sha256').read_text().splitlines()[-1].split()[0]
 assert (root/(arch+'-wire-proof/test-programs.sha256')).read_text().splitlines()[-1].split()[0]==expected
out.mkdir(exist_ok=False)
def proofs(p,d):
 d.mkdir(parents=True,exist_ok=True)
 for f in p.rglob('*'):
  if f.is_file() and f.suffix in ['.log','.json','.txt','.sha256']:
   q=d/f.relative_to(p);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,q)
proofs(native,out/'native')
for arch in ['arm64','amd64']:
 proofs(root/(arch+'-proof'),out/(arch+'-sdk'));proofs(root/(arch+'-wire-proof'),out/(arch+'-wire'))
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sh','.sha256']:shutil.copy2(f,out/f.name)
for name in ['nano-prepare-dependency-preparation-20261006.py','nano-prepare-dependency-preparation-native-20261006.py','nano-qualify-dependency-preparation-native-20261006.py','nano-repair-dependency-preparation-native-wire-20261006.py','nano-launch-dependency-preparation-20261006.py','nano-collect-dependency-preparation-20261006.py','nano-archive-dependency-preparation-20261006.py']:shutil.copy2(tmp/name,out/name)
(out/'source').mkdir()
for name in ['CMakeLists.txt','capella_block_delivery.hpp','capella_head_catalog_relay.hpp','main.cpp','test_capella_dependency_preparation.cpp']:shutil.copy2(repo/'network'/name,out/'source'/name)
(out/'README.md').write_text('''# Préparation bornée des dépendances entre tours par pair

Le contre-exemple conservé avec v22 répète vingt parcours trop profonds sans envoyer aucun bloc. Le nouveau mécanisme garde une cible locale authentifiée et un curseur de préparation par session de pair. Au plafond de parcours, le prochain tour explore le parent authentifié avant de revenir vers la cible. Ce curseur n'est jamais un ACK : chaque payload continue d'exiger l'admission effective du récepteur, et le succès reste conditionné à l'ACK de la tête réellement observée.

Profondeur, octets, quantum d'admission, quorum, fencing et délai total inchangés. Deux chaînes de root par pair, pas de collection non bornée de cibles. Les deux chemins de remplacement de session réinitialisent la préparation ; les callbacks de génération périmée restent isolés. Un export local explicitement indisponible et une tête redevenue ancre libèrent la préparation périmée. Catalogue historique et alternance des catégories conservés.

Qualification ciblée native, Docker ARM64 et Docker AMD64 : quatorze suites affectées et sept nouvelles variantes (normal, duplication, indisponible, ancre élaguée indisponible, refus, ACK perdu, annulation/ACK tardif). Vingt-et-un tests CTest réussissent sur chaque Docker ; natif via commandes enregistrées équivalentes. Runtime nano-p2p compilé sur les trois architectures. Sources figées 349 fichiers vérifiées avant/après ; sources du cœur d285db64 inchangées et bibliothèques cœur vérifiées. Images/volumes réutilisés, sorties SDK distinctes conservant les anciens programmes.

La liaison native initiale du scénario wire utilisait encore l'objet de fixture parent ; elle est conservée mais exclue. Après la sortie de son contrôleur d'origine, le nouvel objet compilé est correctement relié au cœur actuel et le scénario est exécuté. Cette réparation ne recompile ni la fixture ni les suites SDK. Les Docker utilisent directement la nouvelle fixture. Le scénario wire couvre neuf blocs, dix-sept messages, reprises SIGKILL, annulations et rejeux dans son périmètre ; finalityScenario=false et networkActivated=false : aucune qualification de finalité WAN.

L'échec de finalité v22 reste conservé. Ce lot corrige un défaut local reproduit et raccorde le runtime ; son effet sur la restauration à trois nœuds doit encore être démontré par une nouvelle campagne avec journaux/engagements audités. Production complète, release sans instrumentation, autres scénarios consensus, bridge/fair launch et revue indépendante restent ouverts.
''')
note='\n\n2026-10-06 — Préparation bornée des dépendances par pair intégrée : cible/cursor locaux authentifiés, mêmes plafonds et ACK effectifs, remise à zéro aux deux chemins de changement de session. Natif/ARM64/AMD64 : quatorze suites affectées + sept variantes nouvelles, runtime nano-p2p compilé et scénario wire ciblé ; 21 tests Docker passent, sources figées vérifiées et cœur d285db64 réutilisé sans modification. Liaison native initiale erronée exclue, nouvel objet relié et scénario exécuté après terminal original. [Preuves ciblées](qualification/dependency-preparation-integrated-20261006/README.md). Effet sur finalité WAN v22 non qualifié ; autres critères et revue indépendante ouverts.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived scoped dependency preparation qualification')
