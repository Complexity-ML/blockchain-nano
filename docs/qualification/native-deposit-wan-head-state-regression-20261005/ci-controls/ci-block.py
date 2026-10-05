import json,pathlib,subprocess,sys
root=pathlib.Path('ci-reports/deposit-fixture').resolve()
base=json.loads((root/'deposit.json').read_text())
(root/'unused-request.json').write_text('{}')
binary=str(pathlib.Path(sys.argv[1],'nano-capella-deposit-audit').resolve())
results=[]
for name,field,value,expected in [('wrong-index','validatorIndex',65,'deposit fixture identity'),('wrong-key','publicKey','0x'+'00'*48,'known public fixture key65 required'),('wrong-amount','amountGwei',1,'deposit fixture identity')]:
    invalid=dict(base);invalid[field]=value
    proof=root/(name+'.json');proof.write_text(json.dumps(invalid))
    never_db=root/(name+'-never-create-db')
    audit=subprocess.run([binary,'--audit',str(root/'genesis.json'),str(root/'node-0.json'),str(never_db),str(root/'unused-request.json'),str(proof)],capture_output=True,text=True,timeout=30)
    pathlib.Path('ci-reports/deposit-audit-'+name+'.log').write_text(audit.stdout+audit.stderr)
    if audit.returncode!=1 or expected not in audit.stderr or never_db.exists():raise SystemExit('deposit metadata refusal/store-creation control failed: '+name)
    results.append({'case':name,'exit':1,'databaseNeverCreated':True})
pathlib.Path('ci-reports/deposit-audit-controls.json').write_text(json.dumps({'cases':results,'positiveNetworkAuditQualified':False,'productionQualified':False},indent=2)+'\n')