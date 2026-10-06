from pathlib import Path
import json,hashlib,shutil,collections
repo=Path('/Users/boris/Dev/blockchain-nano');base=Path('/private/tmp');src=base/'nano-restored-three-node-v18-namespace-selection-20261006/campaign';out=repo/'docs/qualification/restored-three-node-v18-terminal-20261006'
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
t=read(src/'terminal-exit.json');assert t['originalSession']==53478 and t['exitCode'] in [0,1]
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
for path,name in [('nano-v18-post-stop-native-diagnostic-20261006','post-stop-mac'),('nano-v18-post-stop-vps-audit-20261006','post-stop-vps')]:shutil.copytree(base/path,out/name)
for name in ['nano-summarize-v17-retry-live-20261006.py','nano-restored-three-node-v18-controller-20261006.log','nano-v18-post-stop-native-diagnostic-20261006.py','nano-v18-vps-local-audit-20261006.py','nano-run-v18-vps-local-audit-20261006.py']:
 shutil.copy2(base/name,out/name)
if (src/'live-resource-snapshot').exists():shutil.copytree(src/'live-resource-snapshot',out/'live-resource-snapshot')
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
text=f'''# Reprise v18 namespace de sélection : admission continue — {status}

Contrôleur original 53478, checkout f457bd3b, core d8d5b46c / relais 5288e024. Sortie {t['exitCode']}. Critère original : trois nouvelles finalités communes dans 900 secondes depuis 1284. Avancées observées : {len(advance)}. Résultat exact et erreurs conservés. Aucun redémarrage ni prolongation ; aucune compilation pendant la campagne.

Trois reçus d'arrêt et audits indépendants read-only : engagements, journaux/témoins, historique canonique et ancêtre finalisé vérifiés. Fichiers inchangés selon les manifests. Sources et binaires identifiés dans la préparation. Pas de transfert inter-hôtes de base ; ni rejeu complet du fork-choice ni portabilité physique qualifiés par ces audits.

Les observations et la revue de source du catalogue sont conservées. Le correctif de sélection malgré la rotation du namespace est qualifié séparément en natif et Docker ARM64/AMD64 ; cela ne prouve pas la cause WAN complète. La revue de source conserve la perte des causes RPC et la pression éventuelle du graphe comme diagnostics à établir. Les compteurs de relais sont des tentatives, pas des admissions uniques. L'alternance observée ne prouve pas à elle seule la convergence. Les causes des refus restent à établir dans leur périmètre réel.

Toutes les autres exigences de production et la revue indépendante restent ouvertes. Les échecs historiques sont conservés.
'''
(out/'README.md').write_text(text)
shutil.copy2(base/'nano-archive-v18-20261006.py',out/'archive.py')
note=f'\n\n2026-10-06 — Reprise v18 namespace de sélection {status} : contrôleur 53478 / sortie {t["exitCode"]}, checkout f457bd3b, cœur d8d5b46c / relais 5288e024 ; {len(advance)} avancées communes observées selon le critère original. Traces de relais/ressources et hypothèses de parcours enregistrées ; cause WAN globale non prouvée. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v18](qualification/restored-three-node-v18-terminal-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
save(out/'manifest.json',{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'})
print('Archived',status,out)
