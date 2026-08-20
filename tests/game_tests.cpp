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
    assert(begin_rally_orders(state, group.front()));
    assert(state.rally_sources == group);
    assert(match_node(state, group[0]).rally_target == -1);
    assert(match_node(state, group[1]).rally_target == -1);
    state.mode = Mode::playing;
    state.input.back_pressed = true;
    step(state);
    state.input.back_pressed = false;
    assert(state.rally_sources.empty());
    assert(state.mode == Mode::playing);
    assert(clear_selected_rallies(state));

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

    // verify HQ relocation keeps exactly one player headquarters
    restart_level(state);
    int replacement = -1;
    for (const NodeState& node : state.match.nodes) {
        if (node.owner == player_owner && !node.headquarters) replacement = node.id;
    }
    assert(replacement >= 0 && relocate_headquarters(state, replacement));
    assert(headquarters_count(state, player_owner) == 1);
    assert(match_node(state, replacement).headquarters);

    state.mode = Mode::playing;
    state.input.dispatch_choice = 2;
    step(state);
    assert(state.dispatch_mode == DispatchMode::all_but_one);

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

    const Vec2 aligned = node_world_position(state.levels.front(), state.levels.front().nodes[0].id);
    assert(std::fmod(aligned.x, 64.0F) == 32.0F);
    assert(std::fmod(aligned.y, 64.0F) == 32.0F);

    std::cout << "loaded campaign; verified distributed GEN, rallies, routing, HQ, and AI styles\n";
    return 0;
}
