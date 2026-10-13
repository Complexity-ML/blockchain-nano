#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
// Test-only caller. All DB handles are closed before this routine. Sources and
// signer/witness trees stay in the synthetic fixture; only PUBLIC proofs leave
// through stdout. The CLI is executed directly, with no shell or preload.
static void gc_cli_save_public(const fs::path& path,std::string_view bytes){
 int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);check(fd>=0,"GC CLI fresh public input");
 size_t at=0;try{while(at<bytes.size()){const auto n=::write(fd,bytes.data()+at,bytes.size()-at);if(n<0&&errno==EINTR)continue;check(n>0,"GC CLI public input write");at+=size_t(n);}const auto closed=::close(fd);fd=-1;check(closed==0,"GC CLI public input close");}catch(...){if(fd>=0)::close(fd);throw;}
}
static std::string gc_cli_read_public(const fs::path& path){
 check(fs::is_regular_file(path)&&!fs::is_symlink(path)&&fs::file_size(path)<=1024*1024,"GC CLI bounded receipt");std::ifstream input(path,std::ios::binary);std::string bytes;char chunk[4096];while(input){input.read(chunk,sizeof(chunk));bytes.append(chunk,size_t(input.gcount()));check(bytes.size()<=1024*1024,"GC CLI receipt grew");}check(!input.bad(),"GC CLI receipt read");return bytes;
}
static std::string gc_cli_sha(std::string_view bytes){return hex(sha256(Bytes(bytes.begin(),bytes.end()))).substr(2);}
static void gc_cli_collect_closed(const fs::path& directory,const std::string& request,const std::string& stopped,
 const std::string& profile,const CheckpointBlobGcBudget& budget,const ForkCheckpointToken& primary_pin,
 const ForkCheckpointToken& witness_pin,const Bytes& orphan,const std::vector<fs::path>& protected_trees){
 const auto* configured=std::getenv("NANO_TEST_CHECKPOINT_GC_TOOL");check(configured&&*configured,"GC CLI tool target path required");const fs::path supplied(configured);const auto tool=fs::canonical(supplied);check(supplied.is_absolute()&&supplied==tool&&fs::is_regular_file(tool)&&::access(tool.c_str(),X_OK)==0,"GC CLI canonical executable target required");
 const auto parent=directory/"gc-cli-public-inputs";check(fs::create_directory(parent)&&::chmod(parent.c_str(),0700)==0,"GC CLI fresh private input root");
 const auto request_file=parent/"request.json",stop_file=parent/"stopped.json",profile_file=parent/"profile.json",receipt_file=parent/"receipt.json";
 gc_cli_save_public(request_file,request);gc_cli_save_public(stop_file,stopped);gc_cli_save_public(profile_file,profile);
 const auto parsed=Json::parse(request);const fs::path primary=parsed.at("primary").at("path").get<std::string>(),witness=parsed.at("witness").at("path").get<std::string>();
 std::map<fs::path,std::string> retained;for(const auto& tree:protected_trees)retained.emplace(tree,checkpoint_blob_gc_copy_inventory(tree,budget.inventory_files,budget.inventory_bytes));
 const auto unchanged_protected=[&]{for(const auto& [tree,sha]:retained)check(checkpoint_blob_gc_copy_inventory(tree,budget.inventory_files,budget.inventory_bytes)==sha,"GC CLI changed protected source/journal/witness files");};
 const auto args=[&](const fs::path& req,const std::string& req_sha,const fs::path& stop,const std::string& stop_sha,const std::string& profile_sha,const fs::path& receipt){return std::vector<std::string>{tool.string(),"--checkpoint-blob-gc-owned-copies",req.string(),req_sha,stop.string(),stop_sha,profile_file.string(),profile_sha,receipt.string()};};
 Json refusals=Json::array();
 const auto refuse=[&](std::vector<std::string> command,const fs::path& receipt,const char* id,const char* expected,const fs::path& p,const fs::path& w,const std::optional<std::string>& old_receipt=std::nullopt){
  const auto logical_primary=gc_case_logical(p),logical_witness=gc_case_logical(w);Process child(command);const auto result=child.wait(70);
  check(result.code==1&&result.err.find(expected)!=std::string::npos&&result.out.empty()&&(old_receipt?(fs::exists(receipt)&&gc_cli_read_public(receipt)==*old_receipt):!fs::exists(receipt)),std::string("GC CLI refusal failed at wrong boundary: ")+id+" "+result.err);
  check(gc_case_logical(p)==logical_primary&&gc_case_logical(w)==logical_witness,std::string("GC CLI refusal changed logical contents: ")+id);unchanged_protected();
  refusals.push_back({{"id",id},{"exitCode",result.code},{"newReceiptPublished",false},{"existingReceiptBytesPreserved",old_receipt.has_value()},{"logicalCopyContentsUnchanged",true},{"expectedDiagnostic",expected}});
 };
 {const auto receipt=parent/"bad-argc-receipt.json";auto command=args(request_file,gc_cli_sha(request),stop_file,gc_cli_sha(stopped),gc_cli_sha(profile),receipt);command.pop_back();refuse(command,receipt,"missing-receipt-argument","Usage: compact-owned-copy",primary,witness);}
 {const auto receipt=parent/"bad-request-pin-receipt.json";auto pin=gc_cli_sha(request);pin[0]=pin[0]=='a'?'b':'a';refuse(args(request_file,pin,stop_file,gc_cli_sha(stopped),gc_cli_sha(profile),receipt),receipt,"request-pin-mismatch","Public input SHA mismatch before parse",primary,witness);}
 {const auto receipt=parent/"bad-profile-pin-receipt.json";auto pin=gc_cli_sha(profile);pin[0]=pin[0]=='a'?'b':'a';refuse(args(request_file,gc_cli_sha(request),stop_file,gc_cli_sha(stopped),pin,receipt),receipt,"profile-pin-mismatch","Public input SHA mismatch before parse",primary,witness);}
 {const auto receipt=parent/"preexisting-public-receipt.json";const std::string sentinel="{\"testSentinel\":true}\n";gc_cli_save_public(receipt,sentinel);refuse(args(request_file,gc_cli_sha(request),stop_file,gc_cli_sha(stopped),gc_cli_sha(profile),receipt),receipt,"preexisting-receipt-no-overwrite","Fresh independent receipt path required",primary,witness,sentinel);}
 // A separate physical copy is mandatory: successful RW acquisition may rotate
 // LOG before the statistics refusal, so its old raw copy pin is not reused.
 {const auto stats_root=directory/"gc-cli-statistics-copy";check(fs::create_directory(stats_root)&&::chmod(stats_root.c_str(),0700)==0,"GC CLI new stats root");const auto p=stats_root/"primary",w=stats_root/"witness";
  const fs::path source_primary=parsed.at("primary").at("sourcePath").get<std::string>(),source_witness=parsed.at("witness").at("sourcePath").get<std::string>();fs::copy(source_primary,p,fs::copy_options::recursive);fs::copy(source_witness,w,fs::copy_options::recursive);check(::chmod(p.c_str(),0700)==0&&::chmod(w.c_str(),0700)==0,"GC CLI stats copy permissions");gc_cli_save_public(stats_root/".nano-owned-checkpoint-gc-copy","owned-offline-checkpoint-gc-v1\n");
  uint64_t maximum_files=0;for(const auto& path:{p,w}){uint64_t count=0;for(const auto& row:fs::recursive_directory_iterator(path))if(row.is_regular_file())++count;maximum_files=std::max(maximum_files,count);}check(maximum_files>0&&maximum_files<budget.inventory_files,"GC CLI finite stats fixture file count");
  // Closed-file inventory counts files; physical statistics also count entries.
  // New EMPTY directories deliberately cross only the latter boundary.
  for(uint64_t n=0;n<=maximum_files;++n){const auto empty=p/("stats-empty-"+std::to_string(n));check(fs::create_directory(empty)&&::chmod(empty.c_str(),0700)==0,"GC CLI empty stats directory");}
  auto stats_request=parsed;stats_request["primary"]["path"]=p.string();stats_request["witness"]["path"]=w.string();stats_request["budget"][0]=quantity(Int(std::to_string(maximum_files)));
  auto stats_stop=Json::parse(stopped);stats_stop["copies"][0]["path"]=p.string();stats_stop["copies"][1]["path"]=w.string();const auto stop_bytes=stats_stop.dump();stats_request["stopReceiptSha256"]=gc_cli_sha(stop_bytes);const auto request_bytes=stats_request.dump();const auto req=parent/"stats-request.json",stop=parent/"stats-stop.json",receipt=parent/"stats-refusal-receipt.json";gc_cli_save_public(req,request_bytes);gc_cli_save_public(stop,stop_bytes);
  refuse(args(req,gc_cli_sha(request_bytes),stop,gc_cli_sha(stop_bytes),gc_cli_sha(profile),receipt),receipt,"physical-statistics-entry-budget","File statistics entry budget exceeded",p,w);
 }
 unchanged_protected();Process child(args(request_file,gc_cli_sha(request),stop_file,gc_cli_sha(stopped),gc_cli_sha(profile),receipt_file));const auto result=child.wait(70);check(result.code==0&&result.err.empty(),"GC CLI positive failed: "+result.err);const auto terminal=Json::parse(result.out);
 const auto receipt_bytes=gc_cli_read_public(receipt_file);const auto report=Json::parse(receipt_bytes);check(terminal.at("completed")==true&&terminal.at("receiptSha256")==gc_cli_sha(receipt_bytes)&&terminal.at("productionQualified")==false,"GC CLI terminal/receipt digest mismatch");
 struct stat metadata{};check(::lstat(receipt_file.c_str(),&metadata)==0&&S_ISREG(metadata.st_mode)&&metadata.st_uid==::geteuid()&&(metadata.st_mode&0777)==0600&&metadata.st_nlink==1,"GC CLI receipt mode/owner/link invalid");
 check(report.at("format")=="nano-owned-checkpoint-gc-maintenance-receipt-v1"&&report.at("requestSha256")==gc_cli_sha(request)&&report.at("stopReceiptSha256")==gc_cli_sha(stopped)&&report.at("profileSha256")==gc_cli_sha(profile)&&report.at("requiresSuccessfulTerminalExit")==true&&report.at("receiptPersistedSeparately")==true&&report.at("atomicExclusiveLinkPublication")==true,"GC CLI public input/receipt binding mismatch");
 const auto& plan=report.at("plan");const auto& collected=report.at("result");check(plan.at("primary")==gc_token(primary_pin)&&plan.at("witness")==gc_token(witness_pin)&&plan.at("next")==gc_token(primary_pin)&&plan.at("deletePrimary").size()==1&&plan.at("deleteWitness").size()==1&&plan.at("deletePrimary")[0][1]==hex(orphan)&&plan.at("deleteWitness")[0][1]==hex(orphan),"GC CLI did not isolate actual staged orphan");
 check(collected.at("noWork")==false&&collected.at("stores").size()==2&&collected.at("stores")[0].at("deletedRecords")==1&&collected.at("stores")[1].at("deletedRecords")==1&&collected.at("allProtectedReferencesReopened")==true&&collected.at("nonGcNamespacesUnchanged")==true&&collected.at("originalSourceInventoriesUnchanged")==true&&collected.at("primary")==gc_token(primary_pin)&&collected.at("witness")==gc_token(witness_pin),"GC CLI deletion/reopen result mismatch");
 for(const auto& row:fs::directory_iterator(parent))check(row.path().filename().string().find(".gc-staged-")==std::string::npos,"GC CLI left temporary receipt link");unchanged_protected();
 // The root collector can retain these exact PUBLIC bytes and their hash before
 // Temp removes the synthetic fixture. No DB, key, ledger or env is exported.
 std::cout<<Json{{"scenario","checkpoint-gc-cli-closed-staged"},{"cliExitCode",result.code},{"actualCliCompleted",true},{"deletedPrimaryRecords",1},{"deletedWitnessRecords",1},{"noWork",false},{"refusals",refusals},{"receiptBytes",receipt_bytes},{"receiptSha256",gc_cli_sha(receipt_bytes)},{"receiptMode",0600},{"receiptLinks",metadata.st_nlink},{"sourcesAndSignerWitnessBytesUnchanged",true},{"syntheticOnly",true},{"naturalPruningReplay",false},{"privateFixtureExported",false},{"runtimeDutiesResumed",false},{"productionQualified",false}}.dump()<<std::endl;
}
