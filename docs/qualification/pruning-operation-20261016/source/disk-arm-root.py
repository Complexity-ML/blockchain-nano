#!/usr/bin/env python3
import pathlib,json,hashlib,types,subprocess,os,sys
E=pathlib.Path(__file__).resolve().parent;P=E/'qualify-complete-nano-arm-root-attempt02-20261016.py';b=P.read_bytes();assert hashlib.sha256(b).hexdigest()=='a73f0267359a4ac155b5a63a005385930b558a3408343622e8909931ffe92442'
m=types.ModuleType('disk_selected_tools');m.__file__=str(P);exec(compile(b,str(P),'exec'),m.__dict__)
control=E/'pruning-sst-disk-arm-root-control-20261016.private.json';runner=E/'pruning-sst-disk-runner-source-20261016/runner.py';OUT=E/'pruning-sst-disk-arm-root-qualification-20261016-01';m.OUT=OUT
m.hold(control,{'sha256':'b893946044e5fb06ab60077a755fb53c4dbef0f0499883b814eb908e18b5a107','bytes':1273});m.hold(runner,{'sha256':'4a7880eed18315ac931adfe1f02ea344642488f2705c23817e776973c340392d','bytes':14526});assert runner.stat().st_mode&0o777==0o400
cp=json.loads(m.hold(E/'campaign-resource-guard-root-control-20261016/probe-only-config.private.json',{'sha256':'fed2abc6d9fd4107be44b4a40e767c05745365515bbc3df35b81cfaa8d4d9209','bytes':2490}))['tools']['docker'];row={k:int(v)for k,v in zip(m.F,cp['material'])};row['sha256']=cp['sha256'];m.hold(pathlib.Path(cp['path']),row);m.fence()
q=subprocess.run([sys.executable,str(runner),'--control',str(control),'--sha256','b893946044e5fb06ab60077a755fb53c4dbef0f0499883b814eb908e18b5a107','--bytes','1273'],capture_output=True,timeout=260)
m.fence()
if OUT.is_dir():
 (OUT/'root-runner-stdout.private.log').write_bytes(q.stdout);(OUT/'root-runner-stderr.private.log').write_bytes(q.stderr);m.save(OUT/'root-tool-fence-public.json',{'exitCode':q.returncode,'selectedToolsAndControlsFullNineAndShaUnchanged':True,'heldFiles':len(m.HELD),'sdkRead':False,'originalAuthorityRefreshed':False})
assert q.returncode==0,q.stderr.decode();j=json.loads((OUT/'result-public.json').read_bytes());assert j['passed']and not j['productionQualified']and j['compiledTranslationUnits']==0;print(json.dumps({'passed':True,'owner':j['owner'],'filesystemType':j['filesystemType'],'sstSummary':j['sstSummary'],'newTmpEmptyAfterTest':j['newTmpEmptyAfterTest'],'ownedContainersAbsent':j['ownedContainersAbsent']}),flush=True)
