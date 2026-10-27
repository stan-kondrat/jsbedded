#!/bin/sh
# Print the CI build matrix as JSON: every board (firmware/boards/*/board.mk) ×
# every engine (firmware/engines/*/engine.cmake), minus the boards an engine lists
# in "unsupported_boards" in its engine.json. Needs jq.
#   sh scripts/matrix/ci-matrix.sh
set -eu

fw=firmware
rows=""

for board_mk in "$fw"/boards/*/board.mk; do
    board=$(basename "$(dirname "$board_mk")")
    platform=$(sed -n 's/^PLATFORM *:= *//p' "$board_mk")
    for engine_cmake in "$fw"/engines/*/engine.cmake; do
        engine_dir=$(dirname "$engine_cmake")
        engine=$(basename "$engine_dir")
        json="$engine_dir/engine.json"
        if jq -e --arg b "$board" '(.unsupported_boards // []) | index($b)' "$json" >/dev/null; then
            continue
        fi
        submodules=$(jq -r '(.submodules // []) | join(" ")' "$json")
        rows="$rows$(jq -cn --arg board "$board" --arg engine "$engine" \
            --arg platform "$platform" --arg submodules "$submodules" \
            '{board: $board, engine: $engine, platform: $platform, submodules: $submodules}')
"
    done
done

printf '%s' "$rows" | jq -cs '{include: .}'
