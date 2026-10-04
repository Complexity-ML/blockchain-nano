import pathlib,shlex,subprocess,json,hashlib
base=pathlib.Path('/build/finalized-retention-4734f285-arm64');output=pathlib.Path('/build/pruning-restore-test-20261004');assert output.is_dir() and not any(output.iterdir())
flags=(base/'CMakeFiles/nano-execution-checkpoint-tests.dir/flags.make').read_text()
values={line.split(' = ',1)[0]:line.split(' = ',1)[1] for line in flags.splitlines() if ' = ' in line}
obj=output/'test_execution_checkpoint.cpp.o';binary=output/'nano-pruning-restore-tests'
compile=['clang++']+shlex.split(values['CXX_DEFINES'])+shlex.split(values['CXX_INCLUDES'])+shlex.split(values['CXX_FLAGS'])+['-c','/work/tests/test_execution_checkpoint.cpp','-o',str(obj)]
link=shlex.split((base/'CMakeFiles/nano-execution-checkpoint-tests.dir/link.txt').read_text())
old='CMakeFiles/nano-execution-checkpoint-tests.dir/tests/test_execution_checkpoint.cpp.o';assert old in link;link[link.index(old)]=str(obj);link[link.index('-o')+1]=str(binary)
report=pathlib.Path('/report');(report/'commands.json').write_text(json.dumps({'compile':compile,'link':link,'baselineBuild':str(base)},indent=2)+'\n')
for stage,args in [('compile',compile),('link',link),('test',[str(binary),'--pruning-restore'])]:
 with (report/(stage+'.log')).open('w') as log:r=subprocess.run(args,cwd=base,stdout=log,stderr=subprocess.STDOUT)
 (report/(stage+'-exit.txt')).write_text(str(r.returncode)+'\n')
 if r.returncode:raise SystemExit(r.returncode)
(report/'program-sha256.txt').write_text(hashlib.sha256(binary.read_bytes()).hexdigest()+'  '+str(binary)+'\n')
print('New offline pruning restore scenario exit 0')
