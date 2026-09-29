import concurrent.futures,json,subprocess,time,pathlib
out=pathlib.Path('/tmp/nano-production-wan-rotation-20260929/resources.jsonl')
name='nano-qualification-{}-mun639bh'
commands={
 'mac-docker':['docker','stats','--no-stream','--format','{{json .}}',name.format('mac')],
 'vps-docker':['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166','sudo -n docker stats --no-stream --format \'{{json .}}\' '+name.format('vps')],
 'mac-native':['ps','-p','4437','-o','pid=,rss=,%cpu=,etime=,command=']}
def take(pair):
 host,cmd=pair
 try:
  p=subprocess.run(cmd,capture_output=True,text=True,timeout=20)
  return {'host':host,'exitCode':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr.strip()}
 except Exception as e:return {'host':host,'error':str(e)}
for _ in range(90):
 with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool: rows=list(pool.map(take,commands.items()))
 with out.open('a') as f:f.write(json.dumps({'timeMs':int(time.time()*1000),'samples':rows})+'\n')
 # This observer never stops the test; it ends when its original native child ends.
 native=next(r for r in rows if r['host']=='mac-native')
 if native.get('exitCode')==1 or (native.get('exitCode')==0 and '/tmp/nano-production-wan-rotation-20260929/' not in native.get('stdout','')):break
 time.sleep(20)
