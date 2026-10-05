import pathlib,json,subprocess,hashlib,sys
out=pathlib.Path('/tmp/nano-capella-sync-native-build-20261005');base=pathlib.Path('/tmp/nano-pruning-wan-native-build-20261004')
for name,args in [('compile-tests-v2',json.loads((out/'compile-nano-capella-transition-tests-command.json').read_text())),('link-tests-v2',None)]:
 if args is None:
  import shlex
  args=shlex.split((base/'CMakeFiles/nano-capella-transition-tests.dir/link.txt').read_text());args=[str(out/'test.cpp.o') if a.startswith('CMakeFiles/') else str(out/'nano-capella-transition-tests') if a=='nano-capella-transition-tests' else str(out/'libnano_core.a') if a=='libnano_core.a' else a for a in args]
 with (out/(name+'.log')).open('w') as log:r=subprocess.run(args,cwd=base,stdout=log,stderr=subprocess.STDOUT)
 (out/(name+'-exit.txt')).write_text(str(r.returncode)+'\n')
 if r.returncode:sys.exit(r.returncode)
(out/'programs.json').write_text(json.dumps({str(out/p):hashlib.sha256((out/p).read_bytes()).hexdigest() for p in ['nano','nano-capella-transition-tests','libnano_core.a']},indent=2)+'\n')
(out/'controller-exit.txt').write_text('0\n')
