import assert from 'node:assert/strict';
// Executed inside the same container namespace after SIGSTOP of its PID 1.
export const writerObservationProgram = `import pathlib,os,json
p=pathlib.Path('/proc/1')
stat=(p/'stat').read_text(); fields=stat[stat.rfind(')')+2:].split()
tasks=[]
for t in sorted((p/'task').iterdir(),key=lambda x:int(x.name)):
 d=dict(line.split(':',1) for line in (t/'status').read_text().splitlines() if ':' in line)
 tasks.append({'tid':int(t.name),'state':d['State'].strip().split()[0]})
print(json.dumps({'namespacePid':1,'executable':os.readlink(p/'exe'),'startTicks':fields[19],'tasks':tasks}))`;
export function verifyWriterSuspension(before,after,samples,expected){
 assert.equal(before.Id,expected.containerId);assert.equal(after.Id,before.Id);
 for(const s of [before,after]){assert.equal(s.State.Running,true);assert.equal(s.State.Paused,false);assert.equal(s.State.Restarting,false);assert.equal(s.Path,expected.executable);assert.ok(s.State.Pid>0);assert.ok(s.Mounts.some(m=>m.Name===expected.volume&&m.Destination==='/var/lib/nano'));}
 assert.equal(before.State.Pid,after.State.Pid);assert.equal(before.State.StartedAt,after.State.StartedAt);
 assert.equal(samples.length,2);
 for(const s of samples){assert.equal(s.namespacePid,1);assert.equal(s.executable,expected.executable);assert.match(s.startTicks,/^[1-9][0-9]*$/);assert.ok(s.tasks.length>0);assert.ok(s.tasks.some(t=>t.tid===1));assert.equal(new Set(s.tasks.map(t=>t.tid)).size,s.tasks.length);for(const t of s.tasks){assert.ok(Number.isInteger(t.tid)&&t.tid>0);assert.equal(t.state,'T','Every writer thread must be stopped');}}
 assert.equal(samples[0].startTicks,samples[1].startTicks);assert.deepEqual(samples[0].tasks,samples[1].tasks);
 return {writersSuspended:true,containerId:before.Id,hostPid:before.State.Pid,startedAt:before.State.StartedAt,executable:expected.executable,volume:expected.volume,samples};
}
