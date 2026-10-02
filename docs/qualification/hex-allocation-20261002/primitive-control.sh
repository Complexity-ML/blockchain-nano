set -euo pipefail
clang++ -O3 -DNDEBUG -std=gnu++20 -I/work/include -I/work/third_party /work/src/types.cpp /work/tests/test_hex_encoding.cpp -o /scratch/hex-encoding-tests -lgmpxx -lgmp -lcrypto
sha256sum /scratch/hex-encoding-tests > /reports/program-before.sha256
/scratch/hex-encoding-tests --benchmark > /reports/results.json
sha256sum /scratch/hex-encoding-tests > /reports/program-after.sha256
cmp /reports/program-before.sha256 /reports/program-after.sha256
printf '0\n' > /reports/test-exit.txt
