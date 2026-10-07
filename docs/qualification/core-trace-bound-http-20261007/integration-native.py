from pathlib import Path
import subprocess,json,shlex,hashlib,shutil
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');old=Path('/private/tmp/nano-catalog-busy-integrated-native-20261006/network');out=r/'native-integration';out.mkdir();staged=out/'source/network';shutil.copytree(repo/'network',staged)
m=json.loads((r/'manifest.json').read_text())
for rel,h in m['files'].items():
 assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256'];shutil.copyfile(r/'source'/rel,out/'source'/rel)
shutil.copyfile(r/'source/network/test_core_trace_bound_http.cpp',staged/'test_core_trace_bound_http.cpp')
parent=Path('/private/tmp/nano-catchup-priority-build-20261007/native/network/nano-p2p');before=hashlib.sha256(parent.read_bytes()).hexdigest()
def run(name,args):
 (out/(name+'-command.json')).write_text(json.dumps({'args':args,'cwd':str(old)})+'\n')
 with (out/(name+'.log')).open('w') as f:x=subprocess.run(args,cwd=old,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':x.returncode})+'\n');assert x.returncode==0,(name,x.returncode);print(name+' exit0',flush=True)
def build(name,target,source,execute):
 d=old/'CMakeFiles'/(target+'.dir');rows=(d/'flags.make').read_text().splitlines();flags=[]
 for key in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']:flags+=shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(key+' = ')))
 obj=out/(name+'.o');binary=out/name
 run(name+'-compile',['/usr/bin/clang++','-I'+str(staged),*flags,'-c',str(staged/source),'-o',str(obj)])
 link=shlex.split((d/'link.txt').read_text());pos=[i for i,v in enumerate(link) if v.endswith('.cpp.o')];assert len(pos)==1;link[pos[0]]=str(obj);link[link.index('-o')+1]=str(binary);run(name+'-link',link)
 if execute:run(name,[str(binary)])
build('trace-http','nano-core-keepalive-tests','test_core_trace_bound_http.cpp',True)
build('keepalive-http','nano-core-keepalive-tests','test_core_keepalive.cpp',True)
build('catalog-busy-http','nano-capella-catalog-busy-http-tests','test_capella_catalog_busy_http.cpp',True)
build('nano-p2p','nano-p2p','main.cpp',False)
assert hashlib.sha256(parent.read_bytes()).hexdigest()==before
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256']
(out/'binding.json').write_text(json.dumps({'parentSHA256':before,'parentUnchanged':True,'draftFiles':m['files'],'mainBuilt':True,'realHttpQualified':True,'networkQualified':False},indent=2)+'\n')
