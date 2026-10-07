from pathlib import Path
import subprocess,json,sys,hashlib
root=Path(__file__).parent;arch=sys.argv[1];assert arch in ('arm64','amd64')
terminal=json.loads((root/'original-exit.json').read_text());assert terminal['session']==14735 and terminal['exitCode'] in (0,1)
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];source=Path('/Users/boris/Dev/blockchain-nano');image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';container='nano-restored-three-node-mac-muxcwsxz'
else:docker=['sudo','-n','docker'];source=Path('/home/ubuntu/blockchain-nano-git');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';container='nano-restored-three-node-vps-muxcwsxz'
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
subprocess.run(['git','diff','--quiet','df31cd48a3d1680f7d864190fcbdd5e437683a23','--','include','tests/capella/paused_capture.cpp'],cwd=source,check=True)
probe=root/(arch+'-capture-link');probe.mkdir();out='/build/startup-checkpoint-memo-'+arch+'-20261007';old='/build/coherent-e33ad89e-'+arch
base=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','-v',volume+':/build','-w',old]
def tool(a):return subprocess.check_output(base+['--entrypoint',a[0],image]+a[1:],text=True)
parents=[out+'/'+n for n in ['nano','test','libnano_core.a']];before=tool(['sha256sum']+parents);(probe/'parent-before.sha256').write_text(before)
qualified=source/'docs/qualification/capella-startup-reference-memo-20261007'/(arch+'-repair/programs.sha256');assert before==qualified.read_text()
cmd=json.loads((source/'docs/qualification/published-block-inventory-20261007'/(arch+'-runtime-link/link-capture-command.json')).read_text());cmd=[out+'/libnano_core.a' if x.endswith('/libnano_core.a') else x for x in cmd];cmd[cmd.index('-o')+1]=out+'/capture'
(probe/'command.json').write_text(json.dumps(cmd)+'\n')
with (probe/'link.log').open('w') as f:r=subprocess.run(base+['--entrypoint',cmd[0],image]+cmd[1:],stdout=f,stderr=subprocess.STDOUT)
(probe/'exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0
assert tool(['sha256sum']+parents)==before;(probe/'parent-after.sha256').write_text(before);(probe/'capture.sha256').write_text(tool(['sha256sum',out+'/capture','/build/private-capture-budget-'+arch+'-20261006/capture.o']));print('Capture linked against qualified core; parent binaries unchanged; source writer stopped')
