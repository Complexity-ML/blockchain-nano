import pathlib,json,subprocess,shlex,concurrent.futures
root=pathlib.Path(__file__).parent;out=pathlib.Path('/private/tmp/nano-exit-wan-current-20261005')
a=json.loads((out/'artifacts.json').read_text());resources=json.loads((out/'resources.json').read_text())
ssh=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166']
remote_request=resources['remoteRoot']+'/native-audit-request.json'
subprocess.run(['/tmp/nano-wan-dual-family-transport-control-20261004/bin/scp','-q',str(out/'vps-audit-request.json'),'ubuntu@145.239.199.166:'+remote_request],check=True,timeout=30)
cmds={'native':[a['native']['nano']['path'],'audit-capella',str(out/'fixture/genesis.json'),str(out/'mac-data/chain'),str(out/'native-audit-request.json')],
'vps':ssh+[shlex.join(['sudo','-n','docker','run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src='+a['vps']['volume']+',dst=/build,readonly','--mount','type=volume,src='+resources['volume']+',dst=/data,readonly','--mount','type=bind,src='+resources['remoteRoot']+'/config,dst=/config,readonly','--mount','type=bind,src='+remote_request+',dst=/request.json,readonly','--entrypoint',a['vps']['executables']['nano']['path'],a['vps']['image'],'audit-capella','/config/genesis.json','/data/chain','/request.json'])]}
def run(name,cmd):
 p=subprocess.run(cmd,text=True,capture_output=True,timeout=180)
 (out/(name+'-audit.stdout')).write_text(p.stdout);(out/(name+'-audit.stderr')).write_text(p.stderr);(out/(name+'-audit-exit.txt')).write_text(str(p.returncode)+'\n')
 assert p.returncode==0,p.stderr
 audit=json.loads(p.stdout);request=json.loads((out/(name+'-audit-request.json')).read_text())
 assert audit['canonicalHistoryVerified'] and audit['externalFinalizedAncestorVerified']
 assert audit['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and audit['checkpointDigest']==request['expectedCheckpoint']['digest']
 assert audit['requiredFinalized']==request['requiredFinalized']
 (out/(name+'-audit.json')).write_text(json.dumps(audit,indent=2)+'\n');return audit
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
 results=list(pool.map(lambda kv:run(*kv),cmds.items()))
assert results[0]['head']==results[1]['head'] and results[0]['finalized']==results[1]['finalized']
(root/'offline-audit-summary.json').write_text(json.dumps({'bothCanonicalReplaysPassed':True,'requiredObservedFinalityVerified':True,'stopTokensIndependentlyVerified':True,'sameCanonicalHeadAndFinality':True,'productionQualified':False,'audits':results},indent=2)+'\n')
print('Two stopped-store readonly canonical audits passed')
