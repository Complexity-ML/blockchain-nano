from pathlib import Path
import shutil,json,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-catalog-termination-integrated-20261006');root.mkdir(exist_ok=False);shutil.copytree(repo/'network',root/'network')
(root/'source-manifest.json').write_text(json.dumps({str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((root/'network').rglob('*')) if p.is_file()},indent=2)+'\n')
for arch in ['arm64','amd64']:
 s=Path('/private/tmp/nano-history-first-20261006/cmake-targets.sh').read_text().replace('history-first-cmake-targets-arm64','catalog-termination-cmake-targets-'+arch).replace('coherent-efbeb9bc-arm64','coherent-efbeb9bc-'+arch)
 s=s.replace('nano-capella-head-catalog-tests nano-capella-head-history-fairness-tests nano-capella-relay-schedule-tests','nano-capella-catalog-termination-tests nano-capella-catalog-relay-tests nano-capella-head-catalog-tests nano-capella-head-history-fairness-tests nano-p2p')
 s=s.replace('head_catalog|head_history_fairness|relay_schedule','catalog_termination|catalog_relay|head_catalog|head_history_fairness').replace('/output/cmake-', '/output/'+arch+'-cmake-')
 (root/(arch+'-cmake.sh')).write_text(s)
s=Path('/private/tmp/nano-build-history-first-native-20261006.py').read_text().replace('nano-history-first-20261006','nano-catalog-termination-integrated-20261006').replace("['capella_head_catalog_relay','capella_head_history_fairness','capella_relay_schedule','runtime']","['capella_catalog_termination','capella_catalog_relay','capella_head_catalog_relay','capella_head_history_fairness','runtime']")
(root/'native.py').write_text(s)
print(root)
