# Beacon cancellation phase diagnostics — ARM64, 2026-10-01

Adds a fixed phase name and total elapsed milliseconds to cancellation responses. Phase identifies where cancellation was observed; elapsedMs is total read duration, not exclusive phase cost. The underlying private exception is not published. Existing two-second HTTP execution budget and all admission/state/SSZ checks remain unchanged.

Docker ARM64 exit 0: publication/commitment cancellation, unchanged durable checkpoint, existing concurrent admission case, real HTTP budget expiry and phase response, privacy and queue-capacity reuse. Observer has seven passing cases, including two new phase/duration validation cases. No amd64 or WAN qualification yet. K and previous failures remain unresolved; this is measurement infrastructure, not a demonstrated performance fix.
