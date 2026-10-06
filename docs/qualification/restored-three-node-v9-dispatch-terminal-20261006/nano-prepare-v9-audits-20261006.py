from pathlib import Path
pairs=[('nano-v8-retention-terminal-verification-20261006.py','nano-v9-dispatch-terminal-verification-20261006.py'),('nano-v8-retention-post-stop-native-diagnostic-20261006.py','nano-v9-dispatch-post-stop-native-diagnostic-20261006.py'),('nano-post-v8-retention-vps-audit-20261006.py','nano-post-v9-dispatch-vps-audit-20261006.py')]
for old,new in pairs:
 s=Path('/private/tmp',old).read_text().replace('nano-restored-three-node-v8-retention-20261006','nano-restored-three-node-v9-dispatch-20261006').replace('restored-three-node-v8-retention-prepare-20261006','restored-three-node-v9-dispatch-prepare-20261006').replace('nano-v8-retention-post-stop-native-diagnostic-20261006','nano-v9-dispatch-post-stop-native-diagnostic-20261006').replace('nano-post-v8-retention-vps-audit-20261006','nano-post-v9-dispatch-vps-audit-20261006').replace("source['retentionExperiment']['blocks']","source['macConfig']['capella']['limits']['blocks']")
 # Audit scripts must never open a database while campaign writers are running.
 guard='''
from pathlib import Path as _AuditPath
import json as _AuditJson
_terminal=_AuditJson.loads(_AuditPath('/private/tmp/nano-restored-three-node-v9-dispatch-20261006/campaign/terminal-exit.json').read_text())
assert _terminal['originalSession']==37036, 'Missing original controller terminal evidence'
'''
 s=guard+s;compile(s,new,'exec');Path('/private/tmp',new).write_text(s)
print('Post-v9 controls prepared; original terminal guard required; no DB opened')
