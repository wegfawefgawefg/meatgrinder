#include "camera.hpp"
#include "ai.hpp"
#include "game.hpp"
#include "level.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>

namespace {

NodeState& match_node(State& state, int id) {
    for (NodeState& node : state.match.nodes) {
        if (node.id == id) return node;
    }
    assert(false);
    return state.match.nodes.front();
}

int headquarters_count(const State& state, int owner) {
    int count = 0;
    for (const NodeState& node : state.match.nodes) {
        if (node.owner == owner && node.headquarters) ++count;
    }
    return count;
}

void step_until_armies_stop(State& state) {
    state.rules.army_speed = 1000.0F;
    state.rules.enemy_think_seconds = 10000.0F;
    state.rules.generation_seconds = 10000.0F;
    state.mode = Mode::playing;
    for (int frame = 0; frame < 2000 && !state.match.armies.empty(); ++frame) step(state);
    assert(state.match.armies.empty());
}

} // namespace

int main() {
    State state;
    assert(state.rules.army_speed == 20.0F);
    std::string error;
    assert(load_campaign(std::string(MG_ASSET_ROOT) + "/levels/campaign.json", state.levels, error));
    assert(state.levels.size() == 10);
    assert(state.levels.front().nodes.size() == 20);
    assert(state.levels.back().nodes.size() == 52);

    start_level(state, 0);
    assert(state.mode == Mode::level_card);
    assert(state.match.nodes.size() == state.levels.front().nodes.size());
    assert(headquarters_count(state, player_owner) == 1);
    assert(headquarters_count(state, enemy_owner) == 1);

    int source = -1;
    int target = -1;
    for (const Link& link : state.levels.front().links) {
        const auto a = state.match.nodes[static_cast<std::size_t>(node_position(state.levels.front(), link.a))];
        if (a.owner == player_owner) {
            source = link.a;
            target = link.b;
            break;
        }
        const auto b = state.match.nodes[static_cast<std::size_t>(node_position(state.levels.front(), link.b))];
        if (b.owner == player_owner) {
            source = link.b;
            target = link.a;
            break;
        }
    }
    assert(source >= 0 && target >= 0);
    assert(send_army(state, source, target));
    assert(state.match.armies.size() == 1);
    assert(state.match.stats.orders == 1);

    step_until_armies_stop(state);

    // verify every owned base receives the same fixed GEN recruit
    restart_level(state);
    state.mode = Mode::playing;
    state.rules.enemy_think_seconds = 10000.0F;
    state.rules.generation_seconds = 0.1F;
    std::vector<float> before;
    for (const NodeState& node : state.match.nodes) before.push_back(node.soldiers);
    for (int frame = 0; frame < 7; ++frame) step(state);
    for (std::size_t index = 0; index < state.match.nodes.size(); ++index) {
        const NodeState& node = state.match.nodes[index];
        if (node.owner >= 0) assert(node.soldiers == before[index] + 1.0F);
        else assert(node.soldiers == before[index]);
    }
    assert(state.match.stats.generations == 1);

    // verify any base can forward its chosen commitment as a direct rally
    restart_level(state);
    int rally_source = -1;
    int rally_target = -1;
    for (const NodeState& node : state.match.nodes) {
        if (node.owner == player_owner && !node.headquarters && rally_source < 0) rally_source = node.id;
        else if (node.owner == player_owner && node.id != rally_source) rally_target = node.id;
    }
    assert(rally_source >= 0 && rally_target >= 0);
    match_node(state, rally_source).soldiers = 5.0F;
    assert(set_rally_order(state, rally_source, rally_target, false, DispatchMode::half));
    assert(!match_node(state, rally_source).rally_assault);
    assert(match_node(state, rally_source).rally_dispatch == DispatchMode::half);
    state.mode = Mode::playing;
    state.rules.enemy_think_seconds = 10000.0F;
    state.rules.generation_seconds = 0.1F;
    for (int frame = 0; frame < 7; ++frame) step(state);
    assert(!state.match.armies.empty());
    assert(!state.match.armies.front().assault);
    assert(state.match.armies.front().soldiers == 3.0F);

    // verify one, half, and all-but-one are exact and usable by regular orders
    for (const auto& [mode, sent] : {std::pair{DispatchMode::one, 1.0F},
                                     std::pair{DispatchMode::half, 5.0F},
                                     std::pair{DispatchMode::all_but_one, 9.0F}}) {
        restart_level(state);
        match_node(state, source).soldiers = 10.0F;
        assert(send_army(state, source, target, mode));
        assert(state.match.armies.back().soldiers == sent);
        assert(match_node(state, source).soldiers == 10.0F - sent);
    }

    // verify a selected group becomes rally sources and old orders clear on entry
    restart_level(state);
    std::vector<int> group;
    for (NodeState& node : state.match.nodes) {
        if (node.owner != player_owner || group.size() == 2) continue;
        node.selected = true;
        node.rally_target = target;
        group.push_back(node.id);
    }
    assert(group.size() == 2);
    state.mode = Mode::playing;
    state.input.rally_orders_pressed = true;
    step(state);
    state.input.rally_orders_pressed = false;
    assert(state.rally_sources == group);
    assert(match_node(state, group[0]).rally_target == -1);
    assert(match_node(state, group[1]).rally_target == -1);
    state.input.back_pressed = true;
    step(state);
    state.input.back_pressed = false;
    assert(state.rally_sources.empty());
    assert(state.mode == Mode::playing);
    assert(clear_selected_rallies(state));

    // verify Shift-style selection adds and toggles without disturbing the group
    clear_selection(state);
    select_node(state, group[0], true);
    select_node(state, group[1], true);
    assert(match_node(state, group[0]).selected);
    assert(match_node(state, group[1]).selected);
    select_node(state, group[0], true);
    assert(!match_node(state, group[0]).selected);
    assert(match_node(state, group[1]).selected);
    const Vec2 group_point = world_to_screen(node_world_position(state.levels.front(), group[0]),
                                             state.camera.center, state.camera.zoom);
    select_box(state, {group_point.x - 2.0F, group_point.y - 2.0F},
               {group_point.x + 2.0F, group_point.y + 2.0F}, true);
    assert(match_node(state, group[0]).selected);
    assert(match_node(state, group[1]).selected);

    // verify a friendly click replaces selection in one click
    clear_selection(state);
    select_node(state, group[0], false);
    const Vec2 other_point = world_to_screen(node_world_position(state.levels.front(), group[1]),
                                             state.camera.center, state.camera.zoom);
    state.input.press_origin = other_point;
    state.input.pointer = other_point;
    state.input.direct_down = false;
    handle_pointer_release(state);
    assert(!match_node(state, group[0]).selected);
    assert(match_node(state, group[1]).selected);
    assert(state.match.armies.empty());

    // verify Control-click retains deliberate one-shot friendly reinforcement
    clear_selection(state);
    select_node(state, group[0], false);
    state.input.direct_down = true;
    handle_pointer_release(state);
    state.input.direct_down = false;
    assert(!state.match.armies.empty());
    assert(state.match.armies.back().path.back() == group[1]);

    // verify an assault captures an intermediate stop while a direct order bypasses it
    restart_level(state);
    int assault_source = -1;
    std::vector<int> assault_path;
    for (const NodeState& node : state.match.nodes) {
        if (node.owner != player_owner) continue;
        for (const NodeState& candidate : state.match.nodes) {
            const std::vector<int> path = find_path(state.levels.front(), node.id, candidate.id);
            if (path.size() >= 3) {
                assault_source = node.id;
                assault_path = path;
                break;
            }
        }
        if (assault_source >= 0) break;
    }
    assert(assault_path.size() >= 3);
    match_node(state, assault_source).soldiers = 40.0F;
    match_node(state, assault_path[1]).owner = enemy_owner;
    match_node(state, assault_path[1]).soldiers = 1.0F;
    match_node(state, assault_path.back()).owner = enemy_owner;
    match_node(state, assault_path.back()).soldiers = 1.0F;
    const Vec2 camera_target = state.camera.target_center;
    assert(send_army(state, assault_source, assault_path.back(), 0.75F, true));
    step_until_armies_stop(state);
    assert(match_node(state, assault_path[1]).owner == player_owner);
    assert(state.camera.target_center.x == camera_target.x);
    assert(state.camera.target_center.y == camera_target.y);

    restart_level(state);
    match_node(state, assault_source).soldiers = 40.0F;
    match_node(state, assault_path[1]).owner = enemy_owner;
    match_node(state, assault_path[1]).soldiers = 1.0F;
    match_node(state, assault_path.back()).owner = enemy_owner;
    match_node(state, assault_path.back()).soldiers = 1.0F;
    assert(send_army(state, assault_source, assault_path.back(), 0.75F, false));
    step_until_armies_stop(state);
    assert(match_node(state, assault_path[1]).owner == enemy_owner);
    assert(match_node(state, assault_path.back()).owner == player_owner);

    // verify a headquarters is fixed and its capture ends the match immediately
    restart_level(state);
    int player_hq = -1;
    int enemy_hq = -1;
    int player_attack_source = -1;
    int enemy_attack_source = -1;
    for (const NodeState& node : state.match.nodes) {
        if (node.owner == player_owner) {
            if (node.headquarters) player_hq = node.id;
            else player_attack_source = node.id;
        }
        if (node.owner == enemy_owner) {
            if (node.headquarters) enemy_hq = node.id;
            else enemy_attack_source = node.id;
        }
    }
    assert(player_hq >= 0 && enemy_hq >= 0);
    assert(player_attack_source >= 0 && enemy_attack_source >= 0);
    match_node(state, player_attack_source).soldiers = 10.0F;
    match_node(state, enemy_hq).soldiers = 1.0F;
    state.mode = Mode::playing;
    state.rules.army_speed = 100000.0F;
    state.rules.enemy_think_seconds = 10000.0F;
    state.rules.generation_seconds = 10000.0F;
    assert(send_army_path(state, player_attack_source, {player_attack_source, enemy_hq}, 9.0F));
    step(state);
    assert(state.match.defeated_owner == enemy_owner);
    assert(state.mode == Mode::score);
    assert(headquarters_count(state, player_owner) == 1);
    assert(headquarters_count(state, enemy_owner) == 0);

    restart_level(state);
    match_node(state, enemy_attack_source).soldiers = 10.0F;
    match_node(state, player_hq).soldiers = 1.0F;
    state.mode = Mode::playing;
    assert(send_army_path(state, enemy_attack_source, {enemy_attack_source, player_hq}, 9.0F));
    step(state);
    assert(state.match.defeated_owner == player_owner);
    assert(state.mode == Mode::defeat);
    assert(headquarters_count(state, player_owner) == 0);
    assert(headquarters_count(state, enemy_owner) == 1);

    restart_level(state);
    state.mode = Mode::playing;
    state.input.dispatch_choice = 2;
    step(state);
    assert(state.dispatch_mode == DispatchMode::all_but_one);

    // verify pointer panning grabs the world at the current zoom
    state.camera.center = {100.0F, 200.0F};
    state.camera.target_center = state.camera.center;
    state.camera.zoom = 2.0F;
    state.camera.target_zoom = 2.0F;
    state.input.camera_pan_delta = {20.0F, -10.0F};
    step_camera(state);
    assert(state.camera.target_center.x == 90.0F);
    assert(state.camera.target_center.y == 205.0F);
    state.input.camera_pan_delta = {};

    start_level(state, 3);
    assert(state.match.ai_style == AiStyle::swarm);
    state.mode = Mode::playing;
    state.rules.army_speed = 40.0F;
    state.rules.generation_seconds = 10000.0F;
    state.rules.enemy_think_seconds = 0.85F;
    for (int frame = 0; frame < 120; ++frame) step(state);
    assert(std::ranges::any_of(state.match.armies, [](const Army& army) {
        return army.owner == enemy_owner;
    }));

    // verify a stalled AI moves rear surplus into a friendly frontline base
    start_level(state, 0);
    int supply_source = -1;
    int supply_stage = -1;
    int defended_target = -1;
    for (const NodeState& from : state.match.nodes) {
        for (const NodeState& to : state.match.nodes) {
            const std::vector<int> path = find_path(state.levels.front(), from.id, to.id);
            if (path.size() >= 3) {
                supply_source = path.front();
                supply_stage = path[path.size() - 2];
                defended_target = path.back();
                break;
            }
        }
        if (supply_source >= 0) break;
    }
    assert(supply_source >= 0 && supply_stage >= 0 && defended_target >= 0);
    for (NodeState& node : state.match.nodes) {
        node.owner = enemy_owner;
        node.soldiers = 2.0F;
        node.headquarters = false;
    }
    match_node(state, supply_source).soldiers = 50.0F;
    match_node(state, defended_target).owner = player_owner;
    match_node(state, defended_target).soldiers = 60.0F;
    match_node(state, defended_target).headquarters = true;
    state.match.ai_style = AiStyle::balanced;
    state.ai_difficulty = AiDifficulty::hard;
    state.mode = Mode::playing;
    state.rules.generation_seconds = 10000.0F;
    run_enemy_decision(state);
    const auto supply = std::ranges::find_if(state.match.armies, [supply_stage](const Army& army) {
        return army.owner == enemy_owner && army.path.back() == supply_stage;
    });
    assert(supply != state.match.armies.end());
    assert(std::ranges::all_of(supply->path, [&state](int node_id) {
        return match_node(state, node_id).owner == enemy_owner;
    }));

    // verify objectives resist tiny score flicker but switch on a material improvement
    state.match.armies.clear();
    int alternate_target = -1;
    for (NodeState& node : state.match.nodes) {
        if (node.id == defended_target || node.owner != enemy_owner) continue;
        alternate_target = node.id;
        break;
    }
    assert(alternate_target >= 0);
    match_node(state, alternate_target).owner = player_owner;
    match_node(state, alternate_target).soldiers = 39.0F;
    match_node(state, defended_target).soldiers = 40.0F;
    match_node(state, defended_target).headquarters = false;
    state.match.ai.objective = defended_target;
    run_enemy_decision(state);
    assert(state.match.ai.objective == defended_target);
    match_node(state, alternate_target).soldiers = 0.0F;
    run_enemy_decision(state);
    assert(state.match.ai.objective == alternate_target);
    match_node(state, alternate_target).owner = enemy_owner;
    run_enemy_decision(state);
    assert(state.match.ai.objective == defended_target);

    // verify inbound player troops interrupt even a weak AI's possible WAIT roll
    start_level(state, 0);
    for (NodeState& node : state.match.nodes) {
        node.owner = enemy_owner;
        node.soldiers = 2.0F;
        node.headquarters = false;
    }
    match_node(state, supply_source).soldiers = 50.0F;
    match_node(state, defended_target).owner = player_owner;
    match_node(state, defended_target).soldiers = 60.0F;
    state.match.armies.push_back({
        .owner = player_owner,
        .soldiers = 10.0F,
        .path = {defended_target, supply_stage},
    });
    state.ai_difficulty = AiDifficulty::weak;
    run_enemy_decision(state);
    assert(state.match.ai.last_behavior == AiBehavior::reinforce);
    assert(std::ranges::any_of(state.match.armies, [supply_stage](const Army& army) {
        return army.owner == enemy_owner && army.path.back() == supply_stage;
    }));

    // verify hard difficulty can issue two commands without reusing a source
    start_level(state, 0);
    for (NodeState& node : state.match.nodes) {
        node.owner = player_owner;
        node.soldiers = 1.0F;
        node.headquarters = false;
    }
    match_node(state, supply_source).owner = enemy_owner;
    match_node(state, supply_source).soldiers = 50.0F;
    match_node(state, supply_stage).owner = enemy_owner;
    match_node(state, supply_stage).soldiers = 50.0F;
    state.match.ai_style = AiStyle::aggressive;
    state.ai_difficulty = AiDifficulty::hard;
    run_enemy_decision(state);
    assert(state.match.ai.orders == 2);
    assert(state.match.armies.size() == 2);
    assert(state.match.armies[0].path.front() != state.match.armies[1].path.front());

    const Vec2 aligned = node_world_position(state.levels.front(), state.levels.front().nodes[0].id);
    assert(std::fmod(aligned.x, 64.0F) == 32.0F);
    assert(std::fmod(aligned.y, 64.0F) == 32.0F);

    std::cout << "loaded campaign; verified GEN, rallies, routing, HQ, and AI logistics\n";
    return 0;
}
