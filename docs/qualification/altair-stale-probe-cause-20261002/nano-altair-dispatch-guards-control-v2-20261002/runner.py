import subprocess,json,os,hashlib,time,shlex
from pathlib import Path
root=Path('/home/ubuntu/nano-altair-dispatch-guards-control-v2-20261002');source=Path('/home/ubuntu/nano-altair-dispatch-guards-candidate-20261002');reports=Path('/home/ubuntu/nano-altair-dispatch-guards-reports-v2-20261002');reports.mkdir(exist_ok=False)
image='sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c'
raw=subprocess.check_output(['sudo','-n','docker','run','--rm','--network','none','-v','nano-linux-sync-build:/build:ro',image,'cat','/build/network/CMakeFiles/nano-p2p.dir/link.txt'],text=True)
link=shlex.split(raw);link[3]='/reports/main.cpp.o';link[5]='/reports/nano-p2p-diagnostic'
paths=[x for x in link if x.startswith('/src/')]+['/build/network/nano-p2p','/build/network/nano-altair-probe']
compile_cmd=['timeout','-s','KILL','600','/usr/bin/clang++','-O3','-DNDEBUG','-std=gnu++20','-DCARES_STATICLIB','-DFMT_CONSTEVAL=','-I/diag/third_party','-isystem','/src/.deps/install/include','-isystem','/src/.deps/hunter/_Base/15ca502/0221b56/9613f97/Install/include','-c','/diag/network/main.cpp','-o','/reports/main.cpp.o']
script='#!/bin/bash\nset -eu\ncd /build/network\nsha256sum '+shlex.join(paths)+' > /reports/network-inputs-before.sha256\nset +e\n'+shlex.join(compile_cmd)+' > /reports/network-build.log 2>&1\ncode=$?\nif [ "$code" -eq 0 ]; then '+shlex.join(['timeout','-s','KILL','120']+link)+' >> /reports/network-build.log 2>&1; code=$?; fi\nset -e\nprintf "%s\\n" "$code" > /reports/network-build-exit.txt\nsha256sum '+shlex.join(paths)+' > /reports/network-inputs-after.sha256\ncmp /reports/network-inputs-before.sha256 /reports/network-inputs-after.sha256\nexit "$code"\n'
(reports/'network-build.sh').write_text(script)
(reports/'network-link-original.txt').write_text(raw)
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip();assert revision.startswith('62e25ba3');assert not subprocess.check_output(['git','status','--porcelain'],cwd=source,text=True)
files=subprocess.check_output(['git','ls-files','-z'],cwd=source).decode().split('\0');files=[x for x in files if x]
def manifest():return {x:hashlib.sha256((source/x).read_bytes()).hexdigest() for x in files}
(reports/'source-before.json').write_text(json.dumps(manifest(),sort_keys=True));(reports/'source-revision.txt').write_text(revision+'\n')
cmd=['sudo','-n','docker','run','--rm','--name','nano-altair-dispatch-guards-v2-amd64','--user',f'{os.getuid()}:{os.getgid()}','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m','-v','nano-linux-sync-build:/build:ro','-v',f'{source}:/diag:ro','-v',f'{root}:/control:ro','-v',f'{reports}:/reports','sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c','bash','/control/run.sh']
(reports/'command.json').write_text(json.dumps(cmd,indent=2));(root/'state.json').write_text(json.dumps({'phase':'running','pid':os.getpid()}))
with (reports/'container.log').open('w') as f:code=subprocess.run(cmd,stdout=f,stderr=f).returncode
(reports/'container-exit.txt').write_text(str(code)+'\n');(reports/'source-after.json').write_text(json.dumps(manifest(),sort_keys=True));assert (reports/'source-before.json').read_bytes()==(reports/'source-after.json').read_bytes();(root/'state.json').write_text(json.dumps({'phase':'terminal','exit':code,'pid':os.getpid()}))
