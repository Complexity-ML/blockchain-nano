from pathlib import Path
import hashlib,json,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-timeout-closed-guard-docker-20261006';native=tmp/'nano-timeout-closed-guard-native-20261006';prior=repo/'docs/qualification/timeout-preparation-integrated-20261006';out=repo/'docs/qualification/timeout-closed-guard-integrated-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
manifest=read(root/'source-manifest.json');old=read(prior/'source-manifest.json');assert [n for n in old if old[n]!=manifest[n]]==['network/capella_head_catalog_relay.hpp']
for n,h in manifest.items():assert sha(repo/n)==h,n
assert read(root/'native-terminal-exit.json')['exitCode']==0
for p in native.glob('*-exit.json'):assert read(p)['exitCode']==(1 if p.name=='runtime-usage-exit.json' else 0)
assert read(native/'source-immutability.json')['hashesUnchanged'];assert read(root/'wire-reuse-boundary.json')['changedHeaderNotInWireCompileDependencies']
for arch in ['arm64','amd64']:
 assert read(root/(arch+'-terminal-exit.json'))['exitCode']==0
 assert (root/(arch+'-proof/sdk-controller-exit.txt')).read_text().strip()=='0';assert '100% tests passed, 0 tests failed out of 12' in (root/(arch+'-proof/sdk-ctest.log')).read_text()
 for phase in ['before','after']:assert (root/(arch+'-source-'+phase+'.log')).read_text().count(': OK')==len(manifest)
out.mkdir(exist_ok=False)
def proofs(source,dest):
 dest.mkdir(parents=True)
 for p in source.rglob('*'):
  if p.is_file() and p.suffix in ['.log','.txt','.json','.sha256']:
   q=dest/p.relative_to(source);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
proofs(native,out/'native')
for arch in ['arm64','amd64']:proofs(root/(arch+'-proof'),out/(arch+'-sdk'))
for p in root.iterdir():
 if p.is_file() and p.suffix in ['.log','.json','.sha256','.sh','.d']:shutil.copy2(p,out/p.name)
for name in ['nano-integrate-timeout-closed-guard-20261006.py','nano-launch-timeout-closed-guard-20261006.py','nano-qualify-timeout-closed-guard-native-20261006.py','nano-collect-timeout-closed-guard-20261006.py','nano-archive-timeout-closed-guard-20261006.py']:shutil.copy2(tmp/name,out/name)
shutil.copy2(repo/'network/capella_head_catalog_relay.hpp',out/'capella_head_catalog_relay.hpp')
(out/'README.md').write_text('''# Garde après fermeture du relais

Complément de la qualification timeout-preparation-integrated-20261006 : un seul header change, capella_head_catalog_relay.hpp. La fonction fail retourne immédiatement si le relais est déjà fermé, avant toute modification du curseur. Un callback de délai déjà prêt ne peut donc réintroduire une préparation après completion. Aucun changement des limites, quorum, wire, cœur ou règles d'ACK.

Natif, Docker ARM64 et Docker AMD64 : douze cas affectés (neuf variantes de préparation, head catalog, termination, history fairness) et runtime nano-p2p compilés. Sources figées 349 fichiers et hashes avant/après vérifiés. Sorties distinctes, anciennes qualifications conservées. Le scénario wire qualifié dans le lot précédent est réutilisé : scan réel des dépendances du compilateur prouve que le header modifié n'est pas une entrée de cette fixture, et tous ses autres fichiers et le cœur restent identiques. Il n'est pas rejoué.

Ces preuves locales ne qualifient ni la finalité WAN ni la régression de branche V24. Les échecs et critères de production non démontrés, bridge/fair launch et revue indépendante restent ouverts. Images/volumes réutilisés, aucun fonds ni déploiement public.
''')
note='\n\n2026-10-06 — Complément de revue : garde après fermeture du relais avant toute mutation du curseur. Un seul header change ; 12 cas affectés et runtime passent en natif/ARM64/AMD64. Wire précédent réutilisé après scan réel de dépendances et hashes des autres entrées inchangés ; aucun rejeu identique. [Preuves](qualification/timeout-closed-guard-integrated-20261006/README.md). Finalité WAN V24, autres critères et revue indépendante toujours ouverts.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Guard qualification archived; prior wire scope verified for reuse')
