#pragma once
// Compatibility names only: tests call the same compiled Core implementation.
#include "nano/consensus_capella_canonical_stream.hpp"

namespace nano::test::lifecycle {
using ::nano::canonical_stream_index_record_bytes;
using ::nano::CanonicalStreamLimits;
using ::nano::CanonicalStreamSource;
using ::nano::CanonicalStreamBoundary;
using ::nano::CanonicalStreamVisit;
using ::nano::CanonicalStreamVisitor;
using ::nano::CanonicalStreamResult;
using ::nano::stream_capella_canonical_history;
}
#include "capella_canonical_stream.ipp"
