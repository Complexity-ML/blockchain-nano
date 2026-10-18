# Package already qualified binaries; no compiler or dependency download.
ARG NANO_RUNTIME_BASE=nano-candidate-base:required
FROM ${NANO_RUNTIME_BASE} AS node
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
RUN chmod 0755 /usr/local/bin/nano /usr/local/bin/nano-p2p \
 && printf '%s  /usr/local/bin/nano\n%s  /usr/local/bin/nano-p2p\n' "$NANO_SHA256" "$P2P_SHA256" | sha256sum -c - \
 && ldd /usr/local/bin/nano > /tmp/nano-candidate.libs \
 && ldd /usr/local/bin/nano-p2p >> /tmp/nano-candidate.libs \
 && ! grep -q 'not found' /tmp/nano-candidate.libs \
 && rm /tmp/nano-candidate.libs \
 && /usr/local/bin/nano --help
USER 10001:10001
ENTRYPOINT ["/usr/local/bin/nano"]
# Require an explicit, reviewed network configuration before running a node.
CMD ["--help"]

FROM node AS complete
USER root
ARG STORAGE_MAINTENANCE_SHA256
ARG BOOTSTRAP_ARCHIVE_MAINTENANCE_SHA256
ARG CAPELLA_DUTY_READINESS_SHA256
COPY nano-storage-maintenance nano-bootstrap-archive-maintenance nano-capella-duty-readiness /usr/local/bin/
RUN chmod 0755 /usr/local/bin/nano-storage-maintenance /usr/local/bin/nano-bootstrap-archive-maintenance /usr/local/bin/nano-capella-duty-readiness \
 && printf '%s  /usr/local/bin/nano-storage-maintenance\n%s  /usr/local/bin/nano-bootstrap-archive-maintenance\n%s  /usr/local/bin/nano-capella-duty-readiness\n' "$STORAGE_MAINTENANCE_SHA256" "$BOOTSTRAP_ARCHIVE_MAINTENANCE_SHA256" "$CAPELLA_DUTY_READINESS_SHA256" | sha256sum -c - \
 && ldd /usr/local/bin/nano-storage-maintenance > /tmp/nano-candidate-complete.libs \
 && ldd /usr/local/bin/nano-bootstrap-archive-maintenance >> /tmp/nano-candidate-complete.libs \
 && ldd /usr/local/bin/nano-capella-duty-readiness >> /tmp/nano-candidate-complete.libs \
 && ! grep -q 'not found' /tmp/nano-candidate-complete.libs \
 && rm /tmp/nano-candidate-complete.libs
USER 10001:10001
