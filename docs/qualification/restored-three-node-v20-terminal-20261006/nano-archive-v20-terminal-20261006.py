from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v20-startup-diagnostic-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v20-terminal-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assert read(src/'terminal-exit.json')=={'exitCode':1,'observedTerminalAt':'2026-10-06T08:41:14.456109+00:00','originalSession':34949}
assert read(src/'result.json')['stopReceipts']==3
assert read(src/'distinct-finality-progress.json')['distinctAdvances']==[]
assert read(src/'native-state.json')['exitCode']==0
for name in ['docker-mac.log.state.json','vps.log.state.json']:
 d=read(src/name)['State'];assert not d['Running'] and d['ExitCode']==0 and not d['OOMKilled']
na=tmp/'nano-v20-post-stop-native-diagnostic-20261006';va=tmp/'nano-v20-post-stop-vps-audit-20261006';ga=tmp/'nano-v20-graph-pressure-native-proof-20261006';draft=tmp/'nano-delivery-cause-native-draft-proof-20261006'
assert read(va/'controller-exit.json')['exitCode']==0
for n,i in [('mac-native',0),('mac-docker',1),('vps-docker',2)]:
 capture=read((va/'capture.json') if i==2 else na/(n+'-capture.json'));receipt=read(src/f'{i}-external-stop-receipt.json')
 assert capture['restart']==receipt['restart'];actual={k['index']:k['restart'] for k in capture['validatorKeys']};expected={k['index']:k['restart'] for k in receipt['validatorKeys']};assert expected and all(actual.get(k)==v for k,v in expected.items())
assert read(ga/'scope.json')['dataHashesUnchanged']
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name.endswith('.state.json'):
  d=read(p);(out/p.name).write_text(json.dumps({k:d[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
for folder,source in [('post-stop-native',na),('post-stop-vps',va),('graph-pressure',ga),('delivery-cause-draft-proof',draft)]:
 dest=out/folder;dest.mkdir()
 for p in source.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.log','.sha256']:continue
  if p.name.endswith('-config.json') or p.name.endswith('-request.json'):continue
  shutil.copy2(p,dest/p.name)
for name in ['nano-archive-v20-terminal-20261006.py','nano-v20-post-stop-native-diagnostic-20261006.py','nano-run-v20-vps-local-audit-20261006.py','nano-v20-reuse-graph-pressure-reader-20261006.py','nano-qualify-delivery-cause-draft-20261006.py']:shutil.copy2(tmp/name,out/name)
public=out/'delivery-cause-draft-source';public.mkdir()
for p in (tmp/'nano-delivery-cause-draft-20261006').iterdir():
 if p.suffix in ['.hpp','.cpp']:shutil.copy2(p,public/p.name)
(out/'README.md').write_text('''# Reprise v20 : finalité commune non avancée

Contrôleur original 34949, checkout e8008698, cœur d8d5b46c et relais diagnostic a373efe7. Sortie terminale 1 à 08:41:14 UTC. Trois propriétaires restaurés et actifs ; disponibilité IPC réussie cette fois, sans explication rétroactive du démarrage v19. Zéro nouvelle finalité commune sur les trois demandées en 900 secondes. Epoch finalisée 1284. Des blocs et des têtes avancent mais cela ne qualifie pas la finalité.

Trois arrêts normaux, zéro OOM indiqué. Audits read-only : engagements de reprise et témoins de clés identiques aux reçus indépendants, historique canonique et ancêtre finalisé vérifiés. Les preuves d'immutabilité donnent le nombre exact de fichiers ; les configurations et requêtes VPS restent dérivées et utilisées sur le VPS. Aucune base ni clé privée transférée.

Les deux captures Mac montrent des participations timely-target limitées au groupe local. Le reader étendu réutilisé mesure la pression du graphe ; tous les autres champs correspondent exactement au reader qualifié et les bases restent inchangées. Ce diagnostic à l'arrêt ne démontre pas l'absence de saturation transitoire ni la cause WAN.

Les logs complets enregistrent des admissions de blocs, des envois non confirmés et des erreurs RPC. Un catalog close admission-rejected n'établit pas un refus de vote distant : le wrapper normalise également les dépendances de blocs non satisfaites. Les erreurs RPC génériques ne prouvent pas une cause unique. Aucun quota, délai ou fencing n'a été relâché.

Brouillon isolé, non intégré : 48 cas d'attestation (observateur absent, présent ou lançant une exception) et 10 cas de classification RPC/redaction réussissent. Il distingue les dépendances non satisfaites, les ACK de vote et les deux messages HTTP fixes de saturation/expiration. Qualification native de diagnostic uniquement : aucune preuve de finalité WAN ni de transport réel ajoutée par ces fixtures.

Aucune image reconstruite, aucun redémarrage de la campagne, fonds, vente ou déploiement public. Le critère de finalité et toutes les exigences de production non démontrées, bridge/lancement et revue indépendante restent ouverts.
''')
note='\n\n2026-10-06 — v20 FAILED : contrôleur original 34949 / sortie 1, checkout e8008698, zéro nouvelle finalité commune en 900 s (3 requises), malgré trois propriétaires restaurés actifs. Trois arrêts normaux ; audits read-only des engagements et témoins comparés aux reçus indépendants, bases inchangées. Le démarrage IPC réussit sans expliquer v19. Les participations Mac restent locales ; les logs ne prouvent pas encore une cause unique. Brouillon non intégré : 48 cas attestation + 10 cas diagnostic RPC passent, sans qualification WAN. [Preuves v20](qualification/restored-three-node-v20-terminal-20261006/README.md). Les autres critères et la revue indépendante restent ouverts.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n')
print('v20 terminal failure, independent audits and isolated draft evidence archived')
