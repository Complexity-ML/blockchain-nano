from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-delivery-diagnostic-docker-20261006';native=tmp/'nano-delivery-diagnostic-native-20261006';out=repo/'docs/qualification/delivery-diagnostic-integrated-20261006'
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
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sh','.sha256']:shutil.copy2(f,out/f.name)
for name in ['nano-integrate-delivery-diagnostic-20261006.py','nano-qualify-delivery-diagnostic-native-20261006.py','nano-launch-delivery-diagnostic-20261006.py','nano-collect-delivery-diagnostic-20261006.py','nano-archive-delivery-diagnostic-20261006.py']:shutil.copy2(tmp/name,out/name)
(out/'source').mkdir()
for name in ['core.hpp','core_rpc_diagnostic.hpp','capella_attestation_delivery.hpp','main.cpp','test_capella_attestation_delivery.cpp','test_core_rpc_diagnostic.cpp']:shutil.copy2(repo/'network'/name,out/'source'/name)
(out/'README.md').write_text('''# Diagnostic des dépendances d’attestation et refus HTTP

Les journaux v20 comptabilisent zéro payload de vote admis alors que des blocs sont admis. Le wrapper transforme tout échec de préparation du bloc en accepted=false : cela ne démontre pas un refus du vote par le pair. L’observateur optionnel identifie désormais frame, block-dependency, vote-ack et cancel avec résultats bornés. Une réponse accepted=false du vote est distinguée d’un ACK perdu/malformé et de la dépendance non satisfaite. Seules deux raisons fixes de dépendance peuvent être imprimées ; aucun root, token, SSZ ni texte arbitraire. Branchement runtime sous NANO_TEST_NETWORK_TRACE, plafond global de 256 événements d’attestation par instance Network. Le plafond ne modifie pas le protocole et n’est pas une qualification de ressources.

Le diagnostic RPC classe exclusivement deux corps HTTP fixes, à statut 503 : capacité simulation dépassée ou attente de file expirée. Tout autre corps reste non imprimé et inconnu. Les exceptions d’observateur sont absorbées. Aucun changement de résultat, callback, cache ACK, délai, limite, admission, quorum, fencing ou retry.

Qualification native, Docker ARM64 et Docker AMD64 : 14 suites consommatrices touchées, runtime P2P compilé et vraie fixture --wire recompilée contre cœur d8d5b46c inchangé. 48 cas d’attestation avec observateur absent/présent/throws ; 10 cas classification/redaction HTTP/RPC ; 18 transports loopback de non-régression. Sources figées 348 unités vérifiées avant/après dans Docker. Images/volumes et bibliothèques cœur réutilisés, aucune image reconstruite.

La fixture --wire ne constitue pas une finalité WAN : finalityScenario=false et networkActivated=false. L’échec réel v20 reste conservé. Ce lot prépare la preuve de cause réelle ; il ne corrige pas encore le blocage et ne clôture aucune exigence de production globale. Critères EVM/forks activés, bridge/lancement et revue indépendante demeurent ouverts.
''')
note='\n\n2026-10-06 — Diagnostic dépendances/ACK des attestations et refus HTTP intégré et qualifié dans son périmètre : natif/ARM64/AMD64, 14 suites affectées et vraie fixture --wire, sources figées vérifiées. 48 cas attestation, 10 classifications/redactions et 18 transports loopback ; aucune modification des politiques ou du quorum. Trace runtime optionnelle bornée à 256 événements d’attestation par nœud. Ne corrige ni ne qualifie la finalité WAN v20 ; cause réelle encore à établir. [Preuves ciblées](qualification/delivery-diagnostic-integrated-20261006/README.md). Les autres critères et la revue indépendante restent ouverts.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived scoped delivery diagnostic qualification')
