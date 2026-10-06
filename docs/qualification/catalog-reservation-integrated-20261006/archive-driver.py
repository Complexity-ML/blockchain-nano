from pathlib import Path
import json,hashlib,shutil,subprocess
r=Path('/Users/boris/Dev/blockchain-nano');p=Path('/private/tmp/nano-catalog-reservation-integrated-docker-20261006');n=Path('/private/tmp/nano-catalog-reservation-integrated-native-20261006')
m=json.loads((p/'source-manifest.json').read_text());assert len(m)==350 and all(hashlib.sha256((r/f).read_bytes()).hexdigest()==h for f,h in m.items())
modes=['catalog-concurrency','configured-runtime','automatic-recovery','capture-control','worker-beacon-reads','worker-queued-bls-rpc','generation-reservation']
for arch in ['arm64','amd64']:
 assert json.loads((p/(arch+'-terminal-exit.json')).read_text())['exitCode']==0
 q=p/(arch+'-proof');assert set((q/'rebuilt-sources.txt').read_text().split())=={'rpc.cpp','consensus_capella_runtime.cpp'}
 for stage in ['controller','runtime-controller','regression-controller']:assert (q/(stage+'-exit.txt')).read_text().strip()=='0'
 for mode in modes:assert (q/('test-'+mode+'-exit.txt')).read_text().strip()=='0'
for mode in modes:assert json.loads((n/('test-'+mode+'-exit.json')).read_text())['exitCode']==0
for q in [p/'arm64-proof',p/'amd64-proof',n]:
 def scenario(mode,name):
  rows=[json.loads(l) for l in (q/('test-'+mode+'.log')).read_text().splitlines() if l.startswith('{')];found=[v for v in rows if v.get('scenario')==name];assert len(found)==1;return found[0]
 v=scenario('generation-reservation','catalog-generation-reservation');assert [c['published'] for c in v['cases']]==[True,False,False,True];assert all(c['rotated'] and c['primaryUnchanged'] and c['capacityRecovered'] for c in v['cases'])
 v=scenario('worker-queued-bls-rpc','catalog-busy-http-control');assert v['realHttpTransport'] and v['busyRpcCode']==-32005 and v['generationAndReleaseResponsive'] and v['unauthorizedRejected'] and v['primaryUnchanged']
 v=scenario('worker-beacon-reads','catalog-worker-control-progress');assert v['acquirerRejectedBusy'] and not v['queuedGenerationExpired'] and not v['queuedReleaseExpired'] and v['allReservationsDrained'] and v['primaryUnchanged']
a=r/'docs/qualification/catalog-reservation-integrated-20261006';a.mkdir(exist_ok=False)
for arch in ['arm64','amd64']:shutil.copytree(p/(arch+'-proof'),a/arch)
(a/'native').mkdir()
for f in n.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.d']:shutil.copy2(f,a/'native'/f.name)
for f in p.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sh','.sha256']:shutil.copy2(f,a/f.name)
shutil.copy2('/private/tmp/nano-build-catalog-reservation-integrated-native-20261006.py',a/'native-driver.py');shutil.copy2('/private/tmp/nano-catalog-reservation-integrated-native-controller-20261006.log',a/'native-controller.log');shutil.copy2(__file__,a/'archive-driver.py')
(a/'implementation.patch').write_text(subprocess.check_output(['git','diff','--','CMakeLists.txt','network/capella_inventory_sessions.hpp','src/rpc.cpp','tests/support/capella_catalog_concurrency.hpp','tests/test_capella_transition.cpp'],cwd=r,text=True))
(a/'README.md').write_text('''# Catalogue : progression bornée, refus occupé et réservation suivie

Parent main 5fa219bd ; 350 sources publiques figées, identiques au worktree. Originaux ARM64 57019, AMD64 81866 et natif 79512 : sorties 0 vérifiées. Sept modes concernés passent sur chaque architecture. Scan de tous les TUs : rpc.cpp et consensus_capella_runtime.cpp seuls consommateurs du layout reconstruits ; autres objets réutilisés. Runtimes, lecteurs et fixtures reliés au nouvel archive. Hashes et commandes conservés. Images/volumes existants, aucun rebuild d’image.

Une capture concurrente reçoit immédiatement un refus typé, traduit en RPC -32005. Pas de travailleur supplémentaire ni de hausse des limites. La réservation active est suivie séparément des tombstones : les rotations de requêtes étrangères n’annulent plus une capture autorisée. Son propre retrait reste mémorisé avant/après rotation, sans résurrection. Arrêt, expiration, annulation, propriétaires, quotas et budget de snapshot restent contrôlés. Une seule réservation, bornée, libérée après drainage.

Nouveaux scénarios effectivement exécutés : deux travailleurs/quatre jobs ne font plus expirer génération/release lorsqu’un acquéreur concurrent reçoit occupé ; quatre rotations (sans rapport, retrait propre avant/après rotation, propriétaire étranger) préservent exactement les deux captures légitimes ; HTTP réel via --queued-bls-rpc vérifie mauvais token, code occupé et contrôles réactifs. Greps et assertions de scénario obligatoires dans les pilotes empêchent un mode omis de compter comme qualification. Le mode --http qui omettait ce cas et le helper ARM64 sans fixture sont conservés dans catalog-generation-reservation-counterfactual-20261006 ; ils ne sont pas utilisés comme preuve du cas HTTP nouveau.

Sept modes : concurrence catalogue (32 travailleurs, quotas, doublons et 128 courses), runtime configuré, reprise automatique/témoins, capture/contrôle (release/stop/expiry/cancel), travailleurs, HTTP occupé, réservation/génération. Deux consommateurs ABI reconstruits sur les trois architectures, parents vérifiés inchangés. Le quorum, le stockage durable, les règles EVM et les limites réseau ne changent pas.

Ce lot qualifie les mécanismes internes reproduits, pas la cause globale WAN, une nouvelle finalité commune, les partitions, les cycles complets ou les autres gates de production. Contention causée par d’autres tâches RPC et transmission des votes récents restent ouvertes. Aucun nœud de campagne, fond réel, vente ni déploiement public. Revue indépendante et lancement Base–Nano restent ouverts.
''')
m={str(f.relative_to(a)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(a.rglob('*')) if f.is_file()};(a/'manifest.json').write_text(json.dumps(m,indent=2))
for doc in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 with (r/'docs'/doc).open('a') as f:f.write('\n\n2026-10-06 — Lot catalogue intégré : refus occupé immédiat (-32005), réservation suivie à travers rotation, retrait propre/arrêt/expiry/cancel conservés. Originaux 57019/81866/79512 sorties 0, sept modes natif/ARM64/AMD64, 350 sources figées et deux consommateurs ABI reconstruits. Scénarios HTTP réel et génération vérifiés explicitement ; modes omis et échecs historiques conservés. Quotas/threads/quorum inchangés. Cause globale et finalité WAN restent ouvertes ; autres tâches RPC/votes récents à qualifier avant clôture production. [Preuves](qualification/catalog-reservation-integrated-20261006/README.md).\n')
print('Archived verified lot',len(m),'files')
