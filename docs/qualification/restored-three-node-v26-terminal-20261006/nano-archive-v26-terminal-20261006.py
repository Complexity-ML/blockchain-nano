from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');src=tmp/'nano-restored-three-node-v26-catalog-reservation-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v26-terminal-20261006';na=tmp/'nano-v26-post-stop-native-replay-budget-20261006';va=tmp/'nano-v26-post-stop-vps-audit-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
t=read(src/'terminal-exit.json');result=read(src/'result.json');assert t['originalSession']==95043 and t['exitCode'] in [0,1];assert result['stopReceipts']==3
assert read(src/'native-state.json')['exitCode']==0 and read(src/'native-state.json')['signalCode'] is None
for f in ['docker-mac.log.state.json','vps.log.state.json']:
 s=read(src/f)['State'];assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
assert read(va/'controller-exit.json')['exitCode']==0
summary={}
for n,i in [('mac-native',0),('mac-docker',1),('vps-docker',2)]:
 c=read(va/'capture.json' if i==2 else na/(n+'-capture.json'));receipt=read(src/f'{i}-external-stop-receipt.json');assert c['restart']==receipt['restart'];actual={k['index']:k['restart'] for k in c['validatorKeys']};owned={k['index']:k['restart'] for k in receipt['validatorKeys']};assert owned and all(actual.get(k)==v for k,v in owned.items())
 if i<2:
  prior=read(repo/'docs/qualification/restored-three-node-v26-catalog-reservation-prepare-20261006/input.json')['macNativeCapsule' if i==0 else 'macCapsule'];expected={k['index']:k['restart'] for k in prior['validatorKeys']};expected.update(owned);assert actual==expected
 else:assert actual==owned
 imm=read(va/'immutability.json' if i==2 else na/(n+'-immutability.json'));assert imm['hashesUnchanged'];assert read(va/'exit.json' if i==2 else na/(n+'-exit.json'))['exitCode']==0
 summary[n]={'fileCount':imm['files'],'participationDiagnostics':c['participationDiagnostics'],'restart':c['restart'],'signerCommitmentsMatchIndependentReceipt':True,'ownedSignerCount':len(owned),'capturedSignerCount':len(actual),'historicalSignerPinsRetainedAndVerified':True}
rows=[json.loads(l) for l in (src/'events.jsonl').read_text().splitlines()];last={};reg=[]
for row in rows:
 if row['event']!='observation':continue
 for x in row['rows']:
  n=x['name'];slot=int(x['slot'])
  if n in last and slot<last[n]['slot']:reg.append({'node':n,'time':row['time'],'before':last[n],'after':x})
  last[n]={'slot':slot,'head':x['head'],'finalized':x['checkpoint']}
out.mkdir(exist_ok=False)
for p in src.iterdir():
 if not p.is_file():continue
 if p.name.endswith('.state.json'):
  d=read(p);(out/p.name).write_text(json.dumps({k:d[k] for k in ['State','Name','Image']},indent=2)+'\n')
 else:shutil.copy2(p,out/p.name)
for folder,source in [('post-stop-native',na),('post-stop-vps',va)]:
 dest=out/folder;dest.mkdir()
 for p in source.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.log','.sha256','.txt']:continue
  if p.name.endswith('-config.json') or p.name.endswith('-request.json'):continue
  shutil.copy2(p,dest/p.name)
shutil.copytree(tmp/'nano-v26-live-analysis-20261006',out/'live-analysis')
for name in ['nano-archive-v26-terminal-20261006.py','nano-v26-post-stop-native-replay-budget-20261006.py','nano-run-v26-vps-local-audit-20261006.py','nano-v26-vps-local-audit-20261006.py','nano-v26-live-vps-rpc-methods-20261006.json','nano-v26-live-mac-docker-rpc-methods-20261006.json','nano-v26-collect-vps-rpc-methods-20261006.py','nano-v26-collect-mac-docker-rpc-methods-20261006.py']:
 shutil.copy2(tmp/name,out/name)
(out/'audit-summary.json').write_text(json.dumps(summary,indent=2)+'\n');(out/'head-regressions.json').write_text(json.dumps(reg,indent=2)+'\n')
adv=read(src/'distinct-finality-progress.json')['distinctAdvances'];verdict=result['result'] if 'result' in result else ('PASSED' if t['exitCode']==0 else 'FAILED')
text=f"""# Reprise v26 : verdict {verdict}

Contrôleur original 95043, checkout 80292a4d, cœur ee5e278b, relais ad536163 réutilisé avec preuve des entrées compilateur inchangées. Sortie terminale {t['exitCode']}, {len(adv)} nouvelles finalités communes distinctes observées pour trois requises en 900 secondes. Aucun redémarrage ni modification runtime durant la mesure. Trois arrêts normaux sans OOM ; engagements primaires et témoins/signataires comparés aux reçus indépendants. Toutes les bases restent inchangées pendant les audits.

Changements vers des têtes plus anciennes : {len(reg)}, détail dans head-regressions.json. Un ACK de bloc, une hauteur rattrapée ou une finalité conservée ne qualifie pas l'avancement de finalité. Résultat exact dans result.json et distinct-finality-progress.json.

Fenêtres de diagnostics publics collectées pendant la campagne : refus temporaires CatalogAcquire -32005 sur VPS, perte de distinction entre erreur RPC et réponse invalide côté relais ; aucune erreur RPC dans la fenêtre Mac Docker collectée. Ces fenêtres bornées ne constituent pas les totaux de campagne et ne doivent pas être additionnées. Cette piste ne démontre pas une cause globale. Une mesure instantanée montre des écritures importantes sur le VPS, sans prouver saturation, durabilité physique ni causalité.

Audits hors ligne avec budget forensic 32768 blocs/512 Mio ; limites runtime inchangées. Configuration et requête VPS dérivées sur son hôte ; seules preuves publiques collectées. L'assertion initiale d'assemblage comparait à tort les 32 journaux historiques Mac aux 16 signataires actuellement possédés. Échec conservé ; correction après vérification des 16 engagements d'arrêt et des 16 engagements historiques supplémentaires, sans relancer les audits. Aucun transfert de base, clés ou configuration Mac vers le VPS. Aucune image reconstruite. Participation par propriétaire et état retenu détaillés dans audit-summary.json. Finalité WAN, cause des changements de branche, autres exigences de production, bridge/fair launch et revue indépendante demeurent ouverts sauf critères précisément démontrés par les preuves.
"""
(out/'README.md').write_text(text)
note=f"\n\n2026-10-06 — V26 {verdict} : original 95043 / sortie {t['exitCode']}, checkout 80292a4d, cœur ee5e278b ; {len(adv)} nouvelles finalités communes distinctes, trois requises en 900 s. Trois arrêts normaux, engagements/témoins et hashes vérifiés après arrêt. {len(reg)} changements vers des têtes plus anciennes conservés. Diagnostics publics bornés : CatalogAcquire -32005 sur VPS, interprété comme invalid-response par le relais ; causalité non démontrée. Limites inchangées, aucune image reconstruite ni relance. [Preuves V26](qualification/restored-three-node-v26-terminal-20261006/README.md). Tous critères non démontrés et revue indépendante restent ouverts.\n"
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('V26 archived',verdict,len(adv))
