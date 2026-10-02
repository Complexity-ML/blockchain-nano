set -eu
out=/scratch/sha-context-direct-20261002
c++ -std=c++20 -O2 -pthread -I/work/include /control/faults.cpp /work/src/consensus_signing.cpp -lcrypto -Wl,--wrap=EVP_MD_CTX_new,--wrap=EVP_MD_CTX_free,--wrap=EVP_DigestInit_ex2,--wrap=EVP_DigestUpdate,--wrap=EVP_DigestFinal_ex -o "$out/fault-tests"
"$out/fault-tests"
sha256sum /control/faults.cpp /work/src/consensus_signing.cpp "$out/fault-tests"
