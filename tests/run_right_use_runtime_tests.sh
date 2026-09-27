#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_binary=$(mktemp)
trap 'rm -f "$test_binary"' EXIT
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
    -fsanitize=undefined -fno-omit-frame-pointer \
    -Itests/stubs -Isrc tests/right_use_runtime_test.cpp \
    src/runtime/ActionHandContext.cpp -ldl -o "$test_binary"
status=0
for case_name in release_off release_main release_stale transaction_unscoped consume_native_off off_count_sync consume_food consume_container consume_last consume_stale consume_main setter_unscoped shears spear_tag_main_hold sword main_pass both_pass main_success main_terminal main_fallback_block_then_off_throwable_terminal main_owned_block_then_off_throwable_terminal main_block_then_off_food_terminal air_main_instant_false_terminal air_empty_main_instant_manual air_empty_main_instant_swap air_empty_main_component_throwable air_empty_main_throwable_potion_a air_empty_main_throwable_potion_b air_empty_main_potion_target_nonthrowable_blocked air_empty_main_component_nonthrowable_blocked air_empty_main_no_off air_empty_main_instant_pass off_weapon_nested_main_food_blocked off_food_selected_hotpath_no_native_probes off_weapon_air_blocked off_weapon_block_blocked off_weapon_direct_hand_blocked air_empty_main_long_use_blocked eat_offhand_manual eat_offhand_swap air_snapshot air_main_success bow_block_pass main_scope off_terminal air_main_pass missing_snapshot disabled_air_empty_main disabled; do
    "$test_binary" "$case_name" || status=1
done
exit "$status"
