import pathlib,json,subprocess,os,time,socket,urllib.request,hashlib
root=pathlib.Path(__file__).parent;fixture=root/'fixture';out=root/'native-smoke';out.mkdir();data=out/'data';data.mkdir()
configs=[json.loads((fixture/f'node-{i}.json').read_text()) for i in range(2)];envs=[json.loads((fixture/f'env-{i}.json').read_text()) for i in range(2)];c=configs[0];c['capella']['validator']['keys']+=configs[1]['capella']['validator']['keys'];assert len(c['capella']['validator']['keys'])==65 and c['capella']['validator']['keys'][-1]['index'] is None
s=socket.socket();s.bind(('127.0.0.1',0));port=s.getsockname()[1];s.close();c['rpc_port']=port;c['rpc_host']='127.0.0.1';c['listen_addr']='/ip4/127.0.0.1/tcp/0';c['p2p_binary']='/tmp/nano-coherent-native-build-20261004/network/nano-p2p';c['p2p_identity']=str(data/'peer.key');c['bootnodes']=[]
for name in ['profile','initial']:c['capella'][name]['file']=str(fixture/(name+'.json'))
c['capella']['recoveryWitness']=str(data/'witness');c['capella']['clock']['maximumWallClockDriftMs']='0xfa'
for i,k in enumerate(c['capella']['validator']['keys']):k['journal']=str(data/f'key-{i}');k['recoveryWitness']=str(data/f'key-{i}-witness')
(out/'node.json').write_text(json.dumps(c,indent=2));deposit=json.loads((fixture/'deposit.json').read_text());env={k:v for k,v in os.environ.items() if not k.startswith('NANO_TEST_')};env.update(envs[0]);env.update(envs[1]);log=open(out/'node.log','w');process=subprocess.Popen(['/tmp/nano-monotonic-runtime-native-build-20261005/nano','run',str(fixture/'genesis.json'),str(data/'chain'),str(out/'node.json')],env=env,stdout=log,stderr=subprocess.STDOUT)
def rpc(method,params=[]):
 req=urllib.request.Request(f'http://127.0.0.1:{port}/',json.dumps({'jsonrpc':'2.0','id':1,'method':method,'params':params}).encode(),{'Content-Type':'application/json'});d=json.load(urllib.request.urlopen(req,timeout=10));assert 'error' not in d,d;return d['result']
try:
 for _ in range(80):
  assert process.poll() is None,'Node stopped before RPC'
  try:assert rpc('eth_chainId')=='0x539';break
  except (ConnectionError,urllib.error.URLError):time.sleep(.25)
 else:raise RuntimeError('RPC startup deadline')
 submitted=rpc('eth_sendRawTransaction',[deposit['raw']]);assert submitted==deposit['hash'];(out/'submitted.json').write_text(json.dumps({'hash':submitted,'publicSyntheticOnly':True}))
 receipt=None
 for _ in range(120):
  assert process.poll() is None,'Node stopped during deposit'
  receipt=rpc('eth_getTransactionReceipt',[submitted])
  if receipt is not None:break
  time.sleep(.25)
 assert receipt and receipt['status']=='0x1' and len(receipt['logs'])==1,receipt
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2));print('native deposit receipt success, one event',flush=True)
finally:
 if process.poll() is None:process.terminate()
 try:process.wait(timeout=20)
 except subprocess.TimeoutExpired:process.kill();process.wait()
 log.close();(out/'node-exit.txt').write_text(str(process.returncode));assert process.returncode==0
rows=[]
for line in (out/'node.log').read_text().splitlines():
 try:d=json.loads(line)
 except:continue
 if d.get('event')=='capella-runtime-stopped':rows.append(d)
assert rows and rows[-1]['clockHealthy'] and rows[-1]['validator']['healthy'] and rows[-1]['validator']['localKeys']==65
assert rows[-1]['validator']['awaitingRegistration']==1
rows[-1]['validator'].pop('dutyTrace',None);(out/'stop-receipt.json').write_text(json.dumps(rows[-1],indent=2));(out/'scope.json').write_text(json.dumps({'executionDepositReceiptVerified':True,'awaitingRegistration':1,'registeredActivationQualified':False,'networkTransportQualified':False,'productionQualified':False},indent=2));print('pending key still awaits consensus registration; activation/WAN not yet qualified')
