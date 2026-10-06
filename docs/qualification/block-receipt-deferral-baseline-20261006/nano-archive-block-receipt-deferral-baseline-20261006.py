from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-block-receipt-deferral-20261006');v15=Path('/private/tmp/nano-restored-three-node-v15-history-first-20261006');m=json.loads((root/'original-data-before.json').read_text());immut={}
for name,before in m.items():
 data=v15/name/'data';actual={str(f.relative_to(data)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(data.rglob('*')) if f.is_file()};assert actual==before;immut[name]={'files':len(actual),'hashesUnchanged':True}
d=json.loads((root/'diagnostic.json').read_text());assert d['receipt']['decision']=='rejected' and not d['admittedByOneReceipt'] and d['additionalManualAdmissionAttempts']==21 and d['admittedAfterManualContinuations']
(root/'terminal-exit.json').write_text(json.dumps({'originalSession':28205,'exitCode':1,'expectedFailure':True,'counterexampleConfirmed':True},indent=2)+'\n');(root/'immutability.json').write_text(json.dumps(immut,indent=2)+'\n')
out=repo/'docs/qualification/block-receipt-deferral-baseline-20261006';out.mkdir(exist_ok=False)
for f in root.iterdir():
 if f.is_file() and f.suffix not in ['.o'] and f.name!='diagnose':shutil.copy2(f,out/f.name)
for n in ['nano-prepare-block-receipt-deferral-20261006.py','nano-archive-block-receipt-deferral-baseline-20261006.py']:shutil.copy2(Path('/private/tmp')/n,out/n)
(out/'README.md').write_text('''# Préparation incomplète classée comme rejet — contre-exemple réel

Source 0a605778, core lié 5787915e (empreinte du libnano_core.a conservée). Session originale 28205/sortie 1 attendue. Test sur un bloc signé conservé du réseau privé v15, racine 0x35cd4a6b…, slot 10751, parent admis au slot 10574.

Le vrai CapellaConsensusService et son worker/registre de reçus rendent completed/rejected pour le premier ticket. Aucune admission ni écriture durable de ce bloc à cette étape. Le même bloc est ensuite admis après 21 continuations explicites supplémentaires sur le même reactor et les mêmes règles. Cela établit que cette préparation incomplète est annoncée comme un rejet dans ce chemin d’admission. La correction doit conserver un travail en attente, ses budgets et sa cancellation, sans ACK avant commit ni boucle qui monopolise les autres messages.

Magasins originaux readonly et 1441/1406 empreintes inchangées ; seule chain du magasin Docker Mac a été copiée localement. Aucun journal, témoin ou stockage de signature copié. Source/readers/profil/engagements v15 vérifiés ; aucune base exportée vers le VPS. La copie et les originaux restent distincts.

Le transport, l’horloge runtime et les refus WAN ne sont pas qualifiés par ce test. Il isole un défaut d’intégration réel ; il ne prouve pas que sa correction suffira à rétablir la finalité commune. Les échecs v15 et tous les critères de production restent ouverts.
''')
note='\n\n2026-10-06 — Nouveau contre-exemple réel d’admission (0a605778/core 5787915e) : ticket completed/rejected pour le bloc retenu 10751, pourtant admis après 21 continuations explicites supplémentaires. Préparation différée confondue avec rejet dans le vrai worker de reçus ; correction pending/cancellation/budgets/fairness requise. Magasins originaux inchangés, copie locale de chain seulement ; cause WAN globale non prouvée. [Preuves](qualification/block-receipt-deferral-baseline-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
(out/'manifest.json').write_text(json.dumps({str(f.relative_to(out)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(out.rglob('*')) if f.is_file()},indent=2)+'\n');print('Real worker rejection counterexample archived; original files unchanged.')
