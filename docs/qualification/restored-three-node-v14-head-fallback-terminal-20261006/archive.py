from pathlib import Path
import json,hashlib,shutil,collections
repo=Path('/Users/boris/Dev/blockchain-nano');base=Path('/private/tmp');src=base/'nano-restored-three-node-v14-head-fallback-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v14-head-fallback-terminal-20261006'
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
t=read(src/'terminal-exit.json');assert t['originalSession']==40209 and t['exitCode'] in [0,1]
r=read(src/'result.json');assert r['result'] in ['PASSED','FAILED'] and r['stopReceipts']==3
assert (r['result']=='PASSED')==(t['exitCode']==0)
a=read(src/'post-stop-audit-terminal-exits.json');assert a['nativeExitCode']==a['vpsExitCode']==0
assert read(src/'native-state.json')['exitCode']==0
for name,digest in read(src/'controller-hashes.json').items():assert hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest,'Controller source changed'
out.mkdir(exist_ok=False)
for f in src.iterdir():
 if not f.is_file():continue
 if f.name.endswith('.state.json'):
  d=read(f);assert not d['State']['Running'] and d['State']['ExitCode']==0 and not d['State']['OOMKilled'];save(out/f.name,{k:d[k] for k in ['State','Name','Image']})
 else:shutil.copy2(f,out/f.name)
if (src/'live-observations').exists():shutil.copytree(src/'live-observations',out/'live-observations')
for path,name in [('nano-v14-head-fallback-post-stop-native-diagnostic-20261006','post-stop-mac'),('nano-post-v14-head-fallback-vps-audit-20261006','post-stop-vps')]:shutil.copytree(base/path,out/name)
for name in ['nano-v14-live-relay-observation-20261006.py','nano-restored-three-node-v14-head-fallback-controller-20261006.log','nano-v14-head-fallback-post-stop-native-diagnostic-20261006.py','nano-post-v14-head-fallback-vps-audit-20261006.py']:shutil.copy2(base/name,out/name)
shutil.copytree(base/'nano-v14-frontier-docker-20261006',out/'frontier-diagnostic')
shutil.copy2(base/'nano-prepare-v14-frontier-docker-20261006.py',out/'frontier-diagnostic/prepare.py')
shutil.copy2(base/'nano-v14-frontier-controller-20261006.log',out/'frontier-diagnostic/controller.log')
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
 sends=[e for e in rows if e.get('event')=='test-consensus-send-completion']
 trace[name]['sendCompletionByKind']={}
 for kind in sorted({e.get('kind','') for e in sends}):
  selected=[e for e in sends if e.get('kind','')==kind];durations=sorted(e['elapsedMs'] for e in selected)
  trace[name]['sendCompletionByKind'][kind]={'attempts':len(selected),'outcomes':dict(collections.Counter(e.get('outcome') for e in selected)),'elapsedMs':{'min':durations[0],'median':durations[len(durations)//2],'max':durations[-1]},'writesCompleted':sum(e.get('writeCompletedMs',-1)>=0 for e in selected)}
save(out/'audit-summary.json',summary);save(out/'trace-summary.json',trace)
status=r['result'];advance=read(out/'distinct-finality-progress.json')['distinctAdvances']
text=f'''# Reprise v14 : parcours historique après refus de tête — {status}

Contrôleur original 40209, checkout ec3b6441, core 5787915e / relais 7248698d. Sortie {t['exitCode']}. Critère original : trois nouvelles finalités communes dans 900 secondes depuis 1284. Avancées observées : {len(advance)}. Résultat et erreur exacts conservés dans result.json. Aucun redémarrage ni prolongation ; aucune compilation pendant la campagne.

Trois reçus d'arrêt et audits read-only indépendants : engagements exacts, journaux/témoins et historique canonique vérifiés, empreintes de fichiers inchangées. Sources, binaires et préparation identifiés séparément. Aucun transfert inter-hôtes de base. Ces audits ne qualifient ni le rejeu complet du fork-choice ni la portabilité physique.

Diagnostic readonly avec le lecteur ARM existant vérifié : mêmes premières branches divergentes manquantes qu’avant v14 (10751/10753 vers Docker Mac, 10752 vers Mac natif), graphes de 394/495 blocs sous limite 1024, 32 votes récents échantillonnés sans références retenues. Empreintes 1389/1402 fichiers inchangées. Cela ne prouve pas à lui seul la cause réseau. Échec de lancement 127 (bibliothèque de test non montée) conservé, montage corrigé sans recompiler, lecteur terminal 76472/sortie 0.

Les compteurs de relais décrivent des tentatives, pas des objets uniques. Des succès de catalogues vides ne prouvent aucune admission. Les relevés de ressources sont ponctuels, pas une qualification de charge. Causes des refus, caches/pruning, WAN, partitions, cycles/rotations, autres critères de production et revue indépendante restent ouverts. Les échecs historiques restent conservés.
'''
(out/'README.md').write_text(text)
shutil.copy2(base/'nano-archive-v14-head-fallback-20261006.py',out/'archive.py')
note=f'\n\n2026-10-06 — Reprise v14 {status} : contrôleur 40209 / sortie {t["exitCode"]}, checkout ec3b6441, cœur 5787915e et relais 7248698d ; {len(advance)} avancées communes observées selon le critère original. Frontières manquantes inchangées, graphes sous capacité ; diagnostic avec lecteur existant réutilisé. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v14](qualification/restored-three-node-v14-head-fallback-terminal-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
save(out/'manifest.json',{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'})
print('Archived',status,out)
