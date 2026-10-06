#!/bin/bash
set -eu
arch=$1
(cd /overlay/source; sha256sum -c /drivers/source.sha256) > /drivers/$arch-source-before.log
bash /drivers/build.sh "$arch"
bash /drivers/runtime.sh "$arch"
bash /drivers/regression.sh "$arch"
(cd /overlay/source; sha256sum -c /drivers/source.sha256) > /drivers/$arch-source-after.log
