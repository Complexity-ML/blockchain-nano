from pathlib import Path
import subprocess,shlex,json,sys,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');base=Path('/tmp/nano-coherent-native-build-20261004/network');out=Path('/private/tmp/nano-export-resume-p2p-native-20261006');out.mkdir(exist_ok=True);phase='candidate';d=out/phase;d.mkdir(exist_ok=False)
parents=[Path('/private/tmp/nano-depth-p2p-native-20261006/candidate/test'),Path('/private/tmp/nano-deposit-deadline-20261005/nano')];before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in parents};(d/'parents-before.json').write_text(json.dumps(before,indent=2)+'\n')
f=(base/'CMakeFiles/nano-p2p.dir/flags.make').read_text();get=lambda key:shlex.split(next(l.split(' = ',1)[1] for l in f.splitlines() if l.startswith(key+' = ')))
cmd=['/usr/bin/c++']+get('CXX_DEFINES')+get('CXX_INCLUDES')+get('CXX_FLAGS')+['-I'+str(repo/'network'),'-I'+str(repo/'third_party'),'-c',str(repo/'network/main.cpp'),'-o',str(d/'test.o')]
link=shlex.split((base/'CMakeFiles/nano-p2p.dir/link.txt').read_text());link=[str(d/'test.o') if s.endswith('.cpp.o') else str(d/'test') if s=='nano-p2p' else s for s in link]
for name,command in [('compile',cmd),('link',link),('run',[str(d/'test')])]:
 (d/(name+'-command.json')).write_text(json.dumps(command,indent=2)+'\n')
 with (d/(name+'.log')).open('w') as log:r=subprocess.run(command,cwd=base,stdout=log,stderr=subprocess.STDOUT)
 (d/(name+'-exit.txt')).write_text(str(r.returncode)+'\n');print(name,r.returncode,flush=True)
 if name!='run' and r.returncode:raise SystemExit(r.returncode)
 if name=='run':assert r.returncode==1 and 'Usage: nano-p2p' in (d/'run.log').read_text()
assert before=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in parents}
(d/'parents-unchanged.json').write_text(json.dumps(before,indent=2)+'\n')
(d/'program.json').write_text(json.dumps({'path':str(d/'test'),'sha256':hashlib.sha256((d/'test').read_bytes()).hexdigest(),'negativeCliExit':1,'networkQualified':False},indent=2)+'\n')
