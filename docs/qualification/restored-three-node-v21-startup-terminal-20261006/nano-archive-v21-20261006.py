from pathlib import Path
import json,shutil,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v21-delivery-diagnostic-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v21-startup-terminal-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
t=read(src/'terminal-exit.json');assert t['originalSession']==5353 and t['exitCode']==1
r=read(src/'result.json');assert r['result']=='FAILED' and r['stopReceipts']==1 and 'VPS IPC readiness deadline' in r['failure']
state=read(src/'vps.log.state.json');assert not state['State']['Running'] and state['State']['ExitCode']==0 and not state['State']['OOMKilled']
audit=tmp/'nano-v21-post-stop-vps-audit-20261006';assert read(audit/'controller-exit.json')['exitCode']==0
receipt=read(src/'2-external-stop-receipt.json');cap=read(audit/'capture.json');assert cap['restart']==receipt['restart'];assert {x['index']:x['restart'] for x in cap['validatorKeys']}=={x['index']:x['restart'] for x in receipt['validatorKeys']}
mac=read(src/'unstarted-mac-immutability.json');assert all(d['hashesUnchangedFromPreparedCopy'] and not d['ownerStarted'] for d in mac.values())
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name=='vps.log.state.json':(out/p.name).write_text(json.dumps({k:state[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
(out/'post-stop-vps').mkdir()
for p in audit.iterdir():
 if p.is_file() and p.suffix in ['.json','.log','.sha256']:shutil.copy2(p,out/'post-stop-vps'/p.name)
for name in ['nano-restored-three-node-v21-controller-20261006.log','nano-launch-v21-delivery-diagnostic-20261006.py','nano-run-v21-vps-local-audit-20261006.py','nano-archive-v21-20261006.py']:shutil.copy2(tmp/name,out/name)
for name in ['nano-network-status-lock-probe-20261006','nano-network-status-lock-probe-repair-20261006']:
 source=tmp/name;dest=out/name;dest.mkdir()
 for f in source.iterdir():
  if f.is_file() and f.suffix in ['.json','.log','.cpp']:shutil.copy2(f,dest/f.name)
for name in ['nano-prepare-network-status-lock-probe-20261006.py','nano-prepare-network-status-lock-probe-repair-20261006.py']:shutil.copy2(tmp/name,out/name)
probe=tmp/'nano-network-status-lock-probe-repair-20261006'
for field in ['identity','sync']:assert read(probe/(field+'-baseline-exit.json'))['exitCode']==1 and '"completedWhileAdmissionHeld":false' in (probe/(field+'-baseline.log')).read_text()
(out/'README.md').write_text('''# Reprise v21 : disponibilité IPC du VPS non atteinte

Contrôleur original 5353, checkout 32e75e19, cœur d8d5b46c et relais diagnostic e7b41646. Sortie terminale 1. Le préflight confirme les programmes et les engagements post-v20. Le VPS démarre mais ses requêtes nano_capellaStatus produisent read-error aux étapes status puis verify ; la récupération termine not-ready après trois null-responses, networkProtocolActive=false. Le délai IPC fixe de 180 secondes expire. Propriétaires Mac non lancés ; le critère de trois nouvelles finalités communes en 900 secondes n’est pas exécuté.

Arrêt VPS normal, sortie 0 sans OOM. Un reçu indépendant est conservé. Audit read-only des 33 clés, engagements et journaux/témoins comparés au reçu ; historique canonique et ancêtre finalisé vérifiés. Nombre exact de fichiers immuables dans post-stop-vps/immutability.json. Aucune base ou clé privée transférée. Copies Mac non exécutées et identiques aux empreintes de préparation, 1573/1538 fichiers.

La phase de lecture HTTP/RPC est établie, pas la cause réseau précise : read-error ne distingue pas à lui seul délai, fermeture ou réinitialisation. La revue de source indique que nano_capellaStatus appelle network_identity et sync_admission_stats, deux lectures sous mutex du réacteur. Un nouveau test isolé contre le cœur inchangé d8d5b46c reproduit les deux lectures bloquées : sorties 1 attendues, publication et engagements inchangés après annulation, même statut après déverrouillage. L’échec initial d’inclusion du brouillon et sa réparation sont conservés. Ce défaut local est démontré ; son lien exact avec le WAN n’est pas encore établi, et le correctif reste à réaliser. Aucun changement de quorum, limites ou délais, aucune nouvelle image ou relance automatique.

L’échec v19, v20 et toutes les autres preuves/critères de production, bridge/lancement et revue indépendante restent conservés et ouverts. Les diagnostics qualifiés de livraison des votes ne sont pas exercés sur un réseau actif par cette campagne.
''')
note='\n\n2026-10-06 — v21 FAILED au démarrage : contrôleur 5353 / sortie 1, checkout 32e75e19, nano_capellaStatus read-error aux étapes status/verify, récupération not-ready puis délai IPC VPS 180 s expiré. Mac non lancés et inchangés (1573/1538 fichiers) ; critère de finalité non exécuté. Arrêt VPS normal, audit read-only des engagements/témoins/historique vérifié sur son hôte. Défaut local reproduit contre cœur inchangé : network_identity et sync_admission_stats bloquent sous admission retenue (deux sorties 1 attendues, annulation sans publication). Correctif et causalité WAN restent ouverts, aucune relance ni limite augmentée. [Preuves v21](qualification/restored-three-node-v21-startup-terminal-20261006/README.md). Toutes les autres exigences et revue indépendante restent ouvertes.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived v21 startup failure and independently checked stop audit')
