#pragma once
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <sys/types.h>
#ifdef __linux__
inline constexpr bool supervisor_allows_init=true;
#else
inline constexpr bool supervisor_allows_init=false;
#endif
inline pid_t managed_supervisor(std::string_view value,pid_t parent,bool allow_init=supervisor_allows_init){
 unsigned long long parsed=0;auto result=std::from_chars(value.data(),value.data()+value.size(),parsed);
 if(value.empty()||result.ec!=std::errc{}||result.ptr!=value.data()+value.size()||parsed<(allow_init?1u:2u)||parsed>static_cast<unsigned long long>(std::numeric_limits<pid_t>::max()))throw std::invalid_argument("Invalid supervisor PID");
 const auto pid=static_cast<pid_t>(parsed);if(parent!=pid)throw std::runtime_error("Supervisor exited before relay startup");return pid;
}
