#!/usr/bin/env python3
"""Read only source CI metadata/annotations; no SSH, settings mutation, or secrets."""
from pathlib import Path
import subprocess,json,hashlib,os,datetime,concurrent.futures
ROOT=Path(__file__).parent/'source-ci-refusal-d7-20261013'
REPO=Path('/Users/boris/Dev/blockchain-nano')
RUN=38325619830
HEAD='d7cc1bdb63f6d5035adc6f4b2420823f135f1bf1'
assert not ROOT.exists();ROOT.mkdir(mode=0o700)
def save(name,data):
 fd=os.open(ROOT/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as stream:stream.write(data)
def api(name,endpoint):
 command=['gh','api','--method','GET','-H','Accept: application/vnd.github+json',endpoint]
 env=os.environ.copy();env.pop('GH_DEBUG',None)
 r=subprocess.run(command,cwd=REPO,env=env,capture_output=True,timeout=60)
 save(name+'.json',r.stdout);save(name+'.stderr.log',r.stderr)
 save(name+'.request.json',(json.dumps({'command':command,'exitCode':r.returncode,'readOnly':True},indent=2)+'\n').encode())
 assert r.returncode==0,name
 return json.loads(r.stdout)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
 a=pool.submit(api,'run',f'repos/Complexity-ML/blockchain-nano/actions/runs/{RUN}')
 b=pool.submit(api,'jobs',f'repos/Complexity-ML/blockchain-nano/actions/runs/{RUN}/jobs?per_page=100')
 run=a.result();jobs=b.result()
assert run['id']==RUN and run['head_sha']==HEAD
assert jobs['total_count']==len(jobs['jobs'])
review=[]
for job in jobs['jobs']:
 check=job['check_run_url'];prefix='https://api.github.com/'
 assert check.startswith(prefix)
 detail=api('job-'+str(job['id'])+'-check',check[len(prefix):])
 annotations=api('job-'+str(job['id'])+'-annotations',check[len(prefix):]+'/annotations?per_page=100')
 assert detail['output']['annotations_count']==len(annotations)
 review.append({'jobId':job['id'],'name':job['name'],'status':job['status'],'conclusion':job['conclusion'],
                'steps':job['steps'],'stepCount':len(job['steps']),
                'annotations':[{'annotationLevel':a['annotation_level'],'message':a['message'],'title':a.get('title'),'path':a['path']} for a in annotations]})
summary={'format':'nano-source-ci-current-refusal-review-v1','runId':RUN,'runUrl':run['html_url'],
         'headSha':run['head_sha'],'status':run['status'],'conclusion':run['conclusion'],
         'createdAt':run['created_at'],'updatedAt':run['updated_at'],
         'allReportedJobsRetrieved':True,'allReportedAnnotationsRetrieved':True,
         'totalStepCount':sum(j['stepCount'] for j in review),'jobs':review,
         'scope':'Actual API run/jobs/check annotations; no inherited billing inference, no CI test result claimed.',
         'privateVpsRecoveryFilesAccessedOrExported':False,'repositoryOrAccountSettingsModified':False}
save('terminal-review.json',(json.dumps(summary,indent=2)+'\n').encode())
save('collector-source.py',Path(__file__).read_bytes())
save('README.md',('''# Current GitHub source CI refusal

The raw run, jobs, check-run details and annotations were retrieved through read-only GitHub API calls for run 38325619830 at commit d7cc1bdb63f6d5035adc6f4b2420823f135f1bf1. The terminal review copies each actual annotation message and each reported step without inferring the reason from another run. A run rejected before any steps is not a C++ or test failure. No billing settings were changed and no private VPS recovery evidence was accessed or transferred.
''').encode())
rows=[]
for p in sorted(ROOT.iterdir()):
 assert p.is_file() and not p.is_symlink();data=p.read_bytes()
 rows.append({'path':p.name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
manifest={'format':'nano-public-source-ci-refusal-evidence-v1','files':rows,'payloadCount':len(rows),'allApiRequestsReadOnly':True}
data=(json.dumps(manifest,indent=2)+'\n').encode();save('manifest.json',data)
print(json.dumps({'directory':str(ROOT),'manifestSha256':hashlib.sha256(data).hexdigest(),
                  'terminalSha256':hashlib.sha256((ROOT/'terminal-review.json').read_bytes()).hexdigest(),
                  'headSha':HEAD,'conclusion':run['conclusion'],'totalStepCount':summary['totalStepCount'],
                  'annotations':[a['message'] for j in review for a in j['annotations']]}))
