from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v22-network-status-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v22-terminal-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
t=read(src/'terminal-exit.json');assert t['exitCode']==1 and t['originalSession']==91325
assert read(src/'result.json')['stopReceipts']==3 and read(src/'distinct-finality-progress.json')['distinctAdvances']==[]
assert read(src/'native-state.json')['exitCode']==0
for name in ['docker-mac.log.state.json','vps.log.state.json']:
 d=read(src/name)['State'];assert not d['Running'] and d['ExitCode']==0 and not d['OOMKilled']
na=tmp/'nano-v22-post-stop-native-replay-budget-20261006';va=tmp/'nano-v22-post-stop-vps-audit-20261006';old=tmp/'nano-v22-post-stop-native-diagnostic-20261006';draft=tmp/'nano-v22-depth-continuation-draft-20261006'
assert read(va/'controller-exit.json')['exitCode']==0
for n,i in [('mac-native',0),('mac-docker',1),('vps-docker',2)]:
 capture=read(va/'capture.json' if i==2 else na/(n+'-capture.json'));receipt=read(src/f'{i}-external-stop-receipt.json')
 assert capture['restart']==receipt['restart'];actual={k['index']:k['restart'] for k in capture['validatorKeys']};expected={k['index']:k['restart'] for k in receipt['validatorKeys']};assert expected and all(actual.get(k)==v for k,v in expected.items())
 assert read(va/'immutability.json' if i==2 else na/(n+'-immutability.json'))['hashesUnchanged']
assert read(draft/'native-proof/baseline-normal-exit.json')['exitCode']==1
proof=draft/'native-review-proof';assert read(proof/'source-immutability.json')['hashesUnchanged']
for mode in ['normal','duplicate','unavailable','unavailable-anchor','refused','lost','cancel']:assert read(proof/('draft-'+mode+'-exit.json'))['exitCode']==0
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name.endswith('.state.json'):
  d=read(p);(out/p.name).write_text(json.dumps({k:d[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
for folder,source in [('post-stop-native',na),('post-stop-vps',va),('original-native-audit-failure',old)]:
 dest=out/folder;dest.mkdir()
 for p in source.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.log','.sha256']:continue
  if p.name.endswith('-config.json') or p.name.endswith('-request.json'):continue
  shutil.copy2(p,dest/p.name)
for folder in ['native-proof','native-review-proof','review-source']:shutil.copytree(draft/folder,out/('isolated-draft-'+folder),ignore=shutil.ignore_patterns('*.o','test'))
for name in ['nano-archive-v22-terminal-20261006.py','nano-v22-post-stop-native-diagnostic-20261006.py','nano-v22-post-stop-native-replay-budget-20261006.py','nano-run-v22-vps-local-audit-20261006.py','nano-v22-vps-local-audit-20261006.py','nano-run-v22-depth-draft-native-20261006.py','nano-run-v22-depth-review-native-20261006.py','nano-v22-offline-audit-budget-20261006.json','nano-v22-bounded-diagnostic-summary-20261006.json','nano-v22-live-vps-public-diagnostics-20261006.jsonl']:
 shutil.copy2(tmp/name,out/name)
(out/'README.md').write_text('''# Reprise v22 : finalité non avancée, défaut de parcours reproduit

Contrôleur original 91325, checkout 80cd143e, cœur d285db64 et relais e7b41646. Sortie terminale 1 ; zéro nouvelle finalité commune en 900 secondes, sur trois requises. Les trois propriétaires ont démarré et repris leurs duties. Le VPS a rattrapé les hauteurs Mac mais la finalité est restée à l'époque 1284. Cela ne qualifie pas la finalité WAN ni n'explique tous les échecs historiques.

Trois arrêts normaux, sans OOM. Audits read-only terminés : engagements et témoins comparés aux reçus indépendants, historique canonique et ancêtre finalisé vérifiés. Fichiers inchangés : natif 1526, Docker Mac 1558, VPS 2360. Les participations timely-target récentes des captures Mac sont limitées au propriétaire local. Configuration et requête VPS dérivées sur place ; aucune base ni clé privée exportée.

Le premier audit natif échoue au plafond de 1024 blocs du rejeu canonique hors ligne ; cet échec et les hashes inchangés sont conservés. Le même lecteur qualifié réussit avec une requête bornée à 32768 blocs et 512 Mio. Seul le budget du lecteur forensic change : les limites de graphe/runtime/réseau demeurent inchangées. L'audit VPS original réussit sans cette réparation. Cette distinction entre graphe retenu et archive canonique doit rester explicite dans les procédures de restauration.

Les traces bornées identifient des attestations non envoyées faute de préparation de parents : dependency-depth-limit. Le catalogue fournit un rattrapage partiel ; ces traces n'établissent pas une cause unique de tout l'échec WAN. La trace d'attestation runtime est plafonnée à 256 événements, donc ses totaux ne sont pas ceux de la campagne entière. La collecte manuelle des logs VPS complets a été refusée par la revue automatique ; les diagnostics supplémentaires ont été filtrés en champs publics sur le VPS avant transfert.

Brouillon isolé, non intégré : cible/cursor de préparation par pair, remise à zéro aux changements de session, règles d'ACK et budgets conservés. Contre-exemple ancienne version : 20 tentatives, 60 exports, aucun envoi et aucun ACK de cible (sortie 1 attendue). Les sept scénarios du brouillon revu/figé passent en natif : normal, duplication, export indisponible, ancre élaguée indisponible, refus, ACK perdu et annulation/ACK tardif. Les sources figées et leurs hashes sont conservés. La première preuve à six cas précède deux ajustements de revue ; elle reste historique, la preuve native-review est celle du brouillon revu. Runtime, Docker ARM/AMD et vraie admission/transport WAN de ce correctif ne sont pas qualifiés.

Aucune image reconstruite, aucune relance de v22, fonds, vente ou déploiement public. Finalité commune, exigences de production non démontrées, bridge/lancement et revue indépendante restent ouverts.
''')
note='\n\n2026-10-06 — v22 FAILED : contrôleur original 91325 / sortie 1, checkout 80cd143e, zéro nouvelle finalité commune en 900 s malgré trois propriétaires actifs et rattrapage des hauteurs VPS. Trois arrêts normaux ; audits read-only et engagements/témoins vérifiés, fichiers inchangés (1526/1558/2360). Premier audit natif limité à 1024 blocs échoué et conservé ; requête hors ligne bornée à 32768 réussie, lecteur et limites runtime inchangés. Dépendances de votes bloquées par profondeur dans les traces ; cause WAN complète non prouvée. Brouillon non intégré : contre-exemple ancien code échoué, sept modes natifs revus passent ; runtime/Docker/WAN non qualifiés. [Preuves v22](qualification/restored-three-node-v22-terminal-20261006/README.md). Autres exigences et revue indépendante ouvertes.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n')
print('v22 terminal failure, independent audits, original forensic limit failure and isolated draft evidence archived')
