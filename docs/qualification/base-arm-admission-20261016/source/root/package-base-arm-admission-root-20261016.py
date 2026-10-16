#!/usr/bin/env python3
"""Publish bounded fixes and results without private metadata or renewed authority."""
import hashlib,json,pathlib,subprocess
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
HEAD='e148e63c326e8f6ae67ce65204ca08e30973dcb6'
DOCS=['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']
D=R/'docs/qualification/base-arm-admission-20261016'
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def git(*a):return subprocess.check_output(['/usr/bin/git','--no-optional-locks','-C',str(R),*a],timeout=30)
def put(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 with p.open('xb')as f:f.write(b)
assert git('rev-parse','HEAD').decode().strip()==HEAD
assert git('diff','--cached','--name-only')==b''
assert set(git('diff','--name-only').decode().splitlines())=={'tools/bridge-source/finality.mjs','tools/bridge-source/finality.test.mjs'}
assert git('ls-files','--others','--exclude-standard')==b''
for tree in ('src','include','network','third_party','tests/capella','tests/support','tests/multihost'):
 assert git('rev-parse','HEAD:'+tree)==git('rev-parse','68d64a5330d16c971dd603b56e0f14c88c1459c8:'+tree)
A=E/'arm-json-admission-qualification-root-20261016-01'
B=E/'base-finality-snapshot-qualification-root-20261016-01'
T=E/'metadata-fence-trace-root-20261016-01'
a=json.loads((A/'result-public.json').read_bytes());b=json.loads((B/'result-public.json').read_bytes());t=json.loads((T/'result-public.json').read_bytes())
assert a['passed'] and a['checkCount']==5 and not a['controllerMainExecuted']
assert b['passed'] and b['counts']=={'tests':31,'suites':0,'pass':31,'fail':0,'cancelled':0,'skipped':0,'todo':0}
assert t['traceCompleted'] and t['rounds']==3 and t['metadataCopies']==6 and t['postHoldFenceSamples']==42 and t['changedAttributes']==['st_ctime_ns']
assert not any(x['productionQualified']for x in (a,b,t))
for z in b['sourceFiles']:assert pin((R/z['path']).read_bytes())==z['after']
assert pin((A/'host.py').read_bytes())==a['fixedController']
assert not D.exists();D.mkdir(mode=0o755)
mapping={
 'reports/arm-metadata-trace.json':T/'result-public.json',
 'reports/arm-admission.json':A/'result-public.json',
 'reports/base-baseline.json':B/'baseline-public.json',
 'reports/base-candidate.json':B/'result-public.json',
 'logs/base-baseline.stdout.log':B/'baseline-new-regressions-stdout.log',
 'logs/base-baseline.stderr.log':B/'baseline-new-regressions-stderr.log',
 'logs/base-candidate.stdout.log':B/'candidate-finality-file-stdout.log',
 'logs/base-candidate.stderr.log':B/'candidate-finality-file-stderr.log',
 'source/arm-host.py':A/'host.py',
 'source/root/qualify-arm-json-admission-root-20261016.py':E/'qualify-arm-json-admission-root-20261016.py',
 'source/root/trace-arm-metadata-first-admission-root-20261016.py':E/'trace-arm-metadata-first-admission-root-20261016.py',
 'source/root/qualify-base-finality-snapshot-root-20261016.py':E/'qualify-base-finality-snapshot-root-20261016.py',
 'source/root/qualify-base-finality-snapshot-root-initial-20261016.py':E/'qualify-base-finality-snapshot-root-initial-20261016.py',
 'source/root/package-base-arm-admission-root-20261016.py':E/'package-base-arm-admission-root-20261016.py',
}
for leaf,p in mapping.items():
 assert '.private' not in leaf and p.is_file() and not p.is_symlink();put(D/leaf,p.read_bytes())
scope=dict(format='nano-base-arm-admission-scope-v1',repositoryHeadBeforePublication=HEAD,effectiveCppSourceRevision='68d64a5330d16c971dd603b56e0f14c88c1459c8',cppSourceTreesUnchanged=True,newControllerSource=a['fixedController'],armHistoricalHostFenceFailureResolved=False,originalMetadataRowsRecovered=False,armFullHostQualification=False,historicalCauseProven=False,freshCampaignAuthorized=False,runtimeDutiesResumed=False,commonFinalityQualified=False,bridgeImplemented=False,mintQualified=False,eligibleForMint=False,cryptographicFinalityVerified=False,productionQualified=False,oldAdmissionsRetargeted=False,cppCommands=0,dockerBuilds=0,oldCxxTestsReplayed=False)
put(D/'scope.json',(json.dumps(scope,sort_keys=True,indent=2)+'\n').encode())
readme='''# Réponses Base et admission ARM — 16 octobre 2026

L’observateur Base copie désormais les champs vérifiés de chaque header et reçu dès que la requête correspondante se termine. Ces copies précèdent les autres attentes parallèles et la vérification canonique d’une tête finalisée avancée. Un cache partagé ne peut plus effacer une divergence déjà observée pendant ces attentes.

Trois nouveaux scénarios reproduisent des acceptations incorrectes dans l’ancienne version : remplacement du hash finalisé avancé, effacement d’un changement de log dans le second reçu, puis remplacement du header pendant une autre réponse parallèle différée. Les trois assertions de refus échouent avec l’ancienne source. Avec le correctif, le fichier entier passe une fois : 30 cas feuilles, dont trois nouveaux, et leur groupe parent, soit **31 résultats Node sans échec ni skip**. Aucun test du transport inchangé n’est rejoué. Le premier analyseur du wrapper attendait TAP alors que Node avait produit son reporter natif : son échec est conservé dans la source initiale et le reçu de lecture ; les sorties du baseline sont réutilisées sans rejouer les tests. Le candidat utilise explicitement TAP.

Ces tests JavaScript tournent avec Node natif macOS v25.9.0 et des RPC injectés ; aucune requête réseau. L’image Docker C++ conservée ne déclare pas de paquet Node. Aucune image ni dépendance n’est téléchargée ou reconstruite. Aucune qualification C++ n’est déduite de ces tests.

Une trace séparée reproduit, sur trois paires de nouvelles copies des deux JSON du worker ARM, un changement de `st_ctime_ns` après leur admission immédiate : six copies, 42 observations après admission, 36 différences de ctime, octets inchangés et zéro incohérence FD/path pendant chaque lecture. Le processus responsable du changement n’est pas identifié. Cette observation correspond à une fragilité réelle de l’admission immédiate ; elle ne prouve pas quelle ancienne ligne avait causé l’échec historique, car la table complète après worker n’avait pas été conservée.

Le nouveau contrôleur ajoute fsync, vérification des deux copies par leurs octets connus, puis stabilité d’au moins trois observations identiques sur deux secondes **avant leur première admission**. Les anciennes autorités ne sont jamais rafraîchies. Il conserve la table complète après worker et, lors d’un premier refus, les neuf attributs attendus/FD/path sans lecture supplémentaire de contenu par le diagnostic. Le refus original demeure prioritaire même si le diagnostic ne peut pas être sauvegardé. Les raccordements du main aux pins stdout/binding et la sauvegarde de sa table sont revus statiquement ; le compteur de lectures diagnostiques est vérifié par lecture du code, sans trace d’E/S indépendante.

Les fonctions exactes sont extraites par AST, sans importer ou lancer les contrôleurs historiques. Cinq contrôles synthétiques passent : admission immédiate refusée après mutation, stabilisation des deux nouvelles copies, mutation postérieure refusée sans rafraîchissement, mauvais octets refusés avant admission, exception originale préservée si la sauvegarde diagnostique échoue. La stabilisation contient quatre observations en 3 017 ms. Ces essais ne relancent ni M, CXX, liens, CLI, Docker, DB, signatures ou réseau. Le nouveau contrôleur complet **n’a pas été exécuté** ; son code et ces contrôles ciblés ne transforment pas l’ancien hôte ARM en réussite.

L’ancien [échec de la barrière ARM et les assemblages](../runtime-core-relink-20261016/README.md) restent conservés, tout comme V37/campagne08, les échecs Doppler et les anciennes admissions. Aucune ancienne identité manquante n’est récupérée ; aucune reprise de duties ni finalité commune nouvelle n’est démontrée. La campagne fraîche attend toujours son autorisation dédiée : l’accord de transfert/audit antérieur ne l’autorise pas.

Le résultat Base conserve `eligibleForMint:false` et `cryptographicFinalityVerified:false`. Ce correctif ne réalise aucun bridge ni preuve d’inclusion/dérivation authentifiée. Le sens du signal finalisé est décrit dans la [spécification de dérivation Base](https://docs.base.org/base-chain/specs/protocol/consensus/derivation) ; sa simple observation RPC ne constitue pas cette preuve.

Restent ouverts : restauration/duties/finalité commune, cycle complet et sanctions des validateurs, partitions/WAN/charge, opérations prolongées, CI complète bloquée par facturation, revue indépendante et architecture économique/bridge/termes Doppler. Aucun EIP, fork ou statut de production n’est promu. Goal actif.

Les rapports publics et logs sont listés dans `MANIFEST.json`. Les traces brutes de métadonnées privées restent sur le Mac. `source/arm-host.py` est le contrôleur prospectif, pas un reçu d’exécution. Les scripts Root conservent leurs chemins et pins de la campagne locale ; ils ne sont pas des CLI portables de production.
'''
put(D/'README.md',readme.encode())
payloads={str(p.relative_to(D)):pin(p.read_bytes())for p in sorted(D.rglob('*'))if p.is_file()}
mf=dict(format='nano-base-arm-admission-public-evidence-v1',payloads=payloads,payloadCount=len(payloads),payloadBytes=sum(x['bytes']for x in payloads.values()),productionQualified=False)
put(D/'MANIFEST.json',(json.dumps(mf,sort_keys=True,indent=2)+'\n').encode())
prefix='''> **16 octobre 2026 — Réponses Base et admission ARM corrigées dans leur périmètre.**
> [Sources, résultats et limites](qualification/base-arm-admission-20261016/README.md). Trois défauts de références RPC mutables sont reproduits sur l’ancienne source ; le correctif passe les 30 cas feuilles du fichier et leur groupe parent (31 résultats Node). Cinq contrôles métadonnées passent pour l’admission prospective des JSON ARM et le refus des mutations après admission.
> Trois paires de nouvelles copies montrent une dérive de ctime à octets inchangés. Le contrôleur complet corrigé n’est pas exécuté ; l’ancien échec hôte ARM reste conservé et sa ligne exacte non récupérée. Aucune ancienne autorité rafraîchie, duty repris, finalité commune démontrée ou image reconstruite.
> Campagne fraîche sans autorisation dédiée, restauration/consensus/WAN/charge, cycle complet des validateurs, opérations prolongées, bridge et termes Base–NANO/Doppler, CI et revue indépendante restent ouverts. Observation RPC seulement : mint et finalité cryptographique non qualifiés. Historique intégral conservé ; aucun EIP/fork ni production promu. Goal actif.

'''.encode()
receipt={}
for leaf in DOCS:
 p=R/'docs'/leaf;before=p.read_bytes();assert before==git('show',HEAD+':docs/'+leaf)
 p.write_bytes(prefix+before);assert p.read_bytes()[len(prefix):]==before
 receipt[leaf]=dict(old=pin(before),new=pin(p.read_bytes()),oldBodyByteExact=True)
p=R/'tools/bridge-source/README.md';before=p.read_bytes()
append='''
## 16 octobre 2026 — copies à chaque réponse parallèle

Chaque header et reçu est maintenant projeté dès sa propre réponse, avant
l’attente des autres RPC parallèles et la vérification d’une tête finalisée
avancée. Trois nouveaux cas reproduisent l’effacement de divergences par un
cache mutable dans l’ancienne version ; le fichier complet passe ensuite :
30 cas feuilles et leur groupe parent, soit 31 résultats Node sans skip.
Les 15 tests du transport inchangé ne sont pas rejoués. Le module demeure
une observation RPC avec `eligibleForMint:false`, sans bridge ni preuve
cryptographique de finalité.
[Rapport et limites](../../docs/qualification/base-arm-admission-20261016/README.md).
'''
updated=before.decode().replace('Vingt-sept tests avec RPC injecté passent','Les vingt-sept tests initiaux avec RPC injecté passent').replace('Le lot de l’observateur compte désormais 27 tests','Le lot de l’observateur comptait 27 tests')
p.write_bytes((updated+append).encode())
put(E/'base-arm-admission-publication-receipt-20261016.json',(json.dumps(dict(headBefore=HEAD,documents=receipt,manifest=pin((D/'MANIFEST.json').read_bytes()),productionQualified=False),sort_keys=True,indent=2)+'\n').encode())
print(json.dumps(dict(payloadCount=len(payloads),manifest=pin((D/'MANIFEST.json').read_bytes()),oldDocumentBodiesByteExact=True,productionQualified=False),sort_keys=True))
