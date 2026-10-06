from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v25-timeout-preparation-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v25-terminal-20261006';na=tmp/'nano-v25-post-stop-native-replay-budget-20261006';va=tmp/'nano-v25-post-stop-vps-audit-20261006';ni=tmp/'nano-v25-inventory-diagnostic-native-20261006';vi=tmp/'nano-v25-inventory-diagnostic-vps-proof-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assert read(src/'terminal-exit.json')['originalSession']==29725 and read(src/'terminal-exit.json')['exitCode']==1;assert read(src/'result.json')['stopReceipts']==3;assert read(src/'distinct-finality-progress.json')['distinctAdvances']==[]
assert read(src/'native-state.json')['exitCode']==0
for f in ['docker-mac.log.state.json','vps.log.state.json']:
 s=read(src/f)['State'];assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
assert read(va/'controller-exit.json')['exitCode']==0 and read(vi/'controller-exit.json')['exitCode']==0
summary={}
for n,i in [('mac-native',0),('mac-docker',1),('vps-docker',2)]:
 capture=read(va/'capture.json' if i==2 else na/(n+'-capture.json'));receipt=read(src/f'{i}-external-stop-receipt.json');assert capture['restart']==receipt['restart'];actual={k['index']:k['restart'] for k in capture['validatorKeys']};expected={k['index']:k['restart'] for k in receipt['validatorKeys']};assert expected and all(actual.get(k)==v for k,v in expected.items())
 imm=read(va/'immutability.json' if i==2 else na/(n+'-immutability.json'));assert imm['hashesUnchanged'];inv=read(vi/'capture.json' if i==2 else ni/(n+'-capture.json'));assert inv['restart']==capture['restart'];invimm=read(vi/'immutability.json' if i==2 else ni/(n+'-immutability.json'));assert invimm['hashesUnchanged'] and invimm['sameFilesAsPriorFullAudit'];summary[n]={'fileCount':imm['files'],'participationDiagnostics':capture['participationDiagnostics'],'inventoryDiagnostics':inv}
