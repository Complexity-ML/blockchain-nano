#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_deposit_transition.hpp"
using namespace test;
using namespace test::phase0_minimal;
namespace {
Phase0DepositState state(const Bytes& b){Phase0DepositState s{};s.registry=decode(b);std::copy_n(b.begin()+4276,32,s.deposit_root.begin());s.deposit_count=little(b,4308,8);s.next_deposit_index=little(b,4352,8);return s;}
Phase0Deposit deposit(const Bytes& b){check(b.size()==1240,"deposit SSZ size");Phase0Deposit d;for(size_t i=0;i<33;++i)std::copy_n(b.begin()+32*i,32,d.proof[i].begin());std::copy_n(b.begin()+1056,48,d.message.public_key.begin());std::copy_n(b.begin()+1104,32,d.message.withdrawal_credentials.begin());d.message.amount_gwei=little(b,1136,8);std::copy_n(b.begin()+1144,96,d.signature.begin());return d;}
}
int main(int argc,char** argv){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";const fs::path root=argc==2?fs::path(argv[1]):fixtures/"consensus-deposit-official";
 const auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");const auto refs=fixtures/"consensus-slashing-official";const auto ref_manifest=load(refs/"manifest.json");for(const auto& [name,info]:ref_manifest.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml"),config=read(refs/"minimal-config.yaml");auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 ConsensusBytes4 version{};std::istringstream lines(config);std::string line;bool found=false;while(std::getline(lines,line))if(line.starts_with("GENESIS_FORK_VERSION:")){std::string value;std::istringstream(line.substr(line.find(':')+1))>>value;auto raw=unhex(value);check(raw.size()==4,"genesis version width");std::copy(raw.begin(),raw.end(),version.begin());found=true;}check(found,"missing genesis fork version");
 const Phase0DepositRules rules{p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),p("VALIDATOR_REGISTRY_LIMIT"),version};
 const Phase0SlashingRules registry_rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("MIN_SLASHING_PENALTY_QUOTIENT"),p("WHISTLEBLOWER_REWARD_QUOTIENT"),p("PROPOSER_REWARD_QUOTIENT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("PROPORTIONAL_SLASHING_MULTIPLIER")};
 unsigned valid=0,invalid=0,admitted=0,topups=0,ignored=0,snapshots=0,replays=0,mutations=0,overflows=0,invalid_snapshots=0;size_t records=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate fixture");for(const auto& [path,info]:row.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  auto actual=state(binary(root/name/"pre.ssz"));const auto initial=actual;const auto d=deposit(binary(root/name/"deposit.ssz"));const auto outcome=process_phase0_deposit(actual,rules,d);
  if(row.at("valid").get<bool>()){
   check(bool(outcome),"valid deposit rejected: "+name);const auto expected=state(binary(root/name/"post.ssz"));check(actual==expected,"official deposit transition mismatch: "+name);records+=actual.registry.validators.size();++valid;
   if(*outcome==Phase0DepositOutcome::Admitted){++admitted;const auto& added=actual.registry.validators.rbegin()->second;check(added.activation_epoch==UINT64_MAX&&added.activation_eligibility_epoch==UINT64_MAX,"deposit prematurely activated validator");}
   else if(*outcome==Phase0DepositOutcome::ToppedUp)++topups;else ++ignored;
   const auto encoded=phase0_deposit_snapshot(actual,rules,registry_rules);auto recovered=phase0_deposit_from_snapshot(Json::parse(encoded.dump()),rules,registry_rules);check(recovered==actual,"deposit snapshot lost cursor or credentials");++snapshots;
   if(name=="new_deposit_max"){
    auto reject_snapshot=[&](const Json& bad){bool threw=false;try{phase0_deposit_from_snapshot(bad,rules,registry_rules);}catch(const std::exception&){threw=true;}check(threw,"malformed deposit snapshot accepted");++invalid_snapshots;};
    auto bad=encoded;bad["format"]="unknown";reject_snapshot(bad);bad=encoded;bad["extra"]=1;reject_snapshot(bad);bad=encoded;bad.erase("depositRoot");reject_snapshot(bad);bad=encoded;bad["depositRoot"]="0x01";reject_snapshot(bad);
    bad=encoded;bad["nextDepositIndex"]="0x01";reject_snapshot(bad);bad=encoded;bad["nextDepositIndex"]=quantity(Int(std::to_string(actual.deposit_count))+1);reject_snapshot(bad);bad=encoded;bad["depositCount"]="0xffffffffffffffff";reject_snapshot(bad);bad=encoded;bad["nextDepositIndex"]=-1;reject_snapshot(bad);
    bad=encoded;bad["rules"][3]="0xffffffff";reject_snapshot(bad);bad=encoded;bad["registry"]["format"]="nano-phase0-slashing-v2";reject_snapshot(bad);bad=encoded;bad["registry"]["validators"][0]["withdrawalCredentials"]="0x00";reject_snapshot(bad);bad=encoded;bad["registry"]["validators"][0]["activationEligibilityEpoch"]="0x00";reject_snapshot(bad);
   }
   check(!process_phase0_deposit(recovered,rules,d)&&recovered==actual,"consumed deposit position replayed");++replays;
   auto refuse=[&](const Phase0Deposit& bad){auto copy=initial;check(!process_phase0_deposit(copy,rules,bad)&&copy==initial,"invalid branch changed state");++mutations;};
   auto bad=d;bad.proof[0][0]^=1;refuse(bad);bad=d;bad.proof[32][0]^=1;refuse(bad);bad=d;bad.message.amount_gwei^=1;refuse(bad);
   if(*outcome==Phase0DepositOutcome::ToppedUp){auto copy=initial;for(auto& [i,v]:copy.registry.validators)if(v.public_key==d.message.public_key)v.balance=UINT64_MAX;const auto before=copy;bool threw=false;try{process_phase0_deposit(copy,rules,d);}catch(const std::overflow_error&){threw=true;}check(threw&&copy==before,"topup overflow consumed index or balance");++overflows;}
   if(*outcome==Phase0DepositOutcome::Admitted){auto limited=rules;limited.validator_registry_limit=initial.registry.validators.size();auto copy=initial;bool threw=false;try{process_phase0_deposit(copy,limited,d);}catch(const std::overflow_error&){threw=true;}check(threw&&copy==initial,"registry bound failure consumed deposit");++overflows;}
  }else{check(!outcome&&actual==initial,"invalid official deposit accepted: "+name);++invalid;}
 }
 check(names.size()==17&&valid==15&&invalid==2&&invalid_snapshots==12,"deposit official coverage");
 std::cout<<Json{{"scenario","official-phase0-deposits"},{"valid",valid},{"invalid",invalid},{"admitted",admitted},{"topups",topups},{"ignoredInvalidSignature",ignored},{"validatorsCompared",records},{"snapshotRoundtrips",snapshots},{"invalidSnapshots",invalid_snapshots},{"consumedPositionRejections",replays},{"branchMutationsRejected",mutations},{"atomicOverflowRejections",overflows},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
