# Package already qualified binaries; no compiler or dependency download.
ARG NANO_RUNTIME_BASE=nano-candidate-base:required
FROM ${NANO_RUNTIME_BASE}
USER root
ARG SOURCE_REVISION
ARG RUNTIME_BASE_ID
ARG NANO_SHA256
ARG P2P_SHA256
LABEL org.opencontainers.image.revision=${SOURCE_REVISION}
LABEL org.nanobees.runtime-base-id=${RUNTIME_BASE_ID}
COPY nano nano-p2p /usr/local/bin/
COPY notices/ /usr/local/share/nano/licenses/
RUN cd /usr/local/share/nano/licenses && sha256sum -c notices.sha256
RUN printf '%s  /usr/local/bin/nano\n%s  /usr/local/bin/nano-p2p\n' "$NANO_SHA256" "$P2P_SHA256" | sha256sum -c - \
 && ldd /usr/local/bin/nano > /tmp/nano-candidate.libs \
 && ldd /usr/local/bin/nano-p2p >> /tmp/nano-candidate.libs \
 && ! grep -q 'not found' /tmp/nano-candidate.libs \
 && rm /tmp/nano-candidate.libs \
 && /usr/local/bin/nano --help
USER 10001:10001
ENTRYPOINT ["/usr/local/bin/nano"]
# Require an explicit, reviewed network configuration before running a node.
CMD ["--help"]
