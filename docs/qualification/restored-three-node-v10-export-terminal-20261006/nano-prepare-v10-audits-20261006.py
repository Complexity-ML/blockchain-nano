from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano')
pairs=[('nano-v9-dispatch-post-stop-native-diagnostic-20261006.py','nano-v10-export-post-stop-native-diagnostic-20261006.py'),('nano-post-v9-dispatch-vps-audit-20261006.py','nano-post-v10-export-vps-audit-20261006.py')]
native_digest='f4875e1f2a9159995acec060919335a889f65661c0d376306950060f27fb517a'
text=(repo/'docs/qualification/retained-export-contention-20261006/amd64/export-runtime-amd64-20261006/programs.sha256').read_text();amd_digest=next(l.split()[0] for l in text.splitlines() if l.endswith('/nano-capella-paused-capture'))
for old,new in pairs:
 s=Path('/private/tmp',old).read_text().replace('nano-restored-three-node-v9-dispatch-20261006','nano-restored-three-node-v10-export-20261006').replace('restored-three-node-v9-dispatch-prepare-20261006','restored-three-node-v10-export-prepare-20261006').replace('nano-v9-dispatch-post-stop-native-diagnostic-20261006','nano-v10-export-post-stop-native-diagnostic-20261006').replace('nano-post-v9-dispatch-vps-audit-20261006','nano-post-v10-export-vps-audit-20261006').replace("_terminal['originalSession']==37036","_terminal['originalSession']==76765").replace('/private/tmp/nano-atomic-proposal-native-20261006/nano-capella-paused-capture','/private/tmp/nano-export-contention-native-20261006/nano-capella-paused-capture').replace('2b4672397a5c46d70a2baeb19416963c9176675d7cbb5d00b73ed96640e5880f',native_digest).replace('/build/atomic-runtime-amd64-d71f574a-20261006/nano-capella-paused-capture','/build/export-runtime-amd64-20261006/nano-capella-paused-capture').replace('ef0731bd68d25d66e9073db01e835e6c576983d625b1de4a6dd98e331241b2fd',amd_digest)
 s=s.replace("['ssh','-o','BatchMode=yes'","['/usr/bin/ssh','-4','-o','BatchMode=yes'").replace("['scp',str(out/f)","['/usr/bin/scp','-4',str(out/f)")
 if 'native-diagnostic' in new:
  a=s.index("assert read(root/'campaign/result.json')['stopReceipts']==2");b=s.index('\nfor name,n in',a)
  s=s[:a]+"assert read(root/'campaign/result.json')['stopReceipts']==3,'Original controller must have stopped all three owners'\nassert len(list((root/'campaign').glob('[0-2]-external-stop-receipt.json')))==3,'Missing original owner stop receipt'\n"+s[b:]
 compile(s,new,'exec');Path('/private/tmp',new).write_text(s)
print('Post-v10 read-only audits prepared; guarded on original controller session 76765 terminal evidence. No DB opened.')
