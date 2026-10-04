from pathlib import Path
import json,hashlib,subprocess,sys,xml.etree.ElementTree as ET
if len(sys.argv)!=2: raise SystemExit('Usage: review.py NEW-OUTPUT-DIRECTORY (VPS; after terminal tests and WAN)')
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=False)
r=Path('/home/ubuntu/nano-coherent-full-amd64-reports-20261004');src=Path('/home/ubuntu/nano-production-coherent-20261004');rev='efbeb9bca3d42e41edb9cb81afba98c0b953a90f';image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
assert Path('/home/ubuntu/nano-coherent-full-amd64-control-20261004/controller-exit.txt').read_text().strip()=='0'
for n in ['ctest-exit.txt','audit-exit.txt','post-test-manifest-exit.txt']:assert (r/n).read_text().strip()=='0'
assert subprocess.check_output(['git','-C',str(src),'rev-parse','HEAD'],text=True).strip()==rev
assert not subprocess.check_output(['git','-C',str(src),'status','--porcelain'],text=True).strip()
reg=json.loads((r/'registry.json').read_text());cases=ET.parse(r/'results.xml').findall('.//testcase');names=[c.attrib['name'] for c in cases];assert len(names)==len(set(names))==427
assert set(names)=={t['name'] for t in reg['tests']};assert not any(c.findall('failure') or c.findall('error') or c.findall('skipped') for c in cases)
sources=json.loads((r/'source-manifest-after-tests.json').read_text());assert sources==json.loads((r/'source-manifest.json').read_text())
assert set(sources)=={p.decode() for p in subprocess.check_output(['git','-C',str(src),'ls-files','-z']).split(b'\0') if p}
assert all(hashlib.sha256((src/p).read_bytes()).hexdigest()==h for p,h in sources.items())
programs=json.loads((r/'executables-after-tests.json').read_text());assert programs==json.loads((r/'executables-before-tests.json').read_text())
subprocess.check_call(['sudo','-n','docker','volume','inspect','nano-clean-rpc-build-20261002'],stdout=subprocess.DEVNULL)
assert subprocess.check_output(['sudo','-n','docker','image','inspect',image,'--format','{{.Id}}'],text=True).strip()==image
cmd=['sudo','-n','docker','run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','1','--memory','512m','--mount','type=volume,src=nano-clean-rpc-build-20261002,dst=/build,readonly',image,'sha256sum',*programs]
raw=subprocess.check_output(cmd,text=True);actual={line.split()[1]:line.split()[0] for line in raw.splitlines()};assert actual==programs
(out/'programs-rehashed.json').write_text(json.dumps(actual,indent=2,sort_keys=True)+'\n')
for name,cmd in [('results',['bash','scripts/ci/verify-results.sh',str(r/'registry.json'),str(r/'results.xml')]),('eest',['node','scripts/ci/audit-eest-ctest.mjs',str(r/'LastTest.log')])]:
 result=subprocess.run(cmd,cwd=src,capture_output=True,text=True);(out/(name+'.json')).write_text(result.stdout);(out/(name+'.stderr')).write_text(result.stderr);assert result.returncode==0
assert subprocess.check_output(['git','-C',str(src),'rev-parse','HEAD'],text=True).strip()==rev
assert not subprocess.check_output(['git','-C',str(src),'status','--porcelain'],text=True).strip()
assert all(hashlib.sha256((src/p).read_bytes()).hexdigest()==h for p,h in sources.items())
(out/'result.json').write_text(json.dumps({'exit':0,'revision':rev,'image':image,'registered':427,'reported':427,'sourcesRehashed':len(sources),'programsRehashed':len(programs),'sourceAndProgramManifestsUnchanged':True,'eestScope':json.loads((out/'eest.json').read_text()),'nativeFullRegression':False,'amd64TerminalVerified':True,'restorationQualified':False,'productionQualified':False},indent=2)+'\n')
print((out/'result.json').read_text())
