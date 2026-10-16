#!/usr/bin/env python3
"""Publish selected assembly evidence with the failed ARM host fence explicit."""
from pathlib import Path
import json,hashlib,argparse,os,subprocess,shutil
E=Path(__file__).resolve().parent;R=Path('/Users/boris/Dev/blockchain-nano')
BASE='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d';REV='68d64a5330d16c971dd603b56e0f14c88c1459c8'
DOCS=['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']
os.umask(0o077)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def put(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 with p.open('xb')as f:f.write(b)
def git(*args):return subprocess.check_output(['/usr/bin/git','--no-optional-locks','-C',str(R),*args],timeout=60)
def main():
 a=argparse.ArgumentParser();a.add_argument('--native',required=True);a.add_argument('--arm',required=True);a.add_argument('--amd',required=True);args=a.parse_args()
 assert git('status','--porcelain')==b'';head=git('rev-parse','HEAD').decode().strip()
 for tree in ('src','include','network','third_party','tests/capella','tests/support','tests/multihost'):assert git('rev-parse','HEAD:'+tree)==git('rev-parse',REV+':'+tree)
 assert git('diff','--name-only',BASE,REV,'--','include','network','third_party')==b''
 roots={k:Path(v).resolve()for k,v in [('native',args.native),('arm',args.arm),('amd',args.amd)]};reports={}
 for owner,root in roots.items():
  q=json.loads((root/'result-public.json').read_bytes());assert q['passed']is True and q['productionQualified']is False;reports[owner]=q
  if owner=='native':
   assert q['format']=='nano-current-native-runtime-relink-continuation-v1' and q['aggregateCompiledTranslationUnits']==1 and q['aggregateCurrentCoreLinks']==7 and q['aggregateSmokeCliCount']==8
   assert q['core']=={'sha256':'0808fa1320026d07c020ff1d1dfd3d4cd4a97ab2efa445e00caf8954cb0395b2','bytes':29263216}
  else:
   assert q['format']=='nano-current-linux-runtime-relink-qualification-v1' and q['compiledTranslationUnits']==0 and q['linkCommands']==(6 if owner=='arm' else 5) and q['smokeCommands']==(7 if owner=='arm' else 6)
   assert q['coreSha256']==('35779b74ff3894d8857d06732caa801cb340381f4cf6dc5801074c7e372dbdf6' if owner=='arm' else '1687f1876ea784006b0d687db0f94e3a49b01f8edaa3a599fdf942a9fc4d1525')
  fence_names=['final-fence-public.json'] if owner=='native' else ['originals-final-fence-public.json','selected-inputs-final-fence-public.json']
  fences=[root/name for name in fence_names if (root/name).is_file()];assert len(fences)==1
  fence=json.loads(fences[0].read_bytes());assert fence.get('passed')is True and fence.get('firstFailurePreserved')is False
  if owner!='native':
   cleanup=json.loads((root/'cleanup-public.json').read_bytes());assert any(cleanup.get(k)is True for k in ('ownContainersAbsent','ownedContainerAbsent','ownedContainersAbsent'))
 p2p=E/'unchanged-p2p-reuse-root-20261016-01/result-public.json';p2pbody=p2p.read_bytes();p2pvalue=json.loads(p2pbody);assert p2pvalue['passed']and not p2pvalue['productionQualified']and p2pvalue['programRuns']==0
 dest=R/'docs/qualification/runtime-core-relink-20261016';assert not dest.exists();dest.mkdir(mode=0o755)
 for owner,root in roots.items():
  put(dest/'reports'/owner/'result.json',(root/'result-public.json').read_bytes())
  for old,new in [('cleanup-public.json','cleanup.json'),('originals-final-fence-public.json','inputs-fence.json'),('selected-inputs-final-fence-public.json','inputs-fence.json'),('final-fence-public.json','inputs-fence.json')]:
   p=root/old
   if p.is_file():
    z=json.loads(p.read_bytes());assert z.get('passed',True)is True
    target=dest/'reports'/owner/new
    if not target.exists():put(target,p.read_bytes())
 put(dest/'reports/p2p-reuse.json',p2pbody)
 put(dest/'reports/arm/original-host-final-fence-failure.json',(roots['arm']/'original-host-final-fence-failure-public.json').read_bytes())
 put(dest/'reports/arm/preflight-failure.json',(E/'arm-runtime-relink-preflight-failure-20261016.json').read_bytes())
 put(dest/'reports/arm/metadata-first-admission-refusal.json',(E/'arm-runtime-relink-metadata-admission-refusal-20261016.json').read_bytes())
 put(dest/'reports/native/partial-failed-episode.json',(E/'native-runtime-relink-partial-episode-root-20261016.json').read_bytes())
 put(dest/'reports/native/first-M-failure.json',(E/'current-native-runtime-relink-root-20261016-01/consensus_capella_runtime-independent-m-exit.json').read_bytes())
 sources={
 'native-continuation':E/'current-native-runtime-relink-continuation-source-20261016',
 'native':E/'current-native-runtime-relink-source-20261016-v2',
 'native-failed01':E/'current-native-runtime-relink-source-20261016',
 'arm':E/'current-arm-runtime-relink-source-20261016-v2',
 'arm-failed01':E/'current-arm-runtime-relink-source-20261016',
 'amd':E/'current-amd-runtime-relink-source-20261016',
 'linux-worker':E/'current-linux-runtime-relink-worker-source-20261016'}
 for family,root in sources.items():
  assert root.is_dir()
  manifest=root/'manifest.json';raw=manifest.read_bytes();listed=json.loads(raw)['payloads'];assert isinstance(listed,list)
  put(dest/'source'/family/'manifest.json',raw)
  seen=set()
  for item in listed:
   leaf=item['path'];assert Path(leaf).name==leaf and leaf not in seen and '.private'not in leaf;seen.add(leaf)
   p=root/leaf;assert p.is_file() and not p.is_symlink();raw=p.read_bytes();assert pin(raw)=={k:item[k]for k in ('sha256','bytes')};put(dest/'source'/family/leaf,raw)
 for leaf in ['audit-unchanged-p2p-reuse-root-20261016.py','package-runtime-core-relink-root-20261016.py','dispatch-runtime-relink-amd-root-20261016.py','reconcile-arm-runtime-relink-root-20261016.py']:
  put(dest/'source/root'/leaf,(E/leaf).read_bytes())
 source_manifest={'format':'nano-current-runtime-core-source-join-v1','sourceRevision':REV,'observedRepositoryHeadBeforeDocumentation':head,'historicalCoreBase':BASE,'sourceTrees':{x:git('rev-parse',REV+':'+x).decode().strip()for x in ('src','include','network','third_party','tests/capella','tests/support','tests/multihost')},'coreSourceDelta':git('diff','--name-only',BASE,REV,'--','src','include').decode().splitlines(),'unchangedP2PSourceBase':BASE,'historicalSourceManifestD30eRetargeted':False,'old24AdmissionsRetargeted':False,'freshCampaignAdmissionIssued':False,'productionQualified':False}
 assert source_manifest['coreSourceDelta']==['src/consensus_capella_runtime.cpp'];put(dest/'source-join.json',(json.dumps(source_manifest,sort_keys=True,indent=2)+'\n').encode())
 assets=[]
 for family,owner in [('native','mac-native'),('arm','mac-docker'),('amd','vps-docker')]:
  report=reports[family];programs=report['programs']
  if isinstance(programs,dict):
   for role,value in programs.items():assets.append({'owner':owner,'role':role,**{k:value[k]for k in ('sha256','bytes')},'proof':'reports/'+family+'/result.json','effectiveCppSourceRevision':REV})
  else:
   for value in programs:assets.append({'owner':owner,'role':value['role'],'sha256':value['softwareSha256'],'bytes':value['softwareBytes'],'proof':'reports/'+family+'/result.json','effectiveCppSourceRevision':REV})
  core=report['core'] if family=='native' else {'sha256':report['coreSha256'],'bytes':report['coreBytes']}
  assets.append({'owner':owner,'role':'core',**core,'proof':'reports/'+family+'/result.json','effectiveCppSourceRevision':REV})
  value=p2pvalue['roles'][owner];assets.append({'owner':owner,'role':'p2p','sha256':value['programSha256'],'bytes':value['programBytes'],'proof':'reports/p2p-reuse.json','historicalSourceBase':BASE,'currentNetworkIncludeThirdPartyTreesUnchanged':True})
 assert len(assets)==24 and len({(p['owner'],p['role'])for p in assets})==24
 put(dest/'software-candidate-index.json',(json.dumps({'format':'nano-software-candidate-index-v1','sourceJoin':'source-join.json','assets':assets,'assetCount':24,'historical24AdmissionsRetargeted':False,'freshPrivateCampaignAdmission':False,'runtimeQualification':False,'evmRpcProfileQualifiedByThisBatch':False,'armOriginalHostFinalFenceFailureUnexplained':True,'completeHostQualification':False,'productionQualified':False},sort_keys=True,indent=2)+'\n').encode())
 readme='''# Assemblage au Core corrigé — 16 octobre 2026

Ce lot relie les programmes Nano et leurs outils au Core contenant le correctif de validation des paramètres de maintenance (source `68d64a5`). Les rapports décrivent les opérations réellement effectuées, les identités produites et les limites de chaque environnement.

Les objets consommateurs sont réutilisés après vérification des preuves de production conservées et comparaison des dépendances atteintes par une M indépendante. Le Core Native ne recompile que son objet runtime et vérifie le maintien des 142 autres membres parmi 143. Les deux Linux réutilisent les Core déjà qualifiés par le lot [pruning](../pruning-operation-20261016/README.md). Les exécutables sont reliés puis soumis aux seules vérifications CLI sans base ni réseau ; les Linux vérifient leurs chargements directs. Ces vérifications d'assemblage ne remplacent pas les tests de fonctionnement du runtime.

P2P ne dépend pas de `nano_core`. Ses trois binaires sont conservés après audit sélectionné de leurs identités et des arbres source inchangés ; aucune compilation, liaison ou exécution P2P n'est ajoutée. Les limites historiques de qualification du loader, des SDK et de CMake global sont conservées.

Le contrôleur hôte ARM échoue après les six liaisons, six loaders et sept CLI réussis, sur sa dernière barrière d'identités physiques. Cette cause reste inexpliquée : il n'est pas marqué réussi. Un rapprochement distinct retrouve les 1 226 entrées hôte sauvegardées avant le worker et les sept fichiers logiciels à leurs neuf attributs et SHA conservés. Les neuf fichiers des volumes Linux sont identiques avant/après le worker ; son résultat et son binding concordent par leurs octets et hashes. Deux nouvelles copies de ces métadonnées sont admises séparément avec au moins trois observations identiques pendant deux secondes, sans récupérer ni rafraîchir leurs anciennes identités. Une première tentative de stabilité de ces nouvelles copies est également refusée puis conservée ; le rapprochement ne recompile et ne rejoue aucun programme. La qualification entière de l'hôte ARM reste ouverte.

Les deux refus Native sont conservés : en-têtes manquants dans la première projection, puis comparaison littérale de chemins équivalents après cinq programmes réussis. La continuation ne relie que les deux outils restants ; elle vérifie la résolution des chemins normalisés et conserve l'absence d'identité brute récupérée pour leurs anciens MD. Le refus préliminaire ARM sur le mode du répertoire source est également conservé. Aucun refus du contrôleur n'est présenté comme un défaut C++ corrigé.

Aucune image Docker reconstruite, ancienne suite rejouée, clé créée, signature produite, base privée ouverte ou reprise de duties. Aucune finalité commune nouvelle n'est démontrée. Les anciens manifeste `d30e…`, admissions et échecs V37/campagne08 restent historiques. Ce lot n'émet pas les nouvelles admissions d'une campagne privée et n'autorise pas cette campagne.

La restauration complète, les duties et la finalité commune, le cycle des validateurs, les partitions/WAN/charge, le pruning prolongé, la CI complète, le bridge et les termes Base–NANO ainsi que la revue de sécurité indépendante restent ouverts. Aucun EIP, fork ou statut de production n'est promu. Goal actif.
'''
 put(dest/'README.md',readme.encode())
 payload={str(p.relative_to(dest)):pin(p.read_bytes())for p in sorted(dest.rglob('*'))if p.is_file()};mf={'format':'nano-runtime-core-relink-public-evidence-v1','sourceRevision':REV,'payloads':payload,'payloadCount':len(payload),'payloadBytes':sum(p['bytes']for p in payload.values()),'newCampaignQualified':False,'oldSourceManifestRetargeted':False,'productionQualified':False};put(dest/'MANIFEST.json',(json.dumps(mf,sort_keys=True,indent=2)+'\n').encode())
 prefix='''> **16 octobre 2026 — Programmes assemblés au Core corrigé.**
> [Rapport, sources et identités](qualification/runtime-core-relink-20261016/README.md). Nano et les outils sont reliés au Core courant sur Mac natif, Docker ARM et Docker AMD ; les rapports bornent les dépendances, réutilisations et vérifications CLI. Le P2P inchangé reste joint séparément à ses preuves conservées.
> Le worker ARM passe ses vérifications mais son contrôleur hôte échoue à la dernière barrière d'identités ; la cause reste inexpliquée et cette qualification complète reste ouverte. Un rapprochement séparé vérifie 1 226 entrées conservées et sept fichiers logiciels, sans rejeu ni rafraîchissement des anciennes identités.
> Aucune reprise réseau/duties ni finalité commune démontrée par ce lot ; aucune image reconstruite, suite antérieure rejouée ou ancienne admission retargetée. Campagne privée, consensus/WAN/charge, cycle des validateurs, opérations prolongées, bridge/lancement, CI et revue indépendante restent ouverts. Historique intégral conservé ; production non qualifiée, goal actif.

'''.encode()
 receipt={}
 for leaf in DOCS:
  p=R/'docs'/leaf;before=p.read_bytes();assert before==git('show',head+':docs/'+leaf);p.write_bytes(prefix+before);assert p.read_bytes()[len(prefix):]==before;receipt[leaf]={'old':pin(before),'new':pin(prefix+before),'oldBodyByteExact':True}
 out=E/'runtime-core-relink-four-document-receipt-20261016.json';put(out,(json.dumps({'headBeforeDocumentation':head,'files':receipt,'publicManifest':pin((dest/'MANIFEST.json').read_bytes())},sort_keys=True)+'\n').encode());print(json.dumps({'publicPayloads':len(payload),'fourOldDocumentBodiesByteExact':True,'productionQualified':False}),flush=True)
if __name__=='__main__':main()
