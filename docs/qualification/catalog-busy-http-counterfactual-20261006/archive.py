from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');out=repo/'docs/qualification/catalog-busy-http-counterfactual-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
b=tmp/'nano-catalog-busy-http-baseline-20261006';d=tmp/'nano-catalog-busy-http-draft-20261006'
assert read(b/'compile-exit.json')['exitCode']==0 and read(b/'run-authorized-exit.json')['exitCode']==0
assert read(d/'compile-enhanced-exit.json')['exitCode']==0 and read(d/'run-enhanced-exit.json')['exitCode']==0
for n,h in read(d/'enhanced-source-manifest.json').items():assert sha(d/n)==h
before=[json.loads(l) for l in (b/'run-authorized.log').read_text().splitlines()];after=[json.loads(l) for l in (d/'run-enhanced.log').read_text().splitlines()];assert len(before)==4 and len(after)==7
assert next(x for x in before if x['scenario']=='busy-then-ready')['success']==False
assert next(x for x in after if x['scenario']=='busy-then-ready')['success']==True
for x in after:assert x['completed']==1 and x['releases']==1
assert next(x for x in after if x['scenario']=='late-handle')['lateHandleCloses']==1
out.mkdir(exist_ok=False)
for name,source in [('baseline',b),('draft',d)]:
 dest=out/name;dest.mkdir()
 for p in source.iterdir():
  if p.is_file() and p.suffix in ['.json','.log','.cpp']:shutil.copy2(p,dest/p.name)
 shutil.copytree(source/'network',dest/'network')
 (dest/'archived-current-program-hash.json').write_text(json.dumps({'sha256':sha(source/'probe'),'binaryNotCommitted':True},indent=2)+'\n')
shutil.copy2(tmp/'nano-build-catalog-busy-draft-20261006.py',out/'build-initial-draft.py');shutil.copy2(tmp/'nano-archive-catalog-busy-http-counterfactual-20261006.py',out/'archive.py')
(out/'scope.json').write_text(json.dumps({'sourceRevision':'63a19de587fa64ba7e3ee57f6db4675ae471195f','nativeHttpScenarios':7,'productionClientHeadersUsed':True,'rpcBackendSimulated':True,'catalogueEmpty':True,'baselineBusyRecovery':False,'draftBusyRecovery':True,'runtimeIntegrated':False,'arm64DockerQualified':False,'amd64DockerQualified':False,'wanFinalityQualified':False,'productionQualified':False,'initialDraftFourScenarioRunNotFinalSafetyQualification':True},indent=2)+'\n')
(out/'README.md').write_text('''# Refus explicite de capture : contre-exemple et prototype HTTP

Base 63a19de5 arrêtée et auditée après V26. Client HTTP et relais de production exécutés sur loopback ; serveur RPC simulé, catalogue vide. Le refus exact -32005 / « Capella inventory capture busy » est perdu par Core et devient une réponse invalide. Dans le contrôle initial, la capture disponible au second appel n'est pas retentée : un acquire, échec. Contrôle disponible immédiatement : succès.

Prototype natif : uniquement ce refus explicite sans result produit un marqueur interne. Le relais retente avec même principal/génération/request après l'intervalle fourni par l'appelant ; délai global inchangé. Aucun rejeu implicite dans Core, aucune reprise sur transport perdu ou autre erreur. Sept scénarios terminent une seule fois et retirent la requête une seule fois : disponibilité immédiate ; busy puis disponible ; autre erreur ; busy persistant borné ; réponse perdue ; busy tardif ; handle tardif nettoyé par Close. Les derniers trois n'émettent qu'un acquire. Le compteur de tentatives persistent peut varier avec l'ordonnanceur ; il ne constitue pas une limite de consensus.

L'exécution initiale du contrôle a échoué au bind dans l'isolation avant le premier scénario. Même binaire exécuté avec accès loopback autorisé, sans recompilation. Échec conservé. Le premier prototype à quatre scénarios ne pompait pas les opérations asynchrones de nettoyage après completion : il est conservé mais exclu de la qualification finale de retrait. Le prototype enrichi attend ces opérations et vérifie release/close effectifs. Sources et hashes du prototype enrichi identifiés séparément.

Prototype seulement, pas de changement runtime du dépôt ; sept scénarios natifs ciblés ne prouvent ni l'équité inter-pairs, ni une capture réelle du moteur, ni la finalité WAN. Prochaine étape : intégration avec intervalle opérationnel déjà configuré, propagation au parcours head/catalogue, contrôles de transport et régression sur natif/Docker ARM64/Docker AMD64. Ne pas relancer une campagne réseau avant qualification du lot. V26, changements de branches, participation insuffisante, revue indépendante et autres exigences de production restent ouverts.
''')
note='\n\n2026-10-06 — Contre-exemple HTTP après V26 : le refus explicite de capture -32005 disparaît dans Core ; busy puis disponible échoue sans second acquire. Prototype natif borné : sept scénarios, reprise sur ce seul refus explicite, même identité de requête, pas de rejeu des réponses perdues/inconnues/tardives, release unique et nettoyage du handle tardif. Backend RPC simulé et catalogue vide ; pas de capture moteur ni de finalité WAN qualifiées. Bind initial interdit par isolation conservé, même binaire réparé par accès loopback autorisé. Prototype non intégré, qualification Docker/propagation head-catalogue restantes. [Preuves](qualification/catalog-busy-http-counterfactual-20261006/README.md). Tous écarts non démontrés et revue indépendante restent ouverts.\n'
for n in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/n;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Archived explicit capture-busy counterexample and scoped native prototype')
