import pathlib,subprocess,json,hashlib,time,sys
ROOT=pathlib.Path(__file__).resolve().parent;SOURCE=pathlib.Path('/tmp/nano-pruning-wan-source-20261004');BUILD=pathlib.Path('/tmp/nano-pruning-wan-native-build-20261004');REV='092a153ca1d203434beb14fe2102e3a092dc454c';P2P=pathlib.Path('/tmp/nano-coherent-native-build-20261004/network/nano-p2p')
def save(n,v):(ROOT/n).write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def manifest():
 names=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0');return {n.decode():hashlib.sha256((SOURCE/n.decode()).read_bytes()).hexdigest() for n in names if n}
def run(stage,args,timeout):
 save('state.json',{'phase':stage,'revision':REV,'build':str(BUILD),'productionQualified':False});save(stage+'-command.json',args)
 with (ROOT/(stage+'.log')).open('w') as log:r=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
 (ROOT/(stage+'-exit.txt')).write_text(str(r.returncode)+'\n')
 if r.returncode:raise RuntimeError(stage+' failed; keep logs')
try:
 assert not BUILD.exists();assert subprocess.check_output(['git','-C',str(SOURCE),'rev-parse','HEAD'],text=True).strip()==REV;assert not subprocess.check_output(['git','-C',str(SOURCE),'status','--porcelain'],text=True)
 subprocess.run(['git','-C',str(SOURCE),'diff','--exit-code','efbeb9bc',REV,'--','network'],check=True,stdout=subprocess.DEVNULL)
 before=manifest();save('sources-before.json',before);p2p_hash=hashlib.sha256(P2P.read_bytes()).hexdigest();save('network-reuse.json',{'sourceIdenticalTo':'efbeb9bc','binary':str(P2P),'sha256':p2p_hash})
 run('configure',['cmake','-S',str(SOURCE),'-B',str(BUILD),'-DCMAKE_BUILD_TYPE=Release','-DNANO_NETWORK_EXECUTABLE='+str(P2P)],120)
 run('build',['cmake','--build',str(BUILD),'--parallel','2','--target','nano','nano-capella-transition-tests'],7200)
 run('pruned-restore',[str(BUILD/'nano-capella-transition-tests'),'--historical-pruning-restore'],180)
 after=manifest();save('sources-after.json',after);assert before==after;assert hashlib.sha256(P2P.read_bytes()).hexdigest()==p2p_hash
 programs={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [BUILD/'nano',BUILD/'nano-capella-transition-tests',P2P]};save('programs.json',programs)
 (BUILD/'production-source-revision.txt').write_text(REV+'\n');save('result.json',{'revision':REV,'platform':'darwin-arm64','build':str(BUILD),'prunedRestorePassed':True,'sourcesUnchanged':True,'p2pSourceAndBinaryUnchanged':True,'productionQualified':False});save('state.json',{'phase':'terminal','exit':0,'revision':REV})
except Exception as e:
 save('failure.json',{'error':str(e),'productionQualified':False});save('state.json',{'phase':'terminal','exit':1,'revision':REV});sys.exit(1)
