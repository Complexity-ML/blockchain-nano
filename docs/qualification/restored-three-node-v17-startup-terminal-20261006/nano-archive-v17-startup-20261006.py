from pathlib import Path
import json,shutil,hashlib,subprocess
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-restored-three-node-v17-catalog-selection-20261006/campaign');dest=repo/'docs/qualification/restored-three-node-v17-startup-terminal-20261006';assert not dest.exists();dest.mkdir()
assert json.loads((root/'terminal-exit.json').read_text())=={'originalSession':92712,'exitCode':1,'failureBeforeDatabaseOpen':True}
assert json.loads((root/'result.json').read_text())['result']=='FAILED';assert 'cannot open /config/genesis.json' in (root/'vps.log').read_text()
for name in ['artifacts.json','controller-hashes.json','events.jsonl','plan.json','result.json','vps-local-preparation-proof.json','vps.log','terminal-exit.json']:shutil.copy2(root/name,dest/name)
s=json.loads((root/'vps.log.state.json').read_text());assert not s['State']['Running'] and s['State']['ExitCode']==1 and not s['State']['OOMKilled'];(dest/'vps-stopped-state.json').write_text(json.dumps({k:s[k] for k in ['Name','Image','State']},indent=2)+'\n')
p=Path('/private/tmp/nano-v17-startup-vps-immutability-20261006.json');audit=json.loads(p.read_text());assert audit['files']==1926 and audit['hashesUnchanged'] and audit['noWriter'];shutil.copy2(p,dest/'vps-immutability.json')
proof=Path('/private/tmp/nano-v17-local-permission-proof-20261006');assert json.loads((proof/'container-read-exit.json').read_text())['exitCode']==0;shutil.copytree(proof,dest/'permission-correction')
for name in ['nano-qualify-vps-local-permissions-20261006.py','nano-archive-v17-startup-20261006.py']:shutil.copy2(Path('/private/tmp')/name,dest/name)
for file in ['tests/capella/restored-three-node-run.mjs','tests/capella/vps-local-launch-prepare.py']:
 data=subprocess.check_output(['git','show','2e8d9c01790feaf077afdab490d4fa711a330873:'+file],cwd=repo);(dest/Path(file).name).write_bytes(data)
shutil.copy2(repo/'tests/capella/vps-local-launch-prepare.py',dest/'permission-correction/vps-local-launch-prepare-corrected.py')
(dest/'README.md').write_text("""# Reprise v17 — échec de permissions avant ouverture du stockage

Session originale 92712, sortie terminale 1, checkout 2e8d9c01. Préflight réussi et configuration construite exclusivement sur le VPS, liée par empreinte au plan indépendant. Le conteneur n'a pas pu lire `/config/genesis.json` : répertoire créé en mode 0700 et utilisateur du conteneur différent. Aucun nœud Mac lancé, aucun reçu d'arrêt de consensus, aucune conclusion sur le critère de finalité en 900 s.

Conteneur VPS arrêté exit 1 sans OOM. Les 1926 fichiers du volume sont identiques à l'audit post-v16 ; aucun writer. Engagements et journaux inchangés. L'état Docker archivé exclut environnement et clés.

Correction du préparateur : répertoire 0755, fichier d'environnement des clés maintenu 0600. Préparation exclusivement locale sur le VPS ; l'image réelle lit genesis/profil/état initial/configuration, sans monter la base ni lancer de signer. Les sorties et empreintes sont conservées. Ce contrôle ne qualifie pas une reprise complète : une nouvelle tentative contrôlée est encore nécessaire.

Aucune recompilation C++, reconstruction d'image, export de configuration ou clé depuis Mac. Production et toutes les exigences restantes restent ouvertes.
""")
manifest={str(f.relative_to(dest)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(dest.rglob('*')) if f.is_file()};(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
paragraph='\n\n2026-10-06 — V17 démarrage FAILED, session 92712/sortie 1 : répertoire VPS local 0700 inaccessible à l’utilisateur du conteneur, avant ouverture de genesis/base. Aucun nœud Mac lancé ni reçu de consensus ; pas un échec du délai de finalité. Volume VPS 1926 fichiers vérifiés inchangés. Correction répertoire 0755/env clés 0600 qualifiée avec image réelle, sans volume de chaîne/signature. Configuration dérivée localement et liée au plan par empreinte ; aucune exportation depuis Mac. Nouvelle tentative requise, finalité et production ouvertes. Preuves restored-three-node-v17-startup-terminal-20261006/.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+paragraph)
print(len(manifest),'proof files; v17 startup failure retained; storage immutable')
