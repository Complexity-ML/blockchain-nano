
    test -d "$1" && test ! -L "$1" || exit 1
    for path in "$1/nano" "$1/production-source-revision.txt" /build/network/nano-p2p; do
      test -f "$path" && test ! -L "$path" || exit 1
    done
    exec tar -C "$1" -cf - nano production-source-revision.txt -C /build network/nano-p2p
  