from pathlib import Path
import hashlib,json,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');raw=Path('/private/tmp/nano-atomic-proposal-evidence-20261006');native=Path('/private/tmp/nano-atomic-proposal-native-20261006');dest=repo/'docs/qualification/capella-atomic-proposal-admission-20261006'
assert not dest.exists()
source_names=['src/consensus_capella_signing.cpp','src/consensus_capella_reactor.cpp','src/consensus_capella_validator.cpp','include/nano/consensus_capella_reactor.hpp','tests/test_capella_transition.cpp']
sources={p:hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in source_names}
for arch in ['arm64','amd64']:
 folder=raw/arch/f'atomic-proposal-{arch}-d71f574a-20261006'
 for label,expected in [('controller',0),('proposal-gap',1),('atomic-proposal',0),('capacity-pressure',0),('scheduler',0),('scheduler-registration',0)]:assert (folder/(label+'-exit.txt')).read_text().strip()==str(expected),(arch,label)
 for line in (folder/'sources.sha256').read_text().splitlines():
  sha,path=line.split(maxsplit=1);assert path.startswith('/work/') and sources[path[6:]]==sha,(arch,path)
 assert 'two-step proposal lost admission capacity' in (folder/'proposal-gap.log').read_text()
 row=json.loads((folder/'atomic-proposal.log').read_text().splitlines()[-1]);assert row['providerCalls']==2 and row['contenderRefusedAtCapacity'] and row['localDurablyAdmitted'] and row['duplicateAdmitted'] and row['witnessRestored'] and not row['networkQualified']
 row=json.loads((folder/'capacity-pressure.log').read_text().splitlines()[-1]);assert row['providerCalls']==0 and row['journalUnchanged'] and row['witnessRestored']
 for name in ['scheduler','scheduler-registration']:
  row=json.loads((folder/(name+'.log')).read_text().splitlines()[-1]);assert row['blocks']==40 and row['votes']==272 and row['syncMessages']==1280 and row['finalizedEpoch']==3 and row['witnessRestart'] and not row['networkTransport']
assert json.loads((native/'sources.json').read_text())==sources
for label in ['controller','atomic-proposal','capacity-pressure','nano-link','nano-capella-paused-capture-link','nano-help']:assert (native/(label+'-exit.txt')).read_text().strip()=='0',label
assert json.loads((native/'parents-before.json').read_text())==json.loads((native/'parents-unchanged.json').read_text())
for name,sha in json.loads((native/'programs.json').read_text()).items():assert hashlib.sha256((native/name).read_bytes()).hexdigest()==sha,name
shutil.copytree(raw,dest)
for p in native.iterdir():
 if p.suffix in ['.txt','.json','.log']:
  out=dest/'native'/p.name;out.parent.mkdir(exist_ok=True);shutil.copyfile(p,out)
