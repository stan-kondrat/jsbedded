#!/bin/sh
# Print the CI build matrix as JSON. Needs jq.
#   sh scripts/matrix/ci-matrix.sh
#
# Rows: every board (firmware/boards/*/board.mk) × every engine (firmware/engines/*/engine.cmake),
# minus the boards an engine lists in "unsupported_boards" in its engine.json.
# Each row: board, engine, platform, submodules, variant (build folder and image name:
# <board>-<engine>).
set -eu

fw=firmware
rows=""

# add_row <board> <engine>
add_row() {
    board_mk="$fw/boards/$1/board.mk"
    json="$fw/engines/$2/engine.json"
    if jq -e --arg b "$1" '(.unsupported_boards // []) | index($b)' "$json" >/dev/null; then
        return 0
    fi
    platform=$(sed -n 's/^PLATFORM *:= *//p' "$board_mk")
    submodules=$(jq -r '(.submodules // []) | join(" ")' "$json")
    rows="$rows$(jq -cn --arg board "$1" --arg engine "$2" --arg platform "$platform" \
        --arg submodules "$submodules" \
        '{board: $board, engine: $engine, platform: $platform, submodules: $submodules,
          variant: "\($board)-\($engine)"}')
"
}

for board_mk in "$fw"/boards/*/board.mk; do
    board=$(basename "$(dirname "$board_mk")")
    for engine_cmake in "$fw"/engines/*/engine.cmake; do
        engine=$(basename "$(dirname "$engine_cmake")")
        add_row "$board" "$engine"
    done
done

printf '%s' "$rows" | jq -cs '{include: .}'
