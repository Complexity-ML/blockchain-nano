from pathlib import Path
import json,shutil,hashlib
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');out=repo/'docs/qualification/v35-post-stop-readonly-20261007';out.mkdir(exist_ok=False)
h=json.loads((r/'original-handles.json').read_text());h['localAudit'].update(status='TERMINAL',exitCode=0,terminalChunk='511769');h['vpsAudit'].update(status='TERMINAL',exitCode=0,terminalChunk='0a65fa');(r/'original-handles.json').write_text(json.dumps(h,indent=2)+'\n')
s={'coreRevision':'c499649857b88a39df3d7badb97156daf96eb2c6','nodes':{},'networkQualified':False,'productionQualified':False}
for name in ['native','mac-docker','vps']:
 p=r/name;q=out/name;q.mkdir();assert json.loads((p/'exit.json').read_text())['exitCode']==0
 for f in ['capture.json','immutability.json','exit.json','error.log']:shutil.copy2(p/f,q/f)
 c=json.loads((p/'capture.json').read_text());i=json.loads((p/'immutability.json').read_text());assert i['hashesUnchanged'] and i['exactStopCommitmentsVerified'] and i['primaryAndIndependentWitnessVerified']
 s['nodes'][name]={'immutability':i,'captureSHA256':hashlib.sha256((p/'capture.json').read_bytes()).hexdigest(),'finalized':c['canonicalReplay']['finalized']}
for f in ['original-exit.json','original-handles.json','local-audit.py','vps-audit.py','local-audit-original.log']:shutil.copy2(r/f,out/f)
shutil.copy2(r/'vps/vps-audit-original.log',out/'vps-audit-original.log');shutil.copy2(__file__,out/'archive.py')
(out/'summary.json').write_text(json.dumps(s,indent=2)+'\n')
(out/'README.md').write_text('''# V35 — audits après arrêt

Les originaux66051 et52488 sont terminés à0. Les captures en lecture seule vérifient les engagements exacts des reçus d’arrêt, principal/témoin indépendant et rejeu canonique. Fichiers inchangés :1915 natif,1946 Docker Mac,3113 Docker VPS. Pins actifs16/16/33 et historiques16/16/0 vérifiés séparément. Cœur de capture c4996498, programmes existants réutilisés sans relink.

Les configurations, demandes, bases et dictionnaires de hashes restent privés sur leur hôte. Les capsules publiées reprennent le format public déjà utilisé pour V34. Aucun writer relancé et aucune image construite par ces audits.

V35 reste FAILED : finalité commune1284 inchangée. Ces audits prouvent l’intégrité après arrêt propre, pas une nouvelle finalité, une reprise de duties ou la production. Base–Nano/Doppler et revue indépendante restent ouverts.
''')
status='Audits V35 terminés à0 sur les trois environnements :1915/1946/3113 fichiers inchangés, pins actifs16/16/33 et historiques16/16/0 conservés séparément ; engagements arrêt, principal/témoin et rejeu canonique vérifiés. [Preuves](qualification/v35-post-stop-readonly-20261007/README.md). V35 FAILED conservée, aucune nouvelle finalité. Correctif inventaires signés privé en qualification : contre-exemple parent reproduit et premiers gates ARM64 passent ; parcours ARM64 encore actif62036, AMD64 lancé64089 après audit, garde inchangée. Pas encore intégré ni qualifié sur réseau/natif/release propre. Tous critères production, Base–Nano/Doppler et revue indépendante restent ouverts.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;t=p.read_text();old=t.split('\n\n',2)[2].split('\n\n',1)[0];assert old.startswith('V35 terminée FAILED');p.write_text(t.replace(old,status+'\n\nÉtat précédent conservé : '+old,1))
