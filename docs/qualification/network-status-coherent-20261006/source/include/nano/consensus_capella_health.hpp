// Shared entry point for the daemon monitor and its qualification.
#pragma once
#include "nano/consensus_capella_validator.hpp"
#include <optional>
namespace nano {
class CapellaValidatorFailureAlert {
 bool emitted_=false;
public:
 // Caller owns/stabilizes validator lifetime. Never waits for the duty mutex,
 // accesses private failure strings, resets journals, or requests node shutdown.
 std::optional<Json> poll(const CapellaValidatorLoop* validator) {
  if(emitted_||!validator||validator->healthy())return std::nullopt;
  emitted_=true;
  return Json{{"event","capella-validator-unavailable"},
              {"nodeShutdownRequested",false}};
 }
};
}
