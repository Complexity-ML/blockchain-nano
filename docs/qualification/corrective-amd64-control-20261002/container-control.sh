set -euo pipefail
build=/build/corrective-1ac57e25-20261002
cat > /reports/manifest-tool.cmake <<'NANO_MANIFEST_TOOL_CMAKE'
if(NOT DEFINED REGISTRY OR NOT DEFINED OUTPUT OR NOT DEFINED NETWORK_EXECUTABLE)
  message(FATAL_ERROR "REGISTRY, OUTPUT and NETWORK_EXECUTABLE required")
endif()
find_program(FILE_TEST_PROGRAM NAMES test REQUIRED)
file(READ "${REGISTRY}" registry)
string(JSON count LENGTH "${registry}" tests)
if(count LESS 1)
  message(FATAL_ERROR "Empty CTest registry")
endif()
set(paths "${NETWORK_EXECUTABLE}")
if(DEFINED NODE_EXECUTABLE)
  list(APPEND paths "${NODE_EXECUTABLE}")
endif()
math(EXPR last "${count}-1")
foreach(i RANGE 0 ${last})
  string(JSON path GET "${registry}" tests ${i} command 0)
  list(APPEND paths "${path}")
  # Some tests launch an external helper supplied as an absolute argument.
  # Hash executable files too; data files and directories are not programs.
  string(JSON arguments LENGTH "${registry}" tests ${i} command)
  if(arguments GREATER 1)
    math(EXPR final_argument "${arguments}-1")
    foreach(j RANGE 1 ${final_argument})
      string(JSON argument GET "${registry}" tests ${i} command ${j})
      if(IS_ABSOLUTE "${argument}")
        get_filename_component(name "${argument}" NAME)
        if(name STREQUAL "nano-altair-probe")
          # This required helper must fail the same checks as command[0].
          list(APPEND paths "${argument}")
        elseif(EXISTS "${argument}" AND NOT IS_DIRECTORY "${argument}")
          execute_process(COMMAND "${FILE_TEST_PROGRAM}" -x "${argument}" RESULT_VARIABLE executable)
          if(executable EQUAL 0)
            # A fixture may also have its executable permission bit set.
            # Recognize program/script headers rather than treating that bit
            # alone as evidence that a data argument is a helper program.
            file(READ "${argument}" header LIMIT 4 HEX)
            if(header MATCHES "^(7f454c46|2321|feedface|cefaedfe|feedfacf|cffaedfe|cafebabe|bebafeca|cafebabf|bfbafeca|4d5a)")
              list(APPEND paths "${argument}")
            endif()
          endif()
        endif()
      endif()
    endforeach()
  endif()
endforeach()
list(REMOVE_DUPLICATES paths)
list(SORT paths)
set(manifest "{}")
foreach(path IN LISTS paths)
  if(NOT EXISTS "${path}" OR IS_DIRECTORY "${path}")
    message(FATAL_ERROR "Missing executable: ${path}")
  endif()
  execute_process(COMMAND "${FILE_TEST_PROGRAM}" -x "${path}" RESULT_VARIABLE status)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "Non-executable CTest command: ${path}")
  endif()
  file(SHA256 "${path}" digest)
  string(JSON manifest SET "${manifest}" "${path}" "\"${digest}\"")
endforeach()
file(WRITE "${OUTPUT}" "${manifest}\n")
NANO_MANIFEST_TOOL_CMAKE
printf '%s\n' daab685d6c1485bc89143f1edd4208dd2bf41686 > /reports/manifest-tool-revision.txt
sha256sum /reports/manifest-tool.cmake > /reports/manifest-tool-sha256.txt
mkdir -p "$build/qualification-scratch"
export TMPDIR="$build/qualification-scratch"
selection='^(core|rpc_load_window|consensus_bellatrix_clock_coalescing|consensus_bellatrix_voluntary_exit(_pruning|_network|_reorg)?|bft_rpc_cost(_sustained)?_(paris|shanghai)_network|rpc_cost_(paris|shanghai)|rpc_cost_(success|nested_[14])_(paris|shanghai))$'
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 --target nano nano-tests nano-bellatrix-transition-tests nano-rpc-load-window-tests nano-bft-network-tests nano-rpc-cost-tests > /reports/build.log 2>&1
ctest --test-dir "$build" --show-only=json-v1 -R "$selection" > /reports/registry.json
cat > /reports/check-selection.cmake <<'CMAKE'
file(READ /reports/registry.json registry)
string(JSON count LENGTH "${registry}" tests)
if(NOT count EQUAL 19)
 message(FATAL_ERROR "Expected exactly 19 corrective scenarios, got ${count}")
endif()
set(expected core rpc_load_window consensus_bellatrix_clock_coalescing
 consensus_bellatrix_voluntary_exit consensus_bellatrix_voluntary_exit_pruning
 consensus_bellatrix_voluntary_exit_network consensus_bellatrix_voluntary_exit_reorg
 bft_rpc_cost_paris_network bft_rpc_cost_sustained_paris_network
 bft_rpc_cost_shanghai_network bft_rpc_cost_sustained_shanghai_network
 rpc_cost_paris rpc_cost_success_paris rpc_cost_nested_1_paris rpc_cost_nested_4_paris
 rpc_cost_shanghai rpc_cost_success_shanghai rpc_cost_nested_1_shanghai rpc_cost_nested_4_shanghai)
foreach(i RANGE 0 18)
 string(JSON name GET "${registry}" tests ${i} name)
 list(APPEND names "${name}")
endforeach()
list(SORT expected)
list(SORT names)
if(NOT "${names}" STREQUAL "${expected}")
 message(FATAL_ERROR "Corrective selection contains missing, duplicate or foreign scenarios")
endif()
CMAKE
cmake -P /reports/check-selection.cmake
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /reports/manifest-tool.cmake
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 -R "$selection" --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
test_exit=$?
printf '%s\n' "$test_exit" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /reports/manifest-tool.cmake
manifest_exit=$?
printf '%s\n' "$manifest_exit" > /reports/post-test-manifest-exit.txt
if (( manifest_exit != 0 )); then exit 1; fi
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
exit "$test_exit"
