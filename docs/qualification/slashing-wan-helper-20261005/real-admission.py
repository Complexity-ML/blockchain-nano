import pathlib,json,subprocess,os,socket,time,hashlib
root=pathlib.Path('/private/tmp/nano-slashing-helper-build-v3-20261005/real-admission');root.mkdir()
base=pathlib.Path('/tmp/nano-monotonic-runtime-native-build-20261005');fixture=root/'fixture'
subprocess.run([str(base/'nano-capella-transition-tests'),'--export-network',str(fixture)],check=True,stdout=(root/'export.log').open('w'))
c=json.loads((fixture/'node-0.json').read_text());data=root/'data';data.mkdir()
s=socket.socket();s.bind(('127.0.0.1',0));port=s.getsockname()[1];s.close()
c.update({'rpc_port':port,'listen_addr':'/ip4/127.0.0.1/tcp/0','bootnodes':[],'p2p_binary':str(base/'nano-capella-slashing-fixture'),'p2p_identity':str(data/'peer.key')})
c['capella']['profile']['file']=str(fixture/'profile.json');c['capella']['initial']['file']=str(fixture/'initial.json');c['capella']['recoveryWitness']=str(data/'witness');c['capella']['clock']['maximumWallClockDriftMs']='0xfa'
for k in c['capella']['validator']['keys']:k['journal']=str(data/('key-'+str(int(k['index'],16))));k['recoveryWitness']=k['journal']+'-witness'
(root/'node.json').write_text(json.dumps(c,indent=2)+'\n')
subprocess.run([str(base/'nano-capella-slashing-fixture'),'--generate',str(fixture/'genesis.json'),str(root/'node.json'),str(root/'proof.json')],check=True,stdout=(root/'generate.log').open('w'))
env={k:v for k,v in os.environ.items() if not k.startswith('NANO_TEST_')};env.update(json.loads((fixture/'env-0.json').read_text()));env.update({'NANO_TEST_SLASHING_FILE':str(root/'proof.json'),'NANO_TEST_SLASHING_DAEMON':'/tmp/nano-coherent-native-build-20261004/network/nano-p2p'})
with (root/'node.log').open('w') as log:
 p=subprocess.Popen([str(base/'nano'),'run',str(fixture/'genesis.json'),str(data/'chain'),str(root/'node.json')],env=env,stdout=log,stderr=subprocess.STDOUT)
 success=False
 try:
  deadline=time.monotonic()+30
  while time.monotonic()<deadline and p.poll() is None:
   if 'private-slashing-bootstrap-passed' in (root/'node.log').read_text():success=True;break
   time.sleep(.1)
 finally:
  if p.poll() is None:p.terminate()
  try:code=p.wait(timeout=20)
  except subprocess.TimeoutExpired:p.kill();code=p.wait()
 (root/'node-exit.txt').write_text(str(code)+'\n')
rows=[]
for line in (root/'node.log').read_text().splitlines():
 try:rows.append(json.loads(line))
 except ValueError:pass
receipts=[r for r in rows if r.get('event')=='private-slashing-bootstrap-passed'];assert success and len(receipts)==1 and receipts[0]['forgedRefusals']==4 and code==0
stop=[r for r in rows if r.get('event')=='capella-runtime-stopped'];assert len(stop)==1 and stop[0]['clockHealthy'] and stop[0]['validator']['healthy']
(root/'result.json').write_text(json.dumps({'nativeCliExit':code,'realCoreIpcAdmission':True,'forgedProofRefusals':4,'validAndReverseAccepted':True,'clockHealthyAtStop':True,'validatorHealthyAtStop':True,'physicalHosts':1,'wanQualification':False,'finalizedPenaltyQualified':False,'productionQualified':False},indent=2)+'\n')
print('Real native IPC: four forged proof refusals, valid/reverse accepted; clean stop')
