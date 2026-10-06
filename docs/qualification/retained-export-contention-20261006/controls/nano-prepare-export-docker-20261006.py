from pathlib import Path
import shutil,subprocess,re,json,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano'); root=Path('/private/tmp/nano-export-contention-docker-20261006'); root.mkdir(exist_ok=True); src=root/'source';src.mkdir(exist_ok=True)
for name in ['include','src','network','third_party','tests/support']:shutil.copytree(repo/name,src/name,dirs_exist_ok=True)
shutil.copy2(repo/'tests/test_capella_transition.cpp',src/'tests/test_capella_transition.cpp')
names=set()
for p in [repo/'tests/test_capella_transition.cpp',*list((repo/'tests/support').glob('*.hpp'))]:names.update(re.findall(r'consensus-[a-z0-9-]+-official',p.read_text()))
for name in names:shutil.copytree(repo/'tests/fixtures'/name,src/'tests/fixtures'/name,dirs_exist_ok=True)
(root/'source-manifest.json').write_text(json.dumps({str(p.relative_to(src)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(src.rglob('*')) if p.is_file()},indent=2)+'\n')
subprocess.run(['tar','-czf',str(root/'source.tar.gz'),'-C',str(root),'source'],check=True)
print(root);print('fixture dirs',sorted(names));print('archive bytes',(root/'source.tar.gz').stat().st_size)
