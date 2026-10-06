from pathlib import Path
import shutil,json,hashlib,subprocess
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-catalog-selection-docker-20261006');root.mkdir(exist_ok=False);source=root/'source';source.mkdir()
for name in ['src','include','network']:shutil.copytree(repo/name,source/name)
for name in ['CMakeLists.txt','tests/test_capella_transition.cpp','tests/capella/paused_capture.cpp']:
 p=source/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(repo/name,p)
manifest={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source.rglob('*')) if p.is_file()};(root/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
old=Path('/private/tmp/nano-admission-continuations-docker-20261006')
for name in ['build.sh','runtime.sh','regression.sh','sdk.sh','all.sh']:
 s=(old/name).read_text().replace('out=/build/admission-continuations-$arch-20261006','out=/build/catalog-selection-$arch-20261006').replace('parent=/build/parent-cache-$arch-20261006','parent=/build/admission-continuations-$arch-20261006')
 if name=='build.sh':s=s.replace("capella_consensus_service.hpp'","capella_consensus_service.hpp|simulation_queue.hpp|capella_inventory_sessions.hpp'")
 if name=='regression.sh':s=s.replace('incoming-parent-interference incoming-alternation vote-target-alternation prepared-block wire configured-runtime automatic-recovery http','configured-runtime automatic-recovery http wire')
 if name=='sdk.sh':
  s=s.replace('nano-altair-worker-tests nano-altair-worker-continuation-tests nano-altair-core-dispatch-tests nano-altair-core-recovery-tests nano-p2p','nano-capella-catalog-selection-tests nano-capella-catalog-relay-tests nano-capella-head-catalog-tests nano-capella-catalog-termination-tests nano-capella-head-history-fairness-tests nano-p2p')
  s=s.replace("'^altair_(worker|worker_continuations|core_dispatch|core_recovery)$'","'^capella_(catalog_selection|catalog_relay|head_catalog|catalog_termination|head_history_fairness)$'")
 if name=='all.sh':s=s.replace('bash /drivers/build.sh', '(cd /overlay/source; sha256sum -c /drivers/source.sha256)\nbash /drivers/build.sh');s+='\n(cd /overlay/source; sha256sum -c /drivers/source.sha256)\n'
 (root/name).write_text(s)
(root/'source.sha256').write_text(''.join(f'{h}  {p}\n' for p,h in manifest.items()))
subprocess.run(['tar','-cf',str(root/'source.tar'),'-C',str(root),'source'],check=True)
subprocess.run(['bash','-n',*[str(root/n) for n in ['build.sh','runtime.sh','regression.sh','sdk.sh','all.sh']]],check=True)
print(len(manifest),'frozen source files; public source only; no database, config or keys')
