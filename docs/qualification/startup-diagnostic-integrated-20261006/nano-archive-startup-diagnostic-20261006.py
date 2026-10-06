from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-startup-diagnostic-docker-20261006';native=tmp/'nano-startup-diagnostic-native-20261006';out=repo/'docs/qualification/startup-diagnostic-integrated-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
m=read(root/'source-manifest.json')
for n,h in m.items():assert sha(repo/n)==h,n
assert read(root/'native-terminal-exit.json')['exitCode']==0
for p in native.glob('*-exit.json'):assert read(p)['exitCode']==(1 if p.name=='runtime-usage-exit.json' else 0),p
for arch in ['arm64','amd64']:
 assert read(root/(arch+'-terminal-exit.json'))['exitCode']==0
 assert (root/(arch+'-proof/sdk-controller-exit.txt')).read_text().strip()=='0'
 assert '100% tests passed, 0 tests failed out of 14' in (root/(arch+'-proof/sdk-ctest.log')).read_text()
 for name in ['regression-controller-exit.txt','test-wire-exit.txt']:assert (root/(arch+'-wire-proof')/name).read_text().strip()=='0'
 for phase in ['before','after']:
  s=(root/(arch+'-source-'+phase+'.log')).read_text();assert s.count(': OK')==len(m) and 'FAILED' not in s
out.mkdir(exist_ok=False)
def proofs(p,d):
 d.mkdir(parents=True,exist_ok=True)
 for f in p.rglob('*'):
  if f.is_file() and f.suffix in ['.log','.json','.txt','.sha256']:
   q=d/f.relative_to(p);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,q)
proofs(native,out/'native')
for arch in ['arm64','amd64']:
 proofs(root/(arch+'-proof'),out/(arch+'-sdk'));proofs(root/(arch+'-wire-proof'),out/(arch+'-wire'))
for name in ['nano-core-rpc-diagnostic-native-draft-proof-20261006','nano-startup-diagnostic-native-proof-20261006','nano-startup-diagnostic-observer-native-proof-20261006','nano-startup-diagnostic-observer-native-repair-proof-20261006']:
 proofs(tmp/name,out/name)
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sh','.sha256']:shutil.copy2(f,out/f.name)
for name in ['nano-prepare-startup-diagnostics-20261006.py','nano-qualify-startup-diagnostic-native-20261006.py','nano-launch-startup-diagnostic-20261006.py','nano-collect-startup-diagnostic-20261006.py','nano-archive-startup-diagnostic-20261006.py','nano-qualify-startup-diagnostic-observer-repair-20261006.py']:shutil.copy2(tmp/name,out/name)
(out/'source').mkdir()
for name in ['CMakeLists.txt','core.hpp','core_rpc_diagnostic.hpp','altair_core_recovery.hpp','main.cpp','test_altair_core_recovery.cpp','test_core_keepalive.cpp','test_core_rpc_diagnostic.cpp']:shutil.copy2(repo/'network'/name,out/'source'/name)
(out/'README.md').write_text('''# Diagnostics RPC et récupération IPC — qualification ciblée

Le relais conservait le même résultat nul pour les erreurs de transport, HTTP, RPC ou format. La récupération ne révélait pas l'étape de son échec. Un observateur optionnel expurgé conserve maintenant phase HTTP, méthode autorisée, code numérique et causes connues ; aucune donnée de requête, token, session, payload ou message brut. Les étapes status/rotate/inventory/cancel/forget/verify et leurs résultats sont identifiées. La trace de runtime est activée uniquement avec NANO_TEST_NETWORK_TRACE ; événements de requête répétitifs non imprimés, diagnostic RPC plafonné par observateur de connexion. Ce plafond ne constitue pas une qualification globale des ressources ou du réseau.

Aucune modification des limites, de la rotation/fencing, du retry, de l'admission ou des ACK. Une exception d'observateur est absorbée. Qualification native, Docker ARM64 et Docker AMD64 : 14 suites affectées, runtime P2P compilé et scénario réel --wire recompilé contre le cœur inchangé d8d5b46c. Récupération : 46 cas avec observateurs off/on/throws ; transport loopback : 18 scénarios, mêmes callbacks, ordre et connexions ; 6 cas de classification/expurgation. Le premier échec de compilation du brouillon (alias privé) et sa correction restent conservés.

Scénario réel --wire : neuf blocs, dix-sept messages, deux reprises SIGKILL, dix-huit annulations ; finalityScenario=false, networkActivated=false. Images/volumes réutilisés, aucune reconstruction d'image. 348 sources figées vérifiées avant/après les deux Docker ; zéro changement du cœur, fixture consommatrice recompilée.

Originaux : natif 43059, ARM64 8395, AMD64 34086, tous terminaux 0. L'échec réel v19 est conservé séparément : ce lot n'en prouve pas la cause. Prochaine exigence : reprise bornée après les audits des journaux/engagements pour obtenir l'étape et la cause réelle, puis démontrer la finalité commune. Toutes les autres exigences de production, EVM/forks, bridge/lancement et revue indépendante restent ouvertes.
''')
note='\n\n2026-10-06 — Diagnostic IPC/RPC intégré et qualifié dans son périmètre : 14 suites affectées et --wire natif/ARM64/AMD64, originaux 43059/8395/34086 sorties 0. Étapes de récupération et causes RPC expurgées ; mêmes règles de fencing/admission/retry. 46 scénarios de récupération, 18 transports loopback et 6 classifications ; cœur réutilisé, fixture recompilée, images inchangées. Cause réelle v19 et finalité commune non prouvées. [Preuves ciblées](qualification/startup-diagnostic-integrated-20261006/README.md). Échecs et critères de production/revue indépendante conservés.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived scoped startup diagnostic qualification')
