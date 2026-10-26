#!/bin/sh
# Collect firmware images from <build>/<board>/ into <build>/dist/ and write SHA256SUMS.
#   sh scripts/release/dist.sh [build-dir]
set -eu

build=${1:-build}
dist="$build/dist"

rm -rf "$dist"
mkdir -p "$dist"

count=0
for f in "$build"/*/jsbedded-*.uf2 "$build"/*/jsbedded-*.bin; do
    [ -f "$f" ] || continue
    cp "$f" "$dist/"
    count=$((count + 1))
done

if [ "$count" -eq 0 ]; then
    echo "dist: no firmware images under $build/ (run make firmware-all first)" >&2
    exit 1
fi

cd "$dist"
if command -v sha256sum >/dev/null 2>&1; then
    sha256sum jsbedded-* > SHA256SUMS
else
    shasum -a 256 jsbedded-* > SHA256SUMS
fi
echo "dist: $count images -> $dist/"
