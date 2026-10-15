// ROOT-run test-only child. Refusal output never prints exception contents.
#include "../support/fresh_managed_submission.hpp"
#include "../support/fresh_managed_submission_self_test.hpp"
#include <iostream>
using namespace nano;
namespace m=nano::test::managed_submission;
int main(int argc,char** argv){try{if(argc==2&&std::string_view(argv[1])=="--self-test")return m::self_test();return m::child(argc,argv);}catch(...){std::cerr<<"{\"event\":\"fresh-lifecycle-child-submission-refused\",\"accepted\":false,\"detailsExported\":false,\"productionQualified\":false}\n";return 1;}}
