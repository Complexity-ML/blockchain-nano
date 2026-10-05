from pathlib import Path
import subprocess,json,shlex,tarfile,io
out=Path('/private/tmp/nano-v7-snapshot-counts-20261006')
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
for suffix,name in [('', 'first-build'),('-fixed','fixed-build')]:
 base='v7-snapshot-counts-amd64-f7785959'+suffix+'-20261006'
 command=['sudo','-n','docker','run','--rm','--network','none','-v','nano-clean-rpc-build-20261002:/build:ro','--entrypoint','tar',image,'-C','/build/'+base,'--exclude=*.o','--exclude=snapshot-counts','-cf','-','.']
 raw=subprocess.check_output(['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(command)])
 dest=out/name;dest.mkdir(exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
  for member in archive.getmembers():
   if not member.isfile():continue
   assert not Path(member.name).is_absolute() and '..' not in Path(member.name).parts
   f=dest/member.name;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(archive.extractfile(member).read())
 container='nano-v7-snapshot-counts-build'+('-fixed' if suffix else '')+'-f7785959'
 state=subprocess.check_output(['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(['sudo','-n','docker','inspect',container,'--format','{{json .State}}'])],text=True)
 (dest/'container-state.json').write_text(state)
 print(name,json.loads(state)['ExitCode'],flush=True)
