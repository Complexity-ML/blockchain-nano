from pathlib import Path
import json,hashlib,shutil,collections
repo=Path('/Users/boris/Dev/blockchain-nano');base=Path('/private/tmp');src=base/'nano-restored-three-node-v13-parent-cache-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v13-parent-cache-terminal-20261006'
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
t=read(src/'terminal-exit.json');assert t['originalSession']==80316 and t['exitCode'] in [0,1]
r=read(src/'result.json');assert r['result'] in ['PASSED','FAILED'] and r['stopReceipts']==3
assert (r['result']=='PASSED')==(t['exitCode']==0)
a=read(src/'post-stop-audit-terminal-exits.json');assert a['nativeExitCode']==a['vpsExitCode']==0
assert read(src/'native-state.json')['exitCode']==0
out.mkdir(exist_ok=False)
for f in src.iterdir():
 if not f.is_file():continue
 if f.name.endswith('.state.json'):
  d=read(f);assert not d['State']['Running'] and d['State']['ExitCode']==0 and not d['State']['OOMKilled'];save(out/f.name,{k:d[k] for k in ['State','Name','Image']})
 else:shutil.copy2(f,out/f.name)
if (src/'live-observations').exists():shutil.copytree(src/'live-observations',out/'live-observations')
for path,name in [('nano-v13-parent-cache-post-stop-native-diagnostic-20261006','post-stop-mac'),('nano-post-v13-parent-cache-vps-audit-20261006','post-stop-vps')]:shutil.copytree(base/path,out/name)
for name in ['nano-v13-live-relay-observation-20261006.py','nano-restored-three-node-v13-parent-cache-controller-20261006.log','nano-v13-parent-cache-post-stop-native-diagnostic-20261006.py','nano-post-v13-parent-cache-vps-audit-20261006.py']:shutil.copy2(base/name,out/name)
summary=[];trace={}
for i,name in enumerate(['mac-native','mac-docker','vps-docker']):
 receipt=read(out/f'{i}-external-stop-receipt.json');audit=read(out/(f'post-stop-mac/{name}-capture.json' if i<2 else 'post-stop-vps/capture.json'));assert audit['restart']==receipt['restart'];pins={k['index']:k['restart'] for k in audit['validatorKeys']}
 for k in receipt['validatorKeys']:assert pins[k['index']]==k['restart']
 assert audit['canonicalReplay']['canonicalHistoryVerified'] and audit['canonicalReplay']['externalFinalizedAncestorVerified']
 summary.append({'name':name,'restart':receipt['restart'],'finalized':receipt['finalized'],'validatorHealthy':receipt['validator']['healthy'],'validatorFailure':receipt['validator']['failure'],'proposals':receipt['validator']['proposalsAdmitted'],'attestations':receipt['validator']['attestationsAdmitted'],'canonicalReplay':audit['canonicalReplay']})
 rows=[]
 for line in (out/['native.log','docker-mac.log','vps.log'][i]).read_text().splitlines():
  try:rows.append(json.loads(line))
  except ValueError:pass
 trace[name]={'catalogues':dict(collections.Counter(e.get('kind','?')+'|'+str(e.get('reason')) for e in rows if e.get('event')=='test-capella-catalog-relay')),'dispatchTransitions':[e for e in rows if e.get('event')=='test-core-dispatch-transition'],'payloadsAdmittedByKind':{kind:sum(e.get('payloadsAdmitted',0) for e in rows if e.get('event')=='test-capella-catalog-relay' and e.get('kind')==kind) for kind in ['block','attestation']}}
save(out/'audit-summary.json',summary);save(out/'trace-summary.json',trace)
status=r['result'];advance=read(out/'distinct-finality-progress.json')['distinctAdvances']
text=f'''# Reprise v13 : continuations par parent/slot — {status}

Contrôleur original 80316, checkout 24ea13b0, core 5787915e / relais 76bb2468. Sortie {t['exitCode']}. Critère original : trois nouvelles finalités communes dans 900 secondes depuis 1284. Avancées observées : {len(advance)}. Résultat et erreur exacts conservés dans result.json. Aucun redémarrage ni prolongation ; aucune compilation pendant la campagne.

Trois reçus d'arrêt et audits read-only indépendants : engagements exacts, journaux/témoins et historique canonique vérifiés, empreintes de fichiers inchangées. Sources, binaires et préparation identifiés séparément. Aucun transfert inter-hôtes de base. Ces audits ne qualifient ni le rejeu complet du fork-choice ni la portabilité physique.

Les compteurs de relais décrivent des tentatives, pas des objets uniques. Des succès de catalogues vides ne prouvent aucune admission. Les relevés de ressources sont ponctuels, pas une qualification de charge. Causes des refus, caches/pruning, WAN, partitions, cycles/rotations, autres critères de production et revue indépendante restent ouverts. Les échecs historiques restent conservés.
'''
(out/'README.md').write_text(text)
shutil.copy2(base/'nano-archive-v13-parent-cache-20261006.py',out/'archive.py')
note=f'\n\n2026-10-06 — Reprise v13 {status} : contrôleur 80316 / sortie {t["exitCode"]}, checkout 24ea13b0, cœur 5787915e et relais 76bb2468 ; {len(advance)} avancées communes observées selon le critère original. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v13](qualification/restored-three-node-v13-parent-cache-terminal-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
save(out/'manifest.json',{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'})
print('Archived',status,out)
