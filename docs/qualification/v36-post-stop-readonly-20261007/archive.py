from pathlib import Path
import json,shutil,hashlib
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');o=repo/'docs/qualification/v36-post-stop-readonly-20261007'
handles=json.loads((r/'original-handles.json').read_text());assert handles['localAudit']['session']==5836 and handles['vpsAudit']['session']==89720
assert all(x['exitCode']==0 and x['terminalChunk'] for x in handles.values());assert not o.exists();o.mkdir()
s={'coreRevision':'4ce3e4e2eaf5ea87372b9ca8eff399d23fdf9510','nodes':{},'networkQualified':False,'productionQualified':False}
for name in ['native','mac-docker','vps']:
 p=r/name;q=o/name;q.mkdir();assert json.loads((p/'exit.json').read_text())['exitCode']==0
 for f in ['capture.json','immutability.json','exit.json','error.log']:shutil.copy2(p/f,q/f)
 c=json.loads((p/'capture.json').read_text());i=json.loads((p/'immutability.json').read_text());assert i['hashesUnchanged'] and i['exactStopCommitmentsVerified'] and i['primaryAndIndependentWitnessVerified']
 s['nodes'][name]={'immutability':i,'captureSHA256':hashlib.sha256((p/'capture.json').read_bytes()).hexdigest(),'finalized':c['canonicalReplay']['finalized']}
for f in ['original-handles.json','local-audit.py','vps-audit.py','original-local.log','original-vps.log','archive.py']:shutil.copy2(r/f,o/f)
(o/'summary.json').write_text(json.dumps(s,indent=2)+'\n')
counts='/'.join(str(s['nodes'][n]['immutability']['files']) for n in ['native','mac-docker','vps'])
(o/'README.md').write_text('# V36 — audits après arrêt propre\n\nOriginaux5836 et89720 terminés0. Captures en lecture seule : engagements exacts des trois reçus d’arrêt, principal/témoin indépendant et rejeu canonique vérifiés. '+counts+' fichiers inchangés ; pins actifs16/16/33 et historiques16/16/0 vérifiés séparément. Cœur de capture4ce3e4e2, programmes déjà qualifiés réutilisés.\n\nConfigurations, demandes, bases et dictionnaires de hashes restent privés sur leur hôte. Capsules publiques au format déjà utilisé. Aucun writer relancé, aucune image construite. V36 FAILED conservée : aucune nouvelle finalité commune. Intégrité après arrêt propre ne prouve pas reprise effective de finalité, production ou lancement Base–Nano/Doppler. Revue indépendante ouverte.\n')
(o/'manifest.json').write_text(json.dumps({'files':{str(p.relative_to(o)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(o.rglob('*')) if p.is_file()}},indent=2)+'\n')
print(json.dumps({'files':counts,'pins':'16/16/33 active and16/16/0 historical','readOnly':True}))
