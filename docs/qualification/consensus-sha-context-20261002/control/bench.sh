set -eu
out=/scratch/sha-context-direct-20261002
c++ -std=c++20 -O2 -pthread -I/work/include /control/bench.cpp /baseline/src/consensus_signing.cpp -lcrypto -o "$out/baseline-bench"
c++ -std=c++20 -O2 -pthread -I/work/include /control/bench.cpp /work/src/consensus_signing.cpp -lcrypto -o "$out/candidate-bench"
sha256sum /work/src/consensus_signing.cpp /work/tests/test_consensus_hash_provider.cpp /baseline/src/consensus_signing.cpp "$out"/hash-tests "$out"/*-bench > /reports/inputs.sha256
openssl version -a > /reports/openssl.txt
for pass in 1 2; do
 if test "$pass" = 1;then order='baseline candidate';else order='candidate baseline';fi
 for variant in $order;do "$out/$variant-bench" > "/reports/$variant-$pass.txt";done
done
