from pathlib import Path
import json,shutil,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v19-signed-anchor-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v19-startup-terminal-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
t=read(src/'terminal-exit.json');assert t['originalSession']==81355 and t['exitCode']==1
r=read(src/'result.json');assert r['result']=='FAILED' and r['stopReceipts']==1 and 'VPS IPC readiness deadline' in r['failure']
state=read(src/'vps.log.state.json');assert not state['State']['Running'] and state['State']['ExitCode']==0 and not state['State']['OOMKilled']
audit=tmp/'nano-v19-post-stop-vps-audit-20261006';assert read(audit/'controller-exit.json')['exitCode']==0
receipt=read(src/'2-external-stop-receipt.json');cap=read(audit/'capture.json');assert cap['restart']==receipt['restart'];assert {x['index']:x['restart'] for x in cap['validatorKeys']}=={x['index']:x['restart'] for x in receipt['validatorKeys']}
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name=='vps.log.state.json':(out/p.name).write_text(json.dumps({k:state[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
(out/'post-stop-vps').mkdir()
for p in audit.iterdir():
 if p.is_file() and p.suffix in ['.json','.log','.sha256']:shutil.copy2(p,out/'post-stop-vps'/p.name)
for name in ['nano-restored-three-node-v19-controller-20261006.log','nano-v19-startup-vps-observation-20261006.log','nano-launch-v19-signed-anchor-20261006.py','nano-run-v19-vps-local-audit-20261006.py','nano-archive-v19-20261006.py']:shutil.copy2(tmp/name,out/name)
(out/'README.md').write_text('''# Reprise v19 : échec de disponibilité IPC du VPS

Contrôleur original 81355, checkout fc704b9f ; cœur d8d5b46c et relais f11250ae. Sortie terminale 1. Préflight vérifié, configuration et environnement dérivés sur le VPS sans export depuis le Mac. Le VPS restaure son état mais signale altair-ipc-recovery-failed, puis le délai de disponibilité IPC de 180 secondes expire. Les deux propriétaires Mac ne sont pas lancés. Le critère de trois finalités communes en 900 secondes n'est donc pas exécuté et reste ouvert.

Arrêt VPS normal, sortie 0 sans OOM. Un reçu indépendant est conservé. Audit read-only : 33 clés, engagements et journaux/témoins comparés au reçu ; historique canonique et ancêtre finalisé vérifiés par le reader qualifié, 2170 fichiers inchangés pendant l'audit. Aucun transfert de base ou de clés entre hôtes. Les copies Mac post-v18 ne sont pas exécutées par cette campagne.

La revue de source montre que les erreurs transport/RPC deviennent nullptr et que l'événement IPC confond fin de récupération nulle et exception de configuration. L'étape/cause précise n'est pas établie. Le parcours d'ancre signée intervient après cette récupération : son correctif ciblé ne qualifie pas ce démarrage. Un brouillon de diagnostic redigé et testé séparément n'est pas encore intégré.

Aucun rebuild d'image, compilation concurrente, flood, fonds ou déploiement public. Tous les échecs antérieurs, critères de production, bridge/lancement et revue indépendante restent ouverts.
''')
note='\n\n2026-10-06 — Reprise v19 FAILED au démarrage : contrôleur 81355 / sortie 1, checkout fc704b9f, délai IPC VPS de 180 s expiré ; propriétaires Mac non lancés et critère de finalité non exécuté. Arrêt VPS normal et audit read-only des engagements/journaux/historique, 2170 fichiers inchangés ; base et clés non exportées. Cause IPC précise encore ouverte, diagnostic isolé en préparation sans changement des politiques. [Preuves v19](qualification/restored-three-node-v19-startup-terminal-20261006/README.md). Toutes les autres exigences et la revue indépendante restent ouvertes.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived v19 startup failure with verified stopped VPS audit')
