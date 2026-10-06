set -eu
clang++ -O2 -std=c++20 /probe/parent/test.cpp -o /probe/parent-test
set +e
/probe/parent-test > /probe/parent-result.log 2>&1
code=$?
set -e
test "$code" = 1
cat /probe/parent-result.log
clang++ -O2 -std=c++20 -DNANO_TEST_PREPARATION_PRIORITY /probe/test.cpp -o /probe/test
/probe/test
clang++ -O2 -std=c++20 /probe/normal.cpp -o /probe/normal
/probe/normal
