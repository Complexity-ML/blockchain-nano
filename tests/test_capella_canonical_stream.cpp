#include "support/capella_canonical_stream_self_test.hpp"
#include <string_view>
int main(int argc,char** argv){
 if(argc==1)return nano::test::lifecycle::fresh_stream_self_test();
 if(argc==2&&std::string_view(argv[1])=="--generate-fixtures")return nano::test::lifecycle::fresh_stream_self_test(true);
 std::cerr<<"usage: nano-capella-canonical-stream-tests [--generate-fixtures]\n";return 2;
}
