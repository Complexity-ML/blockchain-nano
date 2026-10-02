#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
namespace fs=std::filesystem;
void info(const fs::path& p){struct stat s{};if(stat(p.c_str(),&s)==0)std::cout<<p<<" mode="<<std::oct<<(s.st_mode&0777)<<std::dec<<" uid="<<s.st_uid<<" gid="<<s.st_gid<<'\n';}
int main(int argc,char** argv){fs::path p=argv[1];fs::create_directories(p/"source");std::ofstream(p/"source"/"fixture")<<"fixture";info(p/"source");info(p/"source"/"fixture");try{fs::copy(p/"source",p/"copy",fs::copy_options::recursive);info(p/"copy");std::cout<<"COPY_PASS\n";}catch(const std::exception& e){info(p/"copy");std::cout<<"COPY_FAIL "<<e.what()<<'\n';return 1;}}
