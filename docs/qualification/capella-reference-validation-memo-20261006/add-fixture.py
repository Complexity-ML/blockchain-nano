from pathlib import Path
import json,hashlib
root=Path('/private/tmp/nano-reference-memo-20261006');source=root/'source';p=source/'tests/test_capella_transition.cpp';s=p.read_text();marker='     auto decoded=Json::parse(warm_manifest);'
insert='''     if(std::getenv("NANO_TEST_REFERENCE_VALIDATION_MEMO")){
      CapellaReferenceSnapshotCache memo;auto no_hot=[](const CapellaColdReference&,std::string_view){throw std::logic_error("memo unexpectedly staged cold row");};
      auto usage=[&](){const auto u=memo.usage();return Json::array({u.records,u.encoded_bytes,u.reuses,u.validated_cold_rows});};
      const auto certified=capella_store_reference_snapshot_cached(cold,rules,compact_limits,no_hot,memo);check(certified==warm_manifest,"memo initial manifest differs");const auto seeded=memo.usage();check(seeded.records==cold.blocks.size()&&seeded.validated_cold_rows==cold.blocks.size()&&seeded.reuses==0&&seeded.encoded_bytes<=compact_limits.envelope_bytes,"memo initial certificates wrong");
      check(capella_store_reference_snapshot_cached(cold,rules,compact_limits,no_hot,memo)==warm_manifest,"memo reuse changed canonical manifest");const auto reused=memo.usage();check(reused.reuses==cold.blocks.size()&&reused.validated_cold_rows==seeded.validated_cold_rows,"memo did not reuse cold certificates");
      auto reject=[&](const CapellaForkStore& graph,const CapellaStoreSnapshotLimits& limits){const auto before_memo=usage();bool refused=false;try{capella_store_reference_snapshot_cached(graph,rules,limits,no_hot,memo);}catch(const std::exception&){refused=true;}check(refused&&usage()==before_memo,"memo accepted mutation or retained failed-call updates");};
      auto smaller=compact_limits;smaller.state_bytes=maximum_state-1;reject(cold,smaller);
      auto bad=cold;ConsensusRoot child{};for(const auto& [id,_]:cold.blocks)if(id!=cold.timed.view.anchor)child=id;
      bad.timed.view.nodes.at(child).realized_justified_epoch+=1;reject(bad,compact_limits);
      bad=cold;bad.unrealized_justifications.at(child).epoch+=1;reject(bad,compact_limits);
      auto provider=[&](const Bytes& hash,size_t bytes)->std::optional<std::string>{const auto found=staged.find(hash);if(found==staged.end())return std::nullopt;check(found->second.size()<=bytes,"memo provider budget mismatch");return found->second;};
      auto fresh_cache=std::make_shared<CapellaColdRecordCache>(rules,crash_limits,2,2*maximum_row);
      bad=cold;const auto& original=cold.blocks.at(child);auto altered_execution=original.execution_header();altered_execution.timestamp+=1;bad.blocks.insert_or_assign(child,CapellaForkRecord(*original.cold_reference(),original.header(),altered_execution,fresh_cache,provider));reject(bad,compact_limits);
      bad=cold;auto altered_ref=*original.cold_reference();altered_ref.digest[0]^=1;bad.blocks.insert_or_assign(child,CapellaForkRecord(altered_ref,original.header(),original.execution_header(),fresh_cache,provider));reject(bad,compact_limits);
      const auto before_cancel=usage();bool cancelled_memo=false;try{capella_store_reference_snapshot_cached(cold,rules,compact_limits,no_hot,memo,[]{throw ExecutionCancelled("memo cancelled");});}catch(const ExecutionCancelled&){cancelled_memo=true;}check(cancelled_memo&&usage()==before_cancel,"memo early cancellation changed certificate state");
      auto changed_rules=rules;changed_rules.attestation_due_ms=rules.attestation_due_ms?rules.attestation_due_ms-1:1;const auto changed_oracle=capella_store_reference_snapshot(cold,changed_rules,compact_limits,no_hot);const auto before_profile=memo.usage();check(capella_store_reference_snapshot_cached(cold,changed_rules,compact_limits,no_hot,memo)==changed_oracle,"memo changed-profile oracle differs");check(memo.usage().validated_cold_rows==before_profile.validated_cold_rows+cold.blocks.size()&&memo.usage().reuses==before_profile.reuses,"memo reused certificates under changed profile");
      std::cout<<Json{{"scenario","capella-reference-validation-memo"},{"records",cold.blocks.size()},{"coldCertificateReuses",reused.reuses},{"canonicalManifestEqual",true},{"mutationRefusals",5},{"failedCallsLeaveMemoUnchanged",true},{"profileInvalidation",true},{"earlyCancellationUnchanged",true},{"heavyStateCached",false},{"runtimeIntegrated",false},{"durablePreflightSkipped",false},{"productionQualified",false}}.dump()<<std::endl;
     }
'''+marker
assert s.count(marker)==1;s=s.replace(marker,insert);p.write_text(s)
m={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()};(root/'source-sha256.json').write_text(json.dumps(m,indent=2)+'\n')
s=Path('/private/tmp/nano-cold-header-links-20261006/build.sh').read_text().replace('/build/cold-header-links-$arch-20261006','/build/reference-memo-$arch-20261006').replace('/build/cold-runtime-$arch-20261006/libnano_core.a','/build/cold-header-links-v2-$arch-20261006/libnano_core.a').replace('NANO_TEST_COMPACT_REFERENCE_RESTORE=1','NANO_TEST_REFERENCE_VALIDATION_MEMO=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1');(root/'build.sh').write_text(s)
print('Added writer-only certificate oracle and five corruption/context refusals')
