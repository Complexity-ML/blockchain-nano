from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');tmp=Path('/private/tmp');root=tmp/'nano-network-status-coherent-20261006';native=tmp/'nano-network-status-coherent-native-20261006';out=repo/'docs/qualification/network-status-coherent-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
m=read(root/'source-manifest.json');assert all(sha(repo/n)==h for n,h in m.items())
assert read(root/'native-terminal-exit.json')['resumeExit']==0
for p in native.glob('*-exit.json'):assert read(p)['exitCode']==0,p
for line in (native/'programs.sha256').read_text().splitlines():
 h,n=line.split();assert sha(native/n)==h,n
assert len(list(native.glob('*.cpp.d')))==136 and len(read(native/'rebuilt-objects.json'))==18
for arch in ['arm64','amd64']:
 assert read(root/(arch+'-terminal-exit.json'))['exitCode']==0
 p=root/(arch+'-proof')
 for n in ['controller-exit.txt','runtime-controller-exit.txt','regression-controller-exit.txt']:assert (p/n).read_text().strip()=='0'
 assert len(list((p/'dependencies').glob('*.d')))==136 and len((p/'rebuilt-sources.txt').read_text().splitlines())==18 and len((p/'reused-sources.txt').read_text().splitlines())==118
 for mode in ['beacon-reads','configured-runtime','automatic-recovery','http','wire','network-status-identity','network-status-sync']:assert (p/('test-'+mode+'-exit.txt')).read_text().strip()=='0'
 for phase in ['before','after']:
  s=(root/(arch+'-source-'+phase+'.log')).read_text();assert s.count(': OK')==len(m) and 'FAILED' not in s
for label,p in [('native',native),('arm64',root/'arm64-proof'),('amd64',root/'amd64-proof')]:
 for field in ['identity','sync']:
  name=('network-status-'+field+'.log') if label=='native' else ('test-network-status-'+field+'.log');rows=[json.loads(x) for x in (p/name).read_text().splitlines() if x.startswith('{')];probes=[x for x in rows if x.get('scenario')=='network-status-during-held-admission'];assert probes and all(x['completedWhileAdmissionHeld'] and x['sameStatus'] and x['cancelledWriterUnpublished'] for x in probes)
out.mkdir(exist_ok=False)
def proofs(p,d):
 d.mkdir(parents=True,exist_ok=True)
 for f in p.rglob('*'):
  if f.is_file() and f.suffix in ['.json','.log','.txt','.sha256','.d','.exit','.hpp','.cpp']:
   q=d/f.relative_to(p);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,q)
proofs(native,out/'native')
for arch in ['arm64','amd64']:proofs(root/(arch+'-proof'),out/arch)
shutil.copytree(root/'source',out/'source')
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sha256','.sh']:shutil.copy2(f,out/f.name)
for n in ['nano-fix-network-status-lock-20261006.py','nano-build-network-status-coherent-native-20261006.py','nano-resume-network-status-coherent-native-20261006.py','nano-launch-network-status-coherent-20261006.py','nano-collect-network-status-coherent-20261006.py','nano-archive-network-status-coherent-20261006.py']:shutil.copy2(tmp/n,out/n)
(out/'README.md').write_text('''# Lectures de statut sous admission — correctif cohérent

Parent main 3bd8c8aa, cœur d8d5b46c ; les deux contre-exemples natifs contre l'ancien cœur (identity et sync) et l'échec WAN v21 sont conservés dans restored-three-node-v21-startup-terminal-20261006. Le défaut local est reproductible : un writer retenu sous le verrou du réacteur empêchait les deux getters de répondre, même sans changement du statut ou de la publication.

network_identity lit maintenant la publication immuable authentifiée. fail_closed et la destruction continuent de vider cette publication ; le fallback du reader conserve le refus et le diagnostic latched. sync_admission_stats utilise un verrou de télémétrie distinct ; toutes les mutations des compteurs saturants utilisent ce même verrou. Les mutateurs prennent d'abord le verrou du réacteur, puis celui des compteurs ; le reader ne prend jamais le verrou du réacteur sous celui des compteurs. Aucun changement de quorum, validation, signing, persistences, fencing, limites ou délais. Aucun format de stockage modifié.

Le membre mutex supplémentaire change le layout C++ : dans chacun des trois environnements, 136 unités cœur rescannées, 18 consommateurs recompilés, 118 objets indépendants réutilisés depuis l'archive qualifiée. nano, le reader arrêté et la fixture sont recompilés et liés contre cette archive cohérente. Programmes parents conservés, images/volumes réutilisés. P2P e7b41646 réutilisable dans son périmètre : aucun fichier network modifié par ce lot. 348 sources figées vérifiées avant/après Docker et identiques au worktree à l'archivage.

Natif original 77990 : cœur et runtime compilés, fixture échouée par deux chemins physiques du même header réseau ; erreur conservée. Reprise 78144 ne recompile pas le cœur/runtime et utilise les headers de support copiés avec un chemin réseau unique. Sortie 0. Docker ARM64 46395 et AMD64 11667, sorties 0. Nouvelles régressions identity/sync : réponses pendant admission retenue, mêmes données, writer annulé sans publication, sur les trois environnements. Deux tests CTest enregistrés, mêmes commandes/variables que les exécutions ciblées ; cela ne prétend pas être une campagne CI complète.

Régression pertinente : Beacon snapshots/exports concurrents, runtime configuré, reprise automatique avec neuf contrôles d'engagements, HTTP et vraie fixture --wire (neuf blocs, dix-sept messages, deux reprises SIGKILL, dix-huit annulations). finalityScenario=false et networkActivated=false. Release sans instrumentation, reprise WAN, finalité commune et toutes autres exigences/revue indépendante restent ouvertes. La causalité exacte WAN du défaut local ne sera qualifiée qu'après reprise des nœuds.
''')
note='\n\n2026-10-06 — Lectures de statut : défaut local identity/sync reproduit contre ancien cœur, correctif qualifié natif/ARM64/AMD64 (78144/46395/11667 sorties 0). Publication authentifiée pour identité et verrou distinct pour compteurs ; aucun quorum, délai ou fencing modifié. ABI cohérente : 136 unités scannées, 18 consommateurs recompilés par architecture, programmes parents conservés. Deux régressions de lecture sous admission retenue passent, ainsi que Beacon/reprise/HTTP/wire ciblés. Échec initial de fixture conservé. [Preuves](qualification/network-status-coherent-20261006/README.md). Effet sur WAN/finalité, release sans instrumentation et autres critères/revue indépendante non qualifiés.\n'
for n in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/n;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('Coherent status correction archived; production remains unqualified')
