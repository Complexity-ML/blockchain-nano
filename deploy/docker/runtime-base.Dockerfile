# Standalone runtime dependencies: no compiler, source build, or node startup.
# Ubuntu OCI index resolved on 2026-10-05; dependency versions are recorded below.
FROM ubuntu:24.04@sha256:534baea6a22c03a63003dbc8dbe78fe34bc0d7e595d9a9dc9834884ff530eb55
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libssl3t64 libicu74 librocksdb8.9 libgmp10 libgmpxx4ldbl \
 && mkdir -p /usr/local/share/nano /var/lib/nano /etc/nano \
 && dpkg-query -W > /usr/local/share/nano/runtime-packages.tsv \
 && rm -rf /var/lib/apt/lists/* \
 && groupadd --gid 10001 nano \
 && useradd --uid 10001 --gid nano --no-create-home --shell /usr/sbin/nologin nano \
 && chown nano:nano /var/lib/nano
USER 10001:10001
WORKDIR /var/lib/nano
# No anonymous data volume: callers select an existing, reviewed volume explicitly.
CMD ["/bin/true"]
