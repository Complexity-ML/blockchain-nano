from pathlib import Path
import hashlib,json,shutil,subprocess
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');m=json.loads((r/'manifest.json').read_text());handles=json.loads((r/'original-handles.json').read_text())
assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
for kind in ['arm64Http','nativeHttp','amd64Http']:
 assert handles[kind]['status']=='TERMINAL' and handles[kind]['exitCode']==0
for arch in ['native','arm64','amd64']:
 assert handles[arch]['status']=='TERMINAL' and handles[arch]['exitCode']==0
 p=r/(arch+'-build');binding=json.loads((p/'binding.json').read_text());assert binding['allAffectedCoreConsumersRecompiled'] and binding['selectedProgramsRecompiled'] and binding['parentUnchanged'];assert binding['sourceFiles']==m['files']
 for gate in ['held-signed-inventories','ephemeral-sync-failfast','published-block-regression','signed-slashing-pruning-restart','startup-recovery-and-migration','cold-runtime-durable-wrapper']:
  assert json.loads((p/(gate+'-exit.json')).read_text())['exitCode']==0
 assert json.loads((p/'parent-held-counterexample-exit.json').read_text())['exitCode']==1
 assert (p/'parent-before.sha256').read_bytes()==(p/'parent-after.sha256').read_bytes()
 assert (p/'held-signed-inventories.log').read_text().count('published-signed-inventory-during-held-admission')==3
for rel,h in m['files'].items():
 assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256']
 assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
ci=json.loads((r/'ci-binding.json').read_text());assert hashlib.sha256((repo/ci['path']).read_bytes()).hexdigest()==ci['baseSHA256'];assert hashlib.sha256((r/'ci-core.sh').read_bytes()).hexdigest()==ci['draftSHA256']
out=repo/'docs/qualification/capella-published-signed-inventory-20261007';assert not out.exists();out.mkdir()
for arch in ['native','arm64','amd64']:
 (out/(arch+'-build')).mkdir()
 for p in (r/(arch+'-build')).iterdir():
  if p.is_file():shutil.copy2(p,out/(arch+'-build')/p.name)
 for suffix in ['build-original.log']:
  shutil.copy2(r/(arch+'-'+suffix),out/(arch+'-'+suffix))
for f in ['build-docker.py','build-native.py','review.json','original-handles.json','amd64-build-initial-gate-refusal.log','arm64-http-original.log','native-http-original.log','amd64-http-original.log','initial-draft-preparation-refusal.json','prepare-original.log','refine-original.log']:shutil.copy2(r/f,out/f)
shutil.copy2(__file__,out/'integrate.py')
public={**m,'status':'INTEGRATED_SCOPED_QUALIFICATION','fixtureCoverageQualified':True,'qualificationScriptExecuted':True,'draftStaticReview':{**m['draftStaticReview'],'compiled':True},'networkQualified':False,'cleanReleaseQualified':False,'pending':['restored WAN finality advancement','clean reproducible release and full relevant regression']};(out/'manifest.json').write_text(json.dumps(public,indent=2)+'\n')
(out/'README.md').write_text('''# Inventaires signés publiés

Les inventaires durables d’attestations et de preuves de slashing sont publiés atomiquement avec leur état/checkpoint. Les captures ne prennent plus le verrou d’admission ; sync, sorties et changements BLS conservent leur verrou. Les snapshots ne retiennent que les racines. Construction avant commit, bornes et annulation conservées ; aucune règle de signature ni format disque modifié.

Sur chaque architecture : contre-exemple ancien moteur attendu1, puis six parcours ciblés0. Les trois états vérifiés sont vote admis, slashing non vide et réouverture durable. Les dépendances compilateur de139 fichiers core identifient16 consommateurs ABI recompilés ; nano/test/capture sont recompilés et reliés avec la nouvelle archive, parents inchangés. HTTP/RPC passe séparément sur les trois environnements. Images Docker existantes réutilisées ; pas de build image ni de release propre qualifiée.

Refus AMD64 initial conservé : audit en lecture seule encore présent avant toute compilation. Le garde reste inchangé ; le parcours a commencé après sa fin. Les erreurs historiques V35 restent FAILED. Ces tests ne prouvent ni la cause globale du blocage réseau ni l’avancement d’une finalité commune. Production, Base–Nano/Doppler et revue indépendante restent ouverts.
''')
for rel in m['files']:shutil.copy2(r/'source'/rel,repo/rel)
shutil.copy2(r/'ci-core.sh',repo/ci['path']);shutil.copy2(r/'ci-binding.json',out/'ci-binding.json')
message='Inventaires signés publiés intégrés après qualification ciblée Mac natif/Docker ARM64/AMD64 : contre-exemple parent attendu1 puis six parcours0 par environnement,16 consommateurs core identifiés sur139 sources et recompilés, nano/test/capture reliés et parents inchangés. HTTP/RPC passe sur les trois environnements. [Sources, commandes et limites](qualification/capella-published-signed-inventory-20261007/README.md). Nouveau gate ajouté à la sélection CI stricte, exécution CI encore à vérifier. Aucun format disque, règle de signature, budget ou délai modifié ; images existantes réutilisées. V35 FAILED conservée. Reprise réseau, finalité commune, release propre, tous critères production, Base–Nano/Doppler et revue indépendante restent ouverts.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;text=p.read_text();marker='## État courant vérifié — 7 octobre 2026\n\n';assert marker in text;p.write_text(text.replace(marker,marker+message+'\n\nÉtat précédent conservé : ',1))
print('Sources integrated after all three original terminal qualifications; review and Git commit remain required')
