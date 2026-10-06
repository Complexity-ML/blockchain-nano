from pathlib import Path
import shlex,subprocess,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');old=Path('/private/tmp/nano-catalog-busy-integrated-native-20261006/network');base=old/'CMakeFiles/nano-p2p.dir';out=Path(__file__).parent/'native';out.mkdir()
rows=(base/'flags.make').read_text().splitlines();flags=[]
for key in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']:flags+=shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(key+' = ')))
files=[repo/'network'/n for n in ['main.cpp','capella_relay_schedule.hpp','capella_block_delivery.hpp','capella_head_catalog_relay.hpp']];before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
subprocess.run(['/usr/bin/clang++',*flags,'-c',str(repo/'network/main.cpp'),'-o',str(out/'main.cpp.o')],check=True)
link=shlex.split((base/'link.txt').read_text());positions=[i for i,v in enumerate(link) if v.endswith('main.cpp.o')];assert len(positions)==1;link[positions[0]]=str(out/'main.cpp.o');link[link.index('-o')+1]=str(out/'nano-p2p');subprocess.run(link,cwd=old,check=True)
assert before=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
for p in [out/'nano-p2p',*files]:print(hashlib.sha256(p.read_bytes()).hexdigest(),p,flush=True)
