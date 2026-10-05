from pathlib import Path
import subprocess,json,tarfile,io,shlex,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-v7-pruning-eligibility-20261006');dest=repo/'docs/qualification/restored-v7-pruning-eligibility-20261006';assert not dest.exists();dest.mkdir()
ssh=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','ubuntu@145.239.199.166'];image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
for label,suffix,expected in [('first','',1),('fixed','-fixed',1),('fixed2','-fixed2',0)]:
 name='nano-v7-pruning-eligibility-build'+('' if not suffix else suffix)+'-05649987'
 state=json.loads(subprocess.check_output(ssh+[shlex.join(['sudo','docker','inspect','--format','{{json .State}}',name])],text=True));assert state['Status']=='exited' and state['ExitCode']==expected and not state['OOMKilled'],state
 (dest/(label+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
 folder='v7-pruning-eligibility-amd64-05649987'+suffix+'-20261006'
 cmd=['sudo','docker','run','--rm','--entrypoint','/bin/bash','-v','nano-clean-rpc-build-20261002:/build:ro',image,'-c','cd /build/'+folder+" && find . -type f \\( -name '*.txt' -o -name '*.log' -o -name '*.sha256' \\) -print0 | tar --null -T - -czf -"]
 data=subprocess.check_output(ssh+[shlex.join(cmd)])
 with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as tf:
  for m in tf.getmembers():
   assert m.isfile() and '..' not in Path(m.name).parts and not m.name.startswith('/')
   p=dest/label/m.name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(tf.extractfile(m).read())
 assert (dest/label/'controller-exit.txt').read_text().strip()==str(expected)
for p in root.iterdir():
 if p.is_file():shutil.copyfile(p,dest/p.name)
shutil.copytree(root/'run',dest/'run')
d=json.loads((dest/'run/capture.json').read_text());receipt=json.loads((repo/'docs/qualification/restored-three-node-v7-terminal-20261006/terminal-verification.json').read_text()) if False else None
assert d['blocks']==d['blockLimit']==256 and d['anchorAlreadyFinalized'] and d['descendantsOfFinalized']==256 and d['removableByFinalizedAncestry']==0 and not d['pruningApplied'] and not d['productionQualified']
assert d['restart']=={'sequence':'0x1c75','digest':'0x7f4d9e3fb24751b86883a3351adf1502e1c4fd8e691f158a920883087b550f35'}
assert json.loads((dest/'run/immutability.json').read_text())['hashesUnchanged']
readme='''# Ancre et éligibilité d’élagage du VPS v7 arrêté — 6 octobre 2026

Lecture Docker AMD64 hors réseau, bases montées en lecture seule. Engagement exact v7 7285/7f4d9e3f et témoin du cœur vérifiés ; 1393 fichiers de données identiques avant/après. Aucun signer rouvert ou réexaminé par cet outil, aucune suppression, aucune reprise de duties. Audits canoniques et de signers précédents conservés séparément.

Le Store retient 256 blocs sur la borne locale de 256. Son ancre égale la racine finalisée epoch 1284, slot 10271. Les 256 blocs descendent tous de cette ancre : zéro bloc supprimable par l’algorithme existant d’élagage sur frontière finalisée. L’âge de la finalité satisfait la garde min_seed_lookahead=1 (horloge epoch 1401), mais la frontière ne peut plus avancer faute de nouvelle finalité. Le graphe a dix points de branchement et douze feuilles ; il ne faut pas supprimer arbitrairement ces branches encore post-finalité.

SSZ des états : 7 956 128 octets sur 67 108 864. Snapshot sérialisé : 21 914 821 octets sur 134 217 728. Cela identifie la borne en nombre de blocs comme la première atteinte dans ce snapshot, sans estimer la RAM réelle ni garantir l’espace pour une transition future. Aucun vote signé conservé dans cet instantané. L’absence actuelle de votes ne qualifie ni le réseau ni son rattrapage.

Diagnostic structural fondé sur le snapshot local engagé : parse les en-têtes, vérifie les parents/slots et compte les descendants ainsi que les contextes de votes hors candidat. Il ne réexécute pas la transition de chaque bloc, ne prouve pas le fork choice historique et ne qualifie pas une opération d’élagage. La validation cryptographique/canonique précédente reste dans son périmètre propre.

Deux échecs de contrôleur/build conservés : ancien nom de source puis include manquant du décodeur SSZ. Version fixed2 compilée/liée sortie 0, pas d’OOM ; source distincte et logs des trois versions conservés. Lien à l’archive actuelle atomic-proposal AMD64 ; runtime b5ee00ac précédent intact, images réutilisées. Le diagnostic ne déploie pas le nouveau runtime.

Conséquence : une relance du prune existant ne libérera rien sur ce VPS. Il reste à fournir une rétention capable de récupérer le progrès sous pression, puis démontrer une finalité commune avec les nouveaux relais et cœur. Une expérience de profil de ressources plus large devrait rester distincte de la qualification à 256 blocs et mesurer les ressources ; augmenter la borne seule ne ferme pas ce défaut. v7 reste FAILED, portabilité physique et production ouvertes.

run/capture.json contient les seuls compteurs et racines exposés ; les bases restent sur le VPS. manifest.json couvre les pièces conservées, hors lui-même.
'''
(dest/'README.md').write_text(readme)
append='''\n## Éligibilité d’élagage du VPS v7 arrêté — 6 octobre 2026\n\nDiagnostic structural engagé : ancre déjà égale à la finalité epoch 1284,\n256/256 blocs tous descendants, zéro bloc supprimable par la frontière actuelle.\nSSZ 7 956 128/67 108 864 octets, snapshot 21 914 821/134 217 728 octets ;\nla borne en nombre de blocs est atteinte avant ces deux bornes d’octets.\n1393 fichiers inchangés ; lecture seule sans suppression, signer ni reprise.\nDeux échecs de build diagnostique conservés puis version corrigée sortie 0.\nCe résultat interdit de qualifier un simple prune comme récupération : rétention\nsous pression et nouvelle finalité doivent encore être démontrées. Pas d’estimation\nde RAM ni de qualification d’un profil agrandi. v7 et production restent ouverts.\n[Preuves et limites](qualification/restored-v7-pruning-eligibility-20261006/README.md).\n'''
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+append)
manifest={str(p.relative_to(dest)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dest.rglob('*')) if p.is_file()};(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Archived',len(manifest),'pieces; zero removable post-finalized blocks, 1393 immutable files')
