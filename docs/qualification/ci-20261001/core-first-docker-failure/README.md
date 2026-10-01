# First Docker core CI — failure preserved

GitHub run 36886531310, source 43e449ddd3479bfd316adc8252e71434f3000900, terminal completed/failure. Container dependency install and checkout completed. The core script failed before CMake at `git rev-parse HEAD`: checkout ownership inside the root container was considered dubious by Git. No C++ build/test ran, and no success is claimed. Operations job independently passed.

Correction scopes the Git ownership exception to the current checkout for that single revision query, not a global wildcard. Container dependencies also explicitly include build-essential (compiler/make toolchain), which the original install list omitted; this omission was not the observed failure. Requalification required.

Earlier non-Docker dispatch 36886165867 is terminal cancelled, core install cancelled and core.sh skipped; its API state is retained separately.
