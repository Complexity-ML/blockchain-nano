from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v24-dependency-preparation-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v24-terminal-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
t=read(src/'terminal-exit.json');assert t['exitCode']==1 and t['originalSession']==4013
assert read(src/'result.json')['stopReceipts']==3 and read(src/'distinct-finality-progress.json')['distinctAdvances']==[]
assert read(src/'native-state.json')['exitCode']==0
for name in ['docker-mac.log.state.json','vps.log.state.json']:
 d=read(src/name)['State'];assert not d['Running'] and d['ExitCode']==0 and not d['OOMKilled']
na=tmp/'nano-v24-post-stop-native-replay-budget-20261006';va=tmp/'nano-v24-post-stop-vps-audit-20261006'
assert read(va/'controller-exit.json')['exitCode']==0
counts={};participation={}
for n,i in [('mac-native',0),('mac-docker',1),('vps-docker',2)]:
 capture=read(va/'capture.json' if i==2 else na/(n+'-capture.json'));receipt=read(src/f'{i}-external-stop-receipt.json')
 assert capture['restart']==receipt['restart'];actual={k['index']:k['restart'] for k in capture['validatorKeys']};expected={k['index']:k['restart'] for k in receipt['validatorKeys']};assert expected and all(actual.get(k)==v for k,v in expected.items())
 imm=read(va/'immutability.json' if i==2 else na/(n+'-immutability.json'));assert imm['hashesUnchanged'];counts[n]=imm['files'];participation[n]=capture['participationDiagnostics']
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name.endswith('.state.json'):
  d=read(p);(out/p.name).write_text(json.dumps({k:d[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
for folder,source in [('post-stop-native',na),('post-stop-vps',va)]:
 dest=out/folder;dest.mkdir()
 for p in source.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.log','.sha256']:continue
  if p.name.endswith('-config.json') or p.name.endswith('-request.json'):continue
  shutil.copy2(p,dest/p.name)
for name in ['nano-archive-v24-terminal-20261006.py','nano-v24-post-stop-native-replay-budget-20261006.py','nano-run-v24-vps-local-audit-20261006.py','nano-v24-vps-local-audit-20261006.py','nano-v24-vote-divergence-summary-20261006.json','nano-v24-live-head-regressions-20261006.json','nano-v24-live-resources-20261006.json','nano-v24-live-native-block-progress-20261006.json']:
 shutil.copy2(tmp/name,out/name)
(out/'audit-summary.json').write_text(json.dumps({'unchangedFileCounts':counts,'participationDiagnostics':participation},indent=2)+'\n')
text="""# Reprise v24 : finalité WAN non avancée

Contrôleur original 4013, checkout 49362f29, cœur d285db64 et relais 514ad2b8 (pilotage 6424fa7c). Sortie terminale 1 : zéro nouvelle finalité commune en 900 secondes, sur trois requises. Trois propriétaires actifs, puis trois arrêts normaux sans OOM. Les engagements et témoins anti-double-signature sont comparés aux reçus indépendants après arrêt ; toutes les bases restent inchangées pendant les audits. Les nombres exacts et participations figurent dans audit-summary.json.

Les lecteurs qualifiés inchangés rejouent les archives canoniques avec un budget forensic de 32768 blocs et 512 Mio. Ce budget hors ligne ne modifie aucune limite runtime. Configuration et requête VPS dérivées sur place ; seules les preuves publiques sont collectées, aucune base ni clé privée exportée.

La correction de préparation des dépendances possède ses qualifications ciblées sur trois architectures ; cette campagne ne qualifie pas son effet WAN. Les diagnostics natifs bornés montrent encore des délais de relais de blocs et des dépendances d'attestations non admises. Leur trace est plafonnée à 256 événements : ils ne représentent pas la totalité des tentatives. À 10:43:25.580Z, la tête du Docker Mac régresse du slot 18356 à 10760. Ce changement de branche doit être expliqué ; il ne prouve pas à lui seul une violation de finalité. Le checkpoint finalisé observé reste à l'époque 1284. Aucune cause globale n'est attribuée sans preuve.

Les mesures de ressources sont ponctuelles, sans qualification de pic ni de charge. Les octets d'E/S cumulés cgroup ne sont pas la taille de la base (environ 383 Mio au relevé VPS). Aucune image reconstruite ni campagne relancée. Finalité commune, autres exigences de production, bridge/lancement et revue indépendante restent ouverts.
"""
(out/'README.md').write_text(text)
note='\n\n2026-10-06 — v24 FAILED : contrôleur original 4013 / sortie 1, checkout 49362f29, cœur d285db64 et relais 514ad2b8. Zéro nouvelle finalité commune en 900 s ; trois propriétaires actifs et trois arrêts normaux. Audits read-only : engagements et témoins comparés aux reçus indépendants, hashes inchangés ; budget de rejeu forensic 32768 blocs / 512 Mio, limites runtime inchangées. Régression de tête Docker Mac 18356 → 10760 observée, checkpoint finalisé 1284 conservé ; cause globale et effet WAN du correctif non qualifiés. [Preuves v24](qualification/restored-three-node-v24-terminal-20261006/README.md). Toutes les exigences non démontrées et la revue indépendante demeurent ouvertes.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n')
print('v24 archived with independent audit commitments and unchanged data proofs')
