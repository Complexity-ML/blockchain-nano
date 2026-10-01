# AMD64 Docker historical export

Git pull received source 5bd6f959bf3c773269d59eeda6792db6c450324e before compiling. Existing dependency image and build volume, no network, 2 CPUs / 3 GiB, nofile 65536. Historical export passes in 116.84 s, signed transition in 42.52 s; block-delivery and head-catalog models pass.

One runtime image assembled: sha256:00d268d1ce13693ae22b557ed1e9c1553a1a06856281289c0682dfd61f660f44. Initial malformed revision marker refused before any image build; corrected, refusal retained. Identity/user/architecture/binary hashes verified. No new WAN recovery qualification and no production claim. Real two-daemon regression was run on ARM64; AMD64 real transport will be exercised by the new WAN campaign.
