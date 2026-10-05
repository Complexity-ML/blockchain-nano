import pathlib,json,subprocess,shlex,concurrent.futures
out=pathlib.Path('/private/tmp/nano-slashing-wan-v2-20261005');control=pathlib.Path('/private/tmp/nano-slashing-wan-v2-offline-20261005');control.mkdir()
a=json.loads((out/'artifacts.json').read_text());tools=json.loads((out/'tools.json').read_text());resources=json.loads((out/'resources.json').read_text());common=json.loads((out/'common-finality.json').read_text())
assert json.loads((out/'result.json').read_text())['result']=='FAILED'
requests=[]
for n in range(2):
 req=json.loads((out/f'{n}-audit-request.json').read_text())
 for key in ['profile','anchor']:req[key]={k:req[key][k] for k in ['file','sha256']}
 assert req['requiredFinalized']==common['afterRestart']
 (control/f'{n}-request.json').write_text(json.dumps(req,indent=2)+'\n');requests.append(req)
ssh=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166'];scp=ssh[0].replace('/ssh','/scp')
state=json.loads(subprocess.check_output(ssh+[shlex.join(['sudo','-n','docker','inspect',resources['remote'],'--format','{{json .State}}'])],text=True,timeout=30));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled'];(control/'stopped-vps.json').write_text(json.dumps(state,indent=2))
remote_request=resources['remoteRoot']+'/slashing-audit-corrected-request.json'
subprocess.run([scp,'-q',str(control/'1-request.json'),'ubuntu@145.239.199.166:'+remote_request],check=True,timeout=30)
cmds={'native':[tools['native']['audit']['path'],'--audit',str(out/'fixture/genesis.json'),str(out/'mac.json'),str(out/'mac-data/chain'),str(control/'0-request.json'),str(out/'fixture/proof.json')],
'vps':ssh+[shlex.join(['sudo','-n','docker','run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src='+a['vps']['volume']+',dst=/build,readonly','--mount','type=volume,src='+resources['volume']+',dst=/var/lib/nano,readonly','--mount','type=bind,src='+resources['remoteRoot']+'/config,dst=/config,readonly','--mount','type=bind,src='+remote_request+',dst=/request.json,readonly','--entrypoint',tools['vps']['audit']['path'],a['vps']['image'],'--audit','/config/genesis.json','/config/node-1.json','/var/lib/nano/chain','/request.json','/config/proof.json'])]}
def run(name,cmd):
 (control/(name+'-command.json')).write_text(json.dumps(cmd,indent=2))
 p=subprocess.run(cmd,text=True,capture_output=True,timeout=180)
 (control/(name+'.stdout')).write_text(p.stdout);(control/(name+'.stderr')).write_text(p.stderr);(control/(name+'-exit.txt')).write_text(str(p.returncode));print(name,p.returncode,p.stderr[-1000:],flush=True);assert p.returncode==0,p.stderr
 result=json.loads(p.stdout);assert result['finalizedSlashed'] and result['canonicalProofInclusions']==1 and result['canonicalSlashedTransitions']==1 and result['proofRetired'] and result['externalStopTokenAndWitnessVerified'] and result['readOnlyStores']
 assert result['requiredFinalized']==common['afterRestart'];assert result['canonicalReplay']['canonicalHistoryVerified'] and result['canonicalReplay']['externalFinalizedAncestorVerified']
 (control/(name+'-audit.json')).write_text(json.dumps(result,indent=2)+'\n');return result
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:audits=list(pool.map(lambda kv:run(*kv),cmds.items()))
assert audits[0]['sanction']==audits[1]['sanction'] and audits[0]['finalizedBalanceGwei']==audits[1]['finalizedBalanceGwei']
(control/'summary.json').write_text(json.dumps({'bothStoppedAuditsPassed':True,'networkNotRerun':True,'originalCollectorFailurePreserved':True,'commonFinality':common,'sanction':audits[0]['sanction'],'finalizedBalanceGwei':audits[0]['finalizedBalanceGwei'],'productionQualified':False},indent=2)+'\n')
