#pragma once
#include "altair_message.hpp"
#include <atomic>
#include <memory>

// Transport-independent dispatch contract, shared by stream and durable service.
struct AltairWork {
 // deferred is internal to the worker: never delivered as a completed receipt.
 enum class Decision {accepted,rejected,local_failure,deferred};
 using Cancellation=std::shared_ptr<std::atomic<bool>>;
 using Done=std::function<void(Decision)>;
 using Dispatch=std::function<bool(AltairWireMessage,Cancellation,Done)>;
};
