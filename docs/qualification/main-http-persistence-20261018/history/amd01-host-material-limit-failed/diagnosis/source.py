import json,stat
from pathlib import Path
p=Path('/home/ubuntu/main-http-amd-qualification-root-20261018-01')
print(json.dumps({'selectedTools':[{'path':n,'bytes':Path(n).stat().st_size,'links':Path(n).stat().st_nlink,'mode':oct(stat.S_IMODE(Path(n).stat().st_mode))} for n in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo')],'publicAdmissions':sorted(x.name for x in p.iterdir() if x.is_file())}))
