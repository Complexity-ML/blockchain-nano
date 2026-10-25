#include "support/capella_stream_public_corpus.hpp"

int main(int argc,char** argv){
 namespace f=nano::test::canonical_stream_fixture;
 try{
  f::check(argc==3,"usage: exporter --complete|--stalled NEW_ABSOLUTE_DIRECTORY");
  const std::string mode=argv[1];f::check(mode=="--complete"||mode=="--stalled","explicit single corpus mode required");
  const auto selected=mode=="--complete"?f::PublicCorpusMode::Complete:f::PublicCorpusMode::Stalled;
  f::test_stage="public-corpus-generation-and-export";
  const auto result=f::export_public_corpus(selected,std::filesystem::path(argv[2]));
  const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-f::started).count();
  std::cout<<nano::Json{{"scenario","fixed-public-stream-corpus-export"},{"passed",true},{"artifact",result},{"elapsedMs",elapsed},{"wholeDriverPeakRssKiB",f::public_peak_rss_kib()},{"generationPerformanceQualified",false},{"runtimeReplayQualified",false},{"newNetworkIdentities",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& error){
  std::cerr<<nano::Json{{"scenario","fixed-public-stream-corpus-export"},{"passed",false},{"reason",error.what()},{"testStage",f::test_stage},{"lastSyntheticSlot",f::test_slot},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-f::started).count()},{"wholeDriverPeakRssKiB",f::public_peak_rss_kib()},{"productionQualified",false}}.dump()<<std::endl;return 1;
 }
}