assert read(ni/'original-terminal-exit.json')['originalSession']==90287 and read(ni/'original-terminal-exit.json')['exitCode']==1;assert read(ni/'link-exit.json')['exitCode']==1 and read(ni/'link-repair-exit.json')['exitCode']==0;assert read(ni/'provenance.json')['sourceUnchanged']
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name.endswith('.state.json'):
  d=read(p);(out/p.name).write_text(json.dumps({k:d[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
for folder,source in [('post-stop-native',na),('post-stop-vps',va),('inventory-native',ni),('inventory-vps',vi)]:
 dest=out/folder;dest.mkdir()
 for p in source.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.log','.sha256','.txt']:continue
  if p.name.endswith('-config.json') or p.name.endswith('-request.json'):continue
  shutil.copy2(p,dest/p.name)
for name in ['nano-archive-v25-terminal-20261006.py','nano-v25-post-stop-native-replay-budget-20261006.py','nano-run-v25-vps-local-audit-20261006.py','nano-v25-vps-local-audit-20261006.py','nano-v25-head-regressions-20261006.json','nano-v25-live-native-relay-summary-20261006.json','nano-v25-live-mac-docker-public-20261006.json','nano-v25-live-vps-public-20261006.json','nano-v25-live-vps-dispatch-20261006.json','nano-v25-live-vps-rpc-20261006.json','nano-v25-live-vps-rpc-methods-20261006.json','nano-v25-collect-vps-public-summary-20261006.py','nano-v25-collect-vps-dispatch-summary-20261006.py','nano-v25-collect-vps-rpc-summary-20261006.py','nano-v25-collect-vps-rpc-methods-20261006.py','nano-v25-inventory-diagnostic-native-20261006.py','nano-v25-inventory-diagnostic-native-link-repair-20261006.py','nano-v25-inventory-diagnostic-vps-controller-20261006.py']:
 shutil.copy2(tmp/name,out/name)
shutil.copytree(tmp/'nano-v25-inventory-diagnostic-vps-driver-20261006',out/'inventory-vps-drivers')
(out/'inventory-source').mkdir();shutil.copy2(tmp/'nano-v25-inventory-diagnostic-source-20261006/tests/capella/paused_capture.cpp',out/'inventory-source/paused_capture.cpp');shutil.copy2(tmp/'nano-v25-inventory-diagnostic-source-20261006/scope.json',out/'inventory-source/scope.json')
(out/'audit-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(out/'README.md').write_text('''# Reprise v25 : rattrapage partiel, finalité non avancée

Contrôleur original 29725, checkout e928b177, cœur d285db64 inchangé, relais ad536163. Sortie terminale 1, zéro nouvelle finalité commune en 900 secondes sur trois requises. Trois propriétaires démarrés, puis trois arrêts normaux sans OOM. Engagements et témoins anti-double-signature comparés aux reçus indépendants ; hashes de toutes les bases inchangés durant les audits. Compteurs exacts et participations dans audit-summary.json. Configurations/requêtes VPS dérivées sur le VPS ; seules des preuves publiques collectées.

Le Docker Mac quitte sa branche ancienne et rattrape des hauteurs récentes, puis régresse du slot 18793 à 10936 à 11:27:05.006Z. Le VPS rattrape lui aussi des hauteurs Mac. Des envois de blocs obtiennent des ACK effectifs dans les snapshots publics ; cela ne qualifie ni une tête commune ni la finalité. Le checkpoint observé reste 1284. Les captures finales donnent des participations timely-target récentes limitées au propriétaire natif sur sa branche, et au groupe VPS sur la branche ancienne du Docker Mac.

Diagnostics bornés publics : suspensions/reprises du transport, dispatch-unavailable, refus RPC capacity/queue-expired, et erreurs internes principalement sur CatalogAcquire. Le transfert supplémentaire filtre les champs publics sur le VPS avant SSH ; aucun log complet supplémentaire ni donnée de requête exporté. Les fenêtres 400/1200 lignes diffèrent : leurs compteurs ne sont pas les totaux de la campagne et ne doivent pas être additionnés. La limite de trace RPC est par connexion, pas globale. Les erreurs internes ne prouvent pas à elles seules une cause de finalité.

Diagnostic neuf, après audit complet, sur métadonnées authentifiées en lecture seule : graphe retenu natif 794, Docker Mac 758, VPS 627, pour 1024 autorisés. Catalogues individuels sous 65536 octets dans les trois captures ; le catalogue de blocs VPS utilise 20032 octets et celui d'attestations est vide. Ces résultats écartent un dépassement statique des quotas dans les états arrêtés, sans exclure une saturation transitoire ou des handles en cours pendant le runtime. Les limites runtime ne sont pas augmentées. Le budget hors ligne de 4 Mio du diagnostic n'est pas un changement de quota. Aucun rejeu canonique identique ajouté : les fichiers et engagements sont comparés au même audit complet déjà terminé.

Premier pilote du diagnostic natif : compilation réussie, liaison échouée faute de répertoire de travail pour les objets d'assemblage relatifs. Échec conservé ; aucune base ouverte par le pilote. Après terminal original, le même objet et les mêmes sources sont liés depuis le répertoire parent correct, puis les deux lectures sont vérifiées. AMD64 compile le même source avec le cœur actuel qualifié sur le VPS. Provenance et hashes conservés.

Les audits complets utilisent les lecteurs précédemment qualifiés, avec budget forensic 32768 blocs/512 Mio, limites de graphe/runtime inchangées. Pas d'image reconstruite, pas de relance de V25, pas de fonds ni de déploiement public. Prochaine investigation : contention RPC et transmission/retenue des votes récents, à reproduire avant modification. Toute cause globale, finalité WAN, autres exigences de production, bridge/fair launch et revue indépendante demeurent ouverts.
''')
note='\n\n2026-10-06 — V25 FAILED : original 29725 / sortie 1, checkout e928b177, zéro nouvelle finalité commune en 900 s. Rattrapage partiel de blocs avec ACK réels ; Docker Mac régresse 18793→10936. Trois arrêts normaux, engagements/témoins et hashes vérifiés. Diagnostic authentifié après arrêt : graphes 794/758/627 sur 1024, catalogues individuels sous 64 Kio ; aucune hausse de quota, pas de rejeu canonique identique. Échec initial de liaison du diagnostic conservé et réparé après terminal original avec le même objet. Traces bornées : capacity/queue-expired, suspensions/reprises et erreurs CatalogAcquire ; cause globale non prouvée. [Preuves V25](qualification/restored-three-node-v25-terminal-20261006/README.md). Investigation contention RPC/votes récents ; toutes exigences non démontrées et revue indépendante restent ouvertes.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('V25 archived: terminal failure, independent audits, quota diagnostic and original diagnostic link failure retained')
