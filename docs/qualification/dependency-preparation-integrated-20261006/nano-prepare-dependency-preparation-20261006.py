from pathlib import Path
import shutil,json,hashlib,tarfile
repo=Path('/Users/boris/Dev/blockchain-nano');base=Path('/private/tmp');root=base/'nano-dependency-preparation-docker-20261006';root.mkdir(exist_ok=False);source=root/'source';parent=base/'nano-network-status-coherent-20261006/source';shutil.copytree(parent,source)
for directory in ['src','include']:
 for p in (source/directory).rglob('*'):
  if p.is_file():assert p.read_bytes()==(repo/p.relative_to(source)).read_bytes(),str(p)
shutil.rmtree(source/'network');shutil.copytree(repo/'network',source/'network')
for rel in ['tests/test_capella_transition.cpp','tests/capella/paused_capture.cpp']:assert (source/rel).read_bytes()==(repo/rel).read_bytes()
manifest={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source.rglob('*')) if p.is_file()};(root/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(root/'source.sha256').write_text(''.join(h+'  '+n+'\n' for n,h in manifest.items()))
with tarfile.open(root/'source.tar','w') as t:t.add(source,arcname='source')
old=base/'nano-delivery-diagnostic-docker-20261006'
s=(old/'sdk.sh').read_text().replace('sdk=/build/catalog-selection-$arch-20261006/network','sdk=/build/dependency-preparation-sdk-$arch-20261006/network').replace('nano-capella-catalog-selection-tests nano-capella-block-delivery-tests','nano-capella-dependency-preparation-tests nano-capella-catalog-selection-tests nano-capella-block-delivery-tests').replace('capella_(catalog_selection|','capella_(dependency_preparation_.*|catalog_selection|');(root/'sdk.sh').write_text(s)
s=(old/'regression.sh').read_text().replace('out=/build/delivery-diagnostic-$arch-20261006','out=/build/dependency-preparation-$arch-20261006').replace('core=/build/catalog-selection-$arch-20261006/libnano_core.a','core=/build/network-status-coherent-$arch-20261006/libnano_core.a');(root/'regression.sh').write_text(s)
(root/'all.sh').write_text((old/'all.sh').read_text())
s=(base/'nano-launch-delivery-diagnostic-20261006.py').read_text().replace('nano-delivery-diagnostic-docker-20261006','nano-dependency-preparation-docker-20261006').replace('nano-delivery-diagnostic-','nano-dependency-preparation-');(base/'nano-launch-dependency-preparation-20261006.py').write_text(s)
print('Frozen',len(manifest),'sources; current core source unchanged, images reused, separate SDK output keeps previous programs intact')
