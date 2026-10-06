from pathlib import Path
import json,subprocess,shlex
p=Path('/private/tmp/nano-dispatch-recovery-20261006');b=Path('/tmp/nano-pruning-wan-native-build-20261004');r=Path('/Users/boris/Dev/blockchain-nano')
base=shlex.split((b/'CMakeFiles/nano-altair-receipts-tests.dir/link.txt').read_text())
for mode,header in [('ledger-baseline','baseline.hpp'),('ledger-candidate','candidate/altair_core_dispatch.hpp')]:
 cmd=['/usr/bin/clang++','-std=c++20','-DFMT_CONSTEVAL=','-I'+str(r/'include'),'-I'+str(r/'third_party'),'-I'+str(r/'network'),'-I/opt/homebrew/include','-DDISPATCH_HEADER="'+str(p/header)+'"','-c',str(p/'ledger.cpp'),'-o',str(p/(mode+'.o'))]
 for phase,command in [('compile',cmd),('link',[str(p/(mode+'.o')) if 'test_altair_receipts.cpp.o' in x else str(p/mode) if x=='nano-altair-receipts-tests' else '/private/tmp/nano-atomic-proposal-native-20261006/libnano_core.a' if x=='libnano_core.a' else x for x in base])]:
  (p/(mode+'-'+phase+'-command.json')).write_text(json.dumps(command)+'\n')
  with (p/(mode+'-'+phase+'.log')).open('w') as f:run=subprocess.run(command,cwd=b,stdout=f,stderr=subprocess.STDOUT)
  (p/(mode+'-'+phase+'-exit.json')).write_text(json.dumps({'exitCode':run.returncode})+'\n');assert run.returncode==0
 run=subprocess.run([str(p/mode)],capture_output=True,text=True);(p/(mode+'-run.log')).write_text(run.stdout+run.stderr);(p/(mode+'-run-exit.json')).write_text(json.dumps({'exitCode':run.returncode})+'\n');print(mode,run.returncode,run.stdout.strip(),run.stderr.strip(),flush=True)
