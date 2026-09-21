.DEFAULT_GOAL := all
CXX ?= c++
PREFIX ?= /opt/homebrew
ICU_PREFIX ?= $(PREFIX)/opt/icu4c
CPPFLAGS += -isystem $(ICU_PREFIX)/include
CPPFLAGS += -isystem third_party/mcl/include
CPPFLAGS += -Iinclude -Ithird_party -isystem $(PREFIX)/include
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++20 -Wall -Wextra -Wpedantic
LDFLAGS += -L$(PREFIX)/lib -Wl,-rpath,$(PREFIX)/lib
LDFLAGS += -L$(ICU_PREFIX)/lib -Wl,-rpath,$(ICU_PREFIX)/lib
LDLIBS += -licuuc -lgmpxx -lgmp -lssl -lcrypto -lrocksdb -pthread
SOURCES := $(wildcard src/*.cpp)
LIBSOURCES := $(filter-out src/main.cpp,$(SOURCES))
OBJECTS := $(patsubst src/%.cpp,build/%.o,$(LIBSOURCES))
MCL_ARCHIVE := build-mcl/lib/libmcl.a
OBJECTS += $(MCL_ARCHIVE)
CMAKE ?= cmake
$(MCL_ARCHIVE):
	$(CMAKE) -S third_party/mcl -B build-mcl -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON -DMCL_BUILD_TESTING=OFF -DMCL_BUILD_SAMPLE=OFF
	$(CMAKE) --build build-mcl --target mcl_st -j 4
.PHONY: all test clean
all: build/nano build/nano-tests build/nano-chain-tests build/nano-rpc-tests build/nano-http-tests build/nano-event-tests build/nano-validator-tests build/nano-migration-tests build/nano-uncle-tests build/nano-staking-tests build/nano-miner-tests build/nano-block-auth-tests
build:
	mkdir -p build
build/%.o: src/%.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
build/nano-tests: tests/test_core.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-chain-tests: tests/test_chain.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano: src/main.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-rpc-tests: tests/test_rpc.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-http-tests: tests/test_http.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-event-tests: tests/test_events.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-block-auth-tests: tests/test_block_auth.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-miner-tests: tests/test_miner.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-staking-tests: tests/test_staking.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-uncle-tests: tests/test_uncles.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-migration-tests: tests/test_migration.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-validator-tests: tests/test_validator.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
build/nano-finality-tests: tests/test_finality.cpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
test: build/nano-finality-tests build/nano-tests build/nano-chain-tests build/nano-rpc-tests build/nano-http-tests build/nano-event-tests build/nano-validator-tests build/nano-migration-tests build/nano-uncle-tests build/nano-staking-tests build/nano-miner-tests build/nano-block-auth-tests
	./build/nano-finality-tests
	./build/nano-tests
	./build/nano-chain-tests
	./build/nano-rpc-tests
	./build/nano-http-tests
	./build/nano-event-tests
	./build/nano-validator-tests
	./build/nano-migration-tests
	./build/nano-uncle-tests
	./build/nano-staking-tests
	./build/nano-miner-tests
	./build/nano-block-auth-tests
clean:
	rm -rf build
-include $(filter %.d,$(OBJECTS:.o=.d))