controls=dest/'controls';controls.mkdir()
for name in ['nano-atomic-proposal-build-20261006.sh','nano-atomic-runtime-link-20261006.sh','nano-atomic-proposal-native-build-20261006.py','nano-atomic-proposal-collect-20261006.py','nano-atomic-proposal-archive-20261006.py']:shutil.copyfile(Path('/private/tmp')/name,controls/name)
summary={'baseCommit':'d71f574a23c7c2447bdd1d1aa1d61c1363439031','sources':sources,'qualification':'local atomic scheduler proposal/signing/admission and full-block refusal','twoStepReproduction':'same candidate, retained separate signing/admission API, forced concurrent order; not an old executable before/after test','twoStepExit':1,'atomicExit':0,'architectures':['mac-native','docker-arm64','docker-amd64'],'schedulerArchitectures':['docker-arm64','docker-amd64'],'runtimeLinkedArchitectures':['mac-native','docker-arm64','docker-amd64'],'runtimeMainAndCaptureObjectsReusedFrom':'b5ee00ac unchanged source files verified by git diff','freshCMakeConfigureQualified':False,'fullReproducibleBuildQualified':False,'capacityRecoveryQualified':False,'wanFinalityQualified':False,'deployed':False,'productionQualified':False}
(dest/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
readme='''# Proposition et admission atomiques du scheduler Capella — 6 octobre 2026

La boucle du validateur appelle désormais propose_and_admit_current. Le verrou du propriétaire couvre le RANDAO, la construction, la réservation/signature dans le journal et l’admission ordinaire jusqu’à la publication durable. Une absence de capacité est refusée avant fournisseur ; un wire n’est rendu au scheduler qu’après admission. Les erreurs réelles de validation/I/O restent fatales selon les chemins existants, sans effacer de réservation. Les API explicites séparées restent disponibles pour les outils ; elles ne garantissent pas cette atomicité.

La reproduction utilise une seule place libre, deux enfants distincts signés correctement et un vrai thread concurrent. Le fournisseur lance le thread, dont le signal est placé immédiatement avant son appel block ; ce marqueur ne mesure pas l’instant d’attente au mutex. Avec les API séparées et l’ordre imposé (attendre l’admission concurrente avant l’admission locale), le bloc local perd sa place : sortie 1 conservée. C’est une reproduction sur la même candidate utilisant le protocole d’appel ancien, pas un ancien exécutable requalifié. Avec l’API atomique, le bloc local est publié une fois et le concurrent est refusé à capacité ; le doublon local est accepté. Fermeture/restauration avec témoin conserve token, tip et engagement du journal. Tests locaux réussis sur Mac natif, Docker ARM64 et Docker AMD64.

La saturation couvre aussi l’API atomique : zéro appel fournisseur, journal intact, trois refus entrants sans écritures, horloge active et restauration exacte. Le scheduler modifié passe sur les deux architectures Docker : 40 blocs, 272 attestations, 1280 messages sync, finalité epoch 3 et 64 journaux rouverts. Le mode scheduler-registration vérifie en plus trois index périmés et trois journaux incohérents refusés avant fournisseur. Ce n’est pas un réseau WAN, une rotation complète de validateurs ou une reprise de finalité sous saturation.

Archives de test isolées : trois unités du cœur recompilées et remplacement de celles-ci dans la copie de l’archive précédemment vérifiée. Le header ajoute uniquement des méthodes, sans changement de champs. Les objets main et paused_capture inchangés depuis b5ee00ac sont réutilisés pour les nouveaux exécutables runtime et audit. Les deux fichiers sources inchangés ont été vérifiés par git diff ; nano --help sort 0 sur les trois plateformes. Les images, archives et anciens binaires sont conservés. Pas de reconstruction d’image, de configuration CMake fraîche ou de build intégral reproductible revendiqué.

La course de capacité est qualifiée sur le chemin atomique du scheduler dans ce scénario précis. Autres budgets, pannes d’I/O en ce point, latence sous contention, libération sûre de capacité, pruning/drainage et reprise WAN restent ouverts. v7 reste FAILED. Aucun des nouveaux runtime n’a été déployé sur le réseau ; aucun journal/témoin de v7 n’a été fusionné ou réinitialisé, aucun stockage transféré entre hôtes. Revue indépendante et critères de production restent ouverts.

summary.json relie les hashes des sources aux reçus ; programs.sha256/programs.json identifient les exécutables. Les états Docker sont limités à .State. manifest.json couvre toutes les pièces sauf lui-même.
'''
(dest/'README.md').write_text(readme)
append='''\n## Signature et admission atomiques du scheduler — 6 octobre 2026\n\nChemin atomique implémenté : verrou du propriétaire conservé entre signature\net admission durable, sans court-circuiter la validation ni supprimer de journal.\nReproduction du protocole séparé sortie 1 conservée ; admission concurrente\ncontrôlée, doublon et restauration réussis sur Mac natif et Docker ARM64/AMD64.\nRefus à pleine capacité avant fournisseur confirmé sur les trois plateformes.\nScheduler et gardes d’inscription passent sur les deux Docker (40 blocs,\n272 votes, 1280 sync). Nouveaux runtime/audits liés et identifiés, pas déployés.\nLa course est qualifiée dans ce périmètre ; libération de capacité, autres budgets,\nI/O à ce point, contention et finalité WAN restent ouverts. Aucun rebuild d’image ;\nCMake frais et build intégral reproductible non qualifiés. v7 reste FAILED.\n[Preuves et limites](qualification/capella-atomic-proposal-admission-20261006/README.md).\n'''
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+append)
manifest={str(p.relative_to(dest)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dest.rglob('*')) if p.is_file()}
(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Archived',len(manifest),'pieces; source, terminal, program and scope checks passed')
