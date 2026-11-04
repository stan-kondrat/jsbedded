#!/bin/sh
# Print the CI build matrix as JSON. Needs jq.
#   sh scripts/matrix/ci-matrix.sh
#
# Rows:
#   - every board (firmware/boards/*/board.mk) × every engine (firmware/engines/*/engine.cmake),
#     minus the boards an engine lists in "unsupported_boards" in its engine.json;
#   - per example with an examples/<name>/example.json ({"script", "boards", "engines"}):
#     those boards × engines with the script built in (SCRIPT=), image suffix -<name>.
# Each row: board, engine, platform, submodules, script ("" = built-in), variant
# (build folder and image name: <board>-<engine>[-<example>]).
set -eu

fw=firmware
rows=""

# add_row <board> <engine> <script> <variant>
add_row() {
    board_mk="$fw/boards/$1/board.mk"
    json="$fw/engines/$2/engine.json"
    if [ ! -f "$board_mk" ] || [ ! -f "$fw/engines/$2/engine.cmake" ]; then
        echo "ci-matrix: unknown board $1 or engine $2 (from $4)" >&2
        exit 1
    fi
    if jq -e --arg b "$1" '(.unsupported_boards // []) | index($b)' "$json" >/dev/null; then
        return 0
    fi
    platform=$(sed -n 's/^PLATFORM *:= *//p' "$board_mk")
    submodules=$(jq -r '(.submodules // []) | join(" ")' "$json")
    rows="$rows$(jq -cn --arg board "$1" --arg engine "$2" --arg platform "$platform" \
        --arg submodules "$submodules" --arg script "$3" --arg variant "$4" \
        '{board: $board, engine: $engine, platform: $platform, submodules: $submodules,
          script: $script, variant: $variant}')
"
}

for board_mk in "$fw"/boards/*/board.mk; do
    board=$(basename "$(dirname "$board_mk")")
    for engine_cmake in "$fw"/engines/*/engine.cmake; do
        engine=$(basename "$(dirname "$engine_cmake")")
        add_row "$board" "$engine" "" "$board-$engine"
    done
done

for example_json in examples/*/example.json; do
    [ -f "$example_json" ] || continue
    dir=$(dirname "$example_json")
    name=$(basename "$dir")
    script="$dir/$(jq -r '.script' "$example_json")"
    for board in $(jq -r '.boards[]' "$example_json"); do
        for engine in $(jq -r '.engines[]' "$example_json"); do
            add_row "$board" "$engine" "$script" "$board-$engine-$name"
        done
    done
done

printf '%s' "$rows" | jq -cs '{include: .}'
