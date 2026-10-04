import pathlib,subprocess,json,hashlib,shlex,os,sys
ARCH=sys.argv[1];assert ARCH in ['arm64','amd64'];ROOT=pathlib.Path(__file__).resolve().parent;REV='092a153ca1d203434beb14fe2102e3a092dc454c';SOURCE=pathlib.Path('/tmp/nano-pruning-wan-source-20261004' if ARCH=='arm64' else '/home/ubuntu/nano-pruning-wan-source-20261004');IMAGE={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}[ARCH];VOLUME='nano-clean-arm64-build-20261002' if ARCH=='arm64' else 'nano-clean-rpc-build-20261002';BASE='/build/finalized-retention-4734f285-'+ARCH;OUT='/build/pruning-wan-092a153c-'+ARCH;OWNER=str(os.getuid())+':'+str(os.getgid());PREFIX=['docker'] if ARCH=='arm64' else ['sudo','-n','docker'];NAME='nano-pruning-wan-delivery-'+ARCH+'-20261004'
def save(n,v):(ROOT/n).write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def call(args,timeout=60):
 p=subprocess.run(args,text=True,capture_output=True,timeout=timeout)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr[-1000:]))
 return p.stdout.strip()
def docker(args):return call(PREFIX+args)
def read_base(path):return docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--mount',f'type=volume,src={VOLUME},dst=/build,readonly',IMAGE,'cat',BASE+'/'+path])
def manifest():
 names=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0');return {n.decode():hashlib.sha256((SOURCE/n.decode()).read_bytes()).hexdigest() for n in names if n}
def baseline():return docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--mount',f'type=volume,src={VOLUME},dst=/build,readonly',IMAGE,'sha256sum',BASE+'/libnano_core.a',BASE+'/nano-capella-transition-tests',BASE+'/nano-execution-checkpoint-tests','/build/coherent-efbeb9bc-'+ARCH+'/network/nano-p2p'])
try:
 assert call(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV;assert not call(['git','-C',str(SOURCE),'status','--porcelain'])
 call(['git','-C',str(SOURCE),'diff','--exit-code','4734f285',REV,'--','src','include','network','third_party','cmake']);assert not docker(['ps','-q']),'No overlapping Docker job'
 docker(['volume','inspect',VOLUME]);assert docker(['image','inspect',IMAGE,'--format','{{.Id}}'])==IMAGE
 before=manifest();save('sources-before.json',before);old=baseline();(ROOT/'baseline-before.txt').write_text(old+'\n')
 flags=read_base('CMakeFiles/nano.dir/flags.make');text=read_base('CMakeFiles/nano.dir/link.txt');(ROOT/'flags.make').write_text(flags+'\n');(ROOT/'link.txt').write_text(text+'\n');v={l.split(' = ',1)[0]:l.split(' = ',1)[1] for l in flags.splitlines() if ' = ' in l};obj=OUT+'/main.cpp.o';binary=OUT+'/nano';compile=['clang++']+shlex.split(v['CXX_DEFINES'])+shlex.split(v['CXX_INCLUDES'])+shlex.split(v['CXX_FLAGS'])+['-c','/work/src/main.cpp','-o',obj];link=shlex.split(text);oldobj='CMakeFiles/nano.dir/src/main.cpp.o';assert oldobj in link;link[link.index(oldobj)]=obj;link[link.index('-o')+1]=binary;save('commands.json',{'compile':compile,'link':link,'runtimeSourceIdenticalTo':'4734f285','qualifiedCoreReused':BASE+'/libnano_core.a','networkCopiedFrom':'/build/coherent-efbeb9bc-'+ARCH+'/network/nano-p2p'})
 docker(['run','--rm','--network','none','--read-only','--user','0:0','--cap-drop','ALL','--cap-add','CHOWN','--mount',f'type=volume,src={VOLUME},dst=/build',IMAGE,'sh','-c','set -eu; test ! -e "$1"; mkdir "$1"; chown "$2" "$1"','sh',OUT,OWNER])
 script='set -u\ncd '+BASE+'\n'
 for stage,args in [('compile',compile),('link',link)]:script+=shlex.join(args)+' > /report/'+stage+'.log 2>&1\nstatus=$?\nprintf "%s\\n" "$status" > /report/'+stage+'-exit.txt\nif [ "$status" -ne 0 ]; then exit "$status"; fi\n'
 script+='set -e\nmkdir '+OUT+'/network\ncp /build/coherent-efbeb9bc-'+ARCH+'/network/nano-p2p '+OUT+'/network/nano-p2p\nprintf "%s\\n" '+REV+' > '+OUT+'/production-source-revision.txt\nsha256sum '+OUT+'/nano '+OUT+'/network/nano-p2p > /report/programs.txt\n';(ROOT/'run.sh').write_text(script);save('state.json',{'phase':'building-entrypoint','revision':REV,'arch':ARCH,'imageBuild':False,'productionQualified':False})
 args=PREFIX+['run','--rm','--name',NAME,'--user',OWNER,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount',f'type=volume,src={VOLUME},dst=/build','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={ROOT},dst=/report',IMAGE,'bash','/report/run.sh']
 with (ROOT/'controller.log').open('w') as log:p=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,timeout=1200)
 (ROOT/'controller-exit.txt').write_text(str(p.returncode)+'\n')
 if p.returncode:raise RuntimeError('Entry point build failed; keep logs')
 after=manifest();save('sources-after.json',after);assert before==after;old_after=baseline();(ROOT/'baseline-after.txt').write_text(old_after+'\n');assert old==old_after
 save('result.json',{'revision':REV,'architecture':ARCH,'runtimeSourceIdenticalTo':'4734f285','existingQualifiedCoreAndNetworkUnchanged':True,'sourcesUnchanged':True,'imageBuild':False,'wanQualified':False,'productionQualified':False});save('state.json',{'phase':'terminal','exit':0,'revision':REV})
except Exception as e:save('failure.json',{'error':str(e),'productionQualified':False});save('state.json',{'phase':'terminal','exit':1,'revision':REV});sys.exit(1)
