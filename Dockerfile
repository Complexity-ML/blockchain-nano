# syntax=docker/dockerfile:1
FROM ubuntu:24.04 AS dependencies
ARG JOBS=2
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential clang cmake git curl ca-certificates pkg-config \
    libssl-dev libicu-dev libboost-dev librocksdb-dev libgmp-dev \
    autoconf automake libtool && rm -rf /var/lib/apt/lists/*
ENV CC=clang CXX=clang++
WORKDIR /src
COPY cmake/ cmake/
COPY scripts/build-libp2p.sh scripts/build-libp2p.sh
RUN JOBS=${JOBS} sh scripts/build-libp2p.sh

FROM dependencies AS builder
ARG JOBS=2
COPY . .
RUN cmake -S network -B build-network -DCMAKE_BUILD_TYPE=Release \
      -DNANO_DEPS_DIR=/src/.deps \
 && cmake --build build-network --target nano-p2p --parallel ${JOBS} \
 && cmake -S . -B build-container -DCMAKE_BUILD_TYPE=Release \
      -DNANO_NETWORK_EXECUTABLE=/src/build-network/nano-p2p \
 && cmake --build build-container --target nano nano-storage-maintenance \
      nano-bootstrap-archive-maintenance nano-capella-duty-readiness --parallel ${JOBS} \
 && cmake --install build-container --prefix /out

FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libssl3t64 libicu74 librocksdb8.9 libgmp10 libgmpxx4ldbl \
 && rm -rf /var/lib/apt/lists/* \
 && groupadd --gid 10001 nano \
 && useradd --uid 10001 --gid nano --no-create-home --shell /usr/sbin/nologin nano \
 && mkdir -p /var/lib/nano /etc/nano \
 && chown nano:nano /var/lib/nano
COPY --from=builder /out/ /usr/local/
COPY deploy/docker/node.json /etc/nano/node.json
# Fail the build if any installed executable has an unresolved shared library.
RUN (cd /usr/local/share/nano/licenses && sha256sum -c notices.sha256) \
 && for name in nano nano-p2p nano-storage-maintenance nano-bootstrap-archive-maintenance nano-capella-duty-readiness; do \
      test -x "/usr/local/bin/$name" \
      && ldd "/usr/local/bin/$name" > /tmp/nano-runtime.libs \
      && ! grep -q 'not found' /tmp/nano-runtime.libs \
      || exit 1; \
    done \
 && rm /tmp/nano-runtime.libs \
 && nano --help
USER 10001:10001
WORKDIR /var/lib/nano
VOLUME ["/var/lib/nano"]
EXPOSE 8545/tcp 30303/tcp
ENTRYPOINT ["/usr/local/bin/nano"]
CMD ["run", "/usr/local/share/nano/genesis/testnet.json", "/var/lib/nano/chain", "/etc/nano/node.json"]
