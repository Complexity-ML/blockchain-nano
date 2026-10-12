#!/usr/bin/env bash
set -euo pipefail
python3 - "$1" "$2" <<'PY'
import json,sys,xml.etree.ElementTree as E
registry=json.load(open(sys.argv[1])); names=[t['name'] for t in registry['tests']]
if not names or len(names)!=len(set(names)): raise SystemExit('empty or duplicated registry')
root=E.parse(sys.argv[2]).getroot()
if root.tag!='testsuite': raise SystemExit('unexpected JUnit root')
cases=root.findall('testcase'); actual=[t.get('name') for t in cases]
if len(actual)!=len(set(actual)) or set(actual)!=set(names): raise SystemExit('missing, duplicated or foreign JUnit result')
if int(root.get('tests','-1'))!=len(names): raise SystemExit('JUnit count mismatch')
if any(int(root.get(k,'0'))!=0 for k in ['skipped','disabled']): raise SystemExit('JUnit reports skipped/disabled tests')
for t in cases:
    if t.get('status') not in ['run','fail'] or t.findall('skipped'): raise SystemExit('skipped/non-executed result: '+t.get('name',''))
    if t.get('status')=='fail' and not (t.findall('failure') or t.findall('error')): raise SystemExit('failed status without failure evidence')
failed=[t.get('name') for t in cases if t.findall('failure') or t.findall('error')]
if int(root.get('failures','0'))+int(root.get('errors','0'))!=len(failed): raise SystemExit('JUnit failure count mismatch')
print(json.dumps({'registered':len(names),'reported':len(actual),'failed':failed,'skipped':0,'complete':True}))
if failed: raise SystemExit(1)
PY
