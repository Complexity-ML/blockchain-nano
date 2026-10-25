#pragma once
#include "capella_stream_public_corpus.hpp"
namespace nano::test::canonical_stream_fixture {
inline PublicCorpusPin complete_public_corpus_pin(){return {fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-canonical-stream-public-v1/complete/manifest.json","1abc347450dcb50b8c3d0f9ffa4327bf351ec202a35e9ea974e98a0d760c5dfd",196};}
inline PublicCorpusPin stalled_public_corpus_pin(){return {fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-canonical-stream-public-v1/stalled/manifest.json","fecfc2fdf55bf8b486d8f1751c81094665a35772f82c6e5258f7eefacbdaf63c",194};}
}
