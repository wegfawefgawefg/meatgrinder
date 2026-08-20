#include "game.hpp"

#include "camera.hpp"
#include "level.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace {

NodeState* find_node(Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

std::vector<int> path_between(const Level& level, int start, int goal) {
    constexpr int slot_count = 100;
    std::vector<int> previous(slot_count, -1);
    std::queue<int> open;
    previous[static_cast<std::size_t>(start)] = start;
    open.push(start);
    while (!open.empty() && previous[static_cast<std::size_t>(goal)] < 0) {
        const int current = open.front();
        open.pop();
        for (const Link& link : level.links) {
            const int next = link.a == current ? link.b : link.b == current ? link.a : -1;
            if (next >= 0 && previous[static_cast<std::size_t>(next)] < 0) {
                previous[static_cast<std::size_t>(next)] = current;
                open.push(next);
            }
        }
    }
    if (goal < 0 || goal >= slot_count || previous[static_cast<std::size_t>(goal)] < 0) return {};
    std::vector<int> path;
    for (int at = goal;; at = previous[static_cast<std::size_t>(at)]) {
        path.push_back(at);
        if (at == start) break;
    }
    std::ranges::reverse(path);
    return path;
}

NodeState* headquarters_for(Match& match, int owner) {
    const auto found = std::ranges::find_if(match.nodes, [owner](const NodeState& node) {
        return node.owner == owner && node.headquarters;
    });
    return found == match.nodes.end() ? nullptr : &*found;
}

void ensure_headquarters(Match& match, int owner) {
    if (headquarters_for(match, owner) != nullptr) return;
    const auto replacement = std::ranges::find_if(match.nodes, [owner](const NodeState& node) {
        return node.owner == owner;
    });
    if (replacement != match.nodes.end()) replacement->headquarters = true;
}

bool launch_army(State& state, NodeState& source, int target_id, float soldiers, bool assault) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    std::vector<int> path = path_between(level, source.id, target_id);
    soldiers = std::min(std::floor(soldiers), std::floor(source.soldiers - 1.0F));
    if (path.size() < 2 || soldiers < 1.0F) return false;
    source.soldiers -= soldiers;
    state.match.armies.push_back({
        .owner = source.owner,
        .soldiers = soldiers,
        .path = std::move(path),
        .assault = assault,
    });
    if (source.owner == player_owner) {
        ++state.match.stats.orders;
        state.match.stats.soldiers_sent += static_cast<int>(soldiers);
    }
    return true;
}

void resolve_arrival(State& state, Army& army, int target_id, bool final) {
    NodeState* target = find_node(state.match, target_id);
    if (target == nullptr) {
        army.soldiers = 0.0F;
        return;
    }
    if (target->owner == army.owner) {
        if (final) {
            target->soldiers += army.soldiers;
            army.soldiers = 0.0F;
        }
        return;
    }

    const int old_owner = target->owner;
    const float defense = target->soldiers;
    if (army.soldiers > defense) {
        if (army.owner == player_owner) ++state.match.stats.nodes_captured;
        if (old_owner == player_owner) state.match.stats.soldiers_lost += static_cast<int>(target->soldiers);
        const float survivors = army.soldiers - defense;
        target->headquarters = false;
        target->rally_target = -1;
        target->owner = army.owner;
        if (final) {
            target->soldiers = std::max(1.0F, survivors);
            army.soldiers = 0.0F;
        } else {
            target->soldiers = 1.0F;
            army.soldiers = std::max(0.0F, survivors - 1.0F);
        }
        if (old_owner >= 0) ensure_headquarters(state.match, old_owner);
        const Vec2 captured = node_world_position(
            state.levels[static_cast<std::size_t>(state.campaign_level)], target_id);
        state.camera.target_center.x = std::lerp(state.camera.target_center.x, captured.x, 0.22F);
        state.camera.target_center.y = std::lerp(state.camera.target_center.y, captured.y, 0.22F);
    } else {
        target->soldiers = std::max(0.0F, target->soldiers - army.soldiers);
        if (army.owner == player_owner) state.match.stats.soldiers_lost += static_cast<int>(army.soldiers);
        army.soldiers = 0.0F;
    }
}

void step_armies(State& state) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    for (Army& army : state.match.armies) {
        army.previous_progress = army.progress;
        const int from_id = army.path[static_cast<std::size_t>(army.leg)];
        const int to_id = army.path[static_cast<std::size_t>(army.leg + 1)];
        const Vec2 from = node_world_position(level, from_id);
        const Vec2 to = node_world_position(level, to_id);
        const float distance = std::hypot(to.x - from.x, to.y - from.y);
        army.progress += state.rules.army_speed * step_seconds / std::max(1.0F, distance);
        if (army.progress < 1.0F) continue;
        const bool final = army.leg + 2 >= static_cast<int>(army.path.size());
        if (army.assault || final) resolve_arrival(state, army, to_id, final);
        if (army.soldiers <= 0.0F) continue;
        if (!final) {
            ++army.leg;
            army.previous_progress = 0.0F;
            army.progress = 0.0F;
        } else {
            resolve_arrival(state, army, to_id, true);
        }
    }
    std::erase_if(state.match.armies, [](const Army& army) { return army.soldiers <= 0.0F; });
}

int generated_soldiers(const Match& match, int owner, float scale) {
    int owned = 0;
    int mines = 0;
    for (const NodeState& node : match.nodes) {
        if (node.owner != owner) continue;
        ++owned;
        if (node.kind == NodeKind::mine) ++mines;
    }
    const int base = owned / 2 + 3;
    return static_cast<int>(std::floor(static_cast<float>(base) *
                                      (1.0F + 0.5F * static_cast<float>(mines)) * scale));
}

void run_generation(State& state) {
    for (int owner : {player_owner, enemy_owner}) {
        NodeState* headquarters = headquarters_for(state.match, owner);
        if (headquarters == nullptr) continue;
        const int recruits = generated_soldiers(state.match, owner, state.rules.generation_scale);
        headquarters->soldiers += static_cast<float>(recruits);
        const int rally_target = headquarters->rally_target;
        if (rally_target >= 0 && rally_target != headquarters->id) {
            (void)launch_army(state, *headquarters, rally_target, static_cast<float>(recruits), true);
        }
    }
    ++state.match.stats.generations;
}

void step_generation(State& state) {
    state.match.generation_clock += step_seconds;
    if (state.match.generation_clock < state.rules.generation_seconds) return;
    state.match.generation_clock -= state.rules.generation_seconds;
    run_generation(state);
}

void step_enemy(State& state) {
    state.match.ai_clock += step_seconds;
    if (state.match.ai_clock < state.rules.enemy_think_seconds) return;
    state.match.ai_clock = 0.0F;
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    NodeState* best_source = nullptr;
    int best_target = -1;
    float best_score = -10000.0F;
    for (NodeState& source : state.match.nodes) {
        if (source.owner != enemy_owner || source.soldiers < 7.0F) continue;
        for (const Link& link : level.links) {
            const int target_id = link.a == source.id ? link.b : link.b == source.id ? link.a : -1;
            const NodeState* target = find_node(state.match, target_id);
            if (target == nullptr || target->owner == enemy_owner) continue;
            const float score = source.soldiers - target->soldiers * 1.3F +
                                (target->owner == player_owner ? 8.0F : 0.0F);
            if (score > best_score) {
                best_score = score;
                best_source = &source;
                best_target = target_id;
            }
        }
    }
    if (best_source != nullptr) {
        (void)send_army(state, best_source->id, best_target, state.rules.enemy_aggression);
    }
}

bool side_alive(const Match& match, int owner) {
    return std::ranges::any_of(match.nodes, [owner](const NodeState& node) { return node.owner == owner; }) ||
           std::ranges::any_of(match.armies, [owner](const Army& army) { return army.owner == owner; });
}

void step_outcome(State& state) {
    const bool player_alive = side_alive(state.match, player_owner);
    const bool enemy_alive = side_alive(state.match, enemy_owner);
    if (player_alive && enemy_alive) {
        state.match.outcome_clock = 0.0F;
        return;
    }
    state.match.outcome_clock += step_seconds;
    if (state.match.outcome_clock < 0.8F) return;
    if (!player_alive) {
        change_mode(state, Mode::defeat);
        return;
    }
    const int time_bonus = std::max(0, 3000 - static_cast<int>(state.match.stats.elapsed_seconds * 10.0F));
    state.campaign_score += 1000 + time_bonus + state.match.stats.nodes_captured * 100;
    ++state.completed_levels;
    change_mode(state, Mode::score);
}

void step_frontend(State& state) {
    if (state.mode == Mode::main_menu) {
        if (state.input.up_pressed) state.menu_choice = std::max(0, state.menu_choice - 1);
        if (state.input.down_pressed) state.menu_choice = std::min(2, state.menu_choice + 1);
        bool activate = state.input.confirm_pressed;
        if (state.input.pointer_released && state.input.pointer.x >= 465.0F &&
            state.input.pointer.x <= 815.0F) {
            if (state.input.pointer.y >= 330.0F && state.input.pointer.y <= 380.0F) state.menu_choice = 0;
            else if (state.input.pointer.y >= 395.0F && state.input.pointer.y <= 445.0F) state.menu_choice = 1;
            else if (state.input.pointer.y >= 460.0F && state.input.pointer.y <= 510.0F) state.menu_choice = 2;
            else return;
            activate = true;
        }
        if (!activate) return;
        if (state.menu_choice == 0) {
            state.campaign_level = 0;
            state.campaign_score = 0;
            state.completed_levels = 0;
            start_level(state, 0);
        } else if (state.menu_choice == 1) {
            change_mode(state, Mode::options);
        } else {
            state.input.close_requested = true;
        }
        return;
    }
    if (state.mode == Mode::options) {
        if (state.input.up_pressed) state.options_choice = std::max(0, state.options_choice - 1);
        if (state.input.down_pressed) state.options_choice = std::min(2, state.options_choice + 1);
        bool activate = state.input.left_pressed || state.input.right_pressed || state.input.confirm_pressed;
        if (state.input.pointer_released && state.input.pointer.x >= 465.0F &&
            state.input.pointer.x <= 815.0F) {
            if (state.input.pointer.y >= 310.0F && state.input.pointer.y <= 360.0F) state.options_choice = 0;
            else if (state.input.pointer.y >= 375.0F && state.input.pointer.y <= 425.0F) state.options_choice = 1;
            else if (state.input.pointer.y >= 440.0F && state.input.pointer.y <= 490.0F) state.options_choice = 2;
            else return;
            activate = true;
        }
        if (activate) {
            if (state.options_choice == 0) {
                state.rules.enemy_aggression = state.rules.enemy_aggression < 0.55F ? 0.65F : 0.40F;
            } else if (state.options_choice == 1) {
                state.fullscreen = !state.fullscreen;
            } else {
                change_mode(state, Mode::main_menu);
            }
        }
        if (state.input.back_pressed) change_mode(state, Mode::main_menu);
        return;
    }
    if (state.mode == Mode::level_card) {
        if (state.mode_seconds >= 1.6F || state.input.confirm_pressed || state.input.pointer_released) {
            change_mode(state, Mode::playing);
        }
        return;
    }
    if (state.mode == Mode::paused) {
        if (state.input.confirm_pressed || state.input.back_pressed) change_mode(state, Mode::playing);
        return;
    }
    if (state.mode == Mode::defeat) {
        if (state.input.up_pressed || state.input.down_pressed) state.menu_choice = 1 - state.menu_choice;
        bool activate = state.input.confirm_pressed;
        if (state.input.pointer_released && state.input.pointer.x >= 465.0F &&
            state.input.pointer.x <= 815.0F) {
            if (state.input.pointer.y >= 340.0F && state.input.pointer.y <= 390.0F) state.menu_choice = 0;
            else if (state.input.pointer.y >= 405.0F && state.input.pointer.y <= 455.0F) state.menu_choice = 1;
            else return;
            activate = true;
        }
        if (!activate) return;
        if (state.menu_choice == 0) restart_level(state);
        else change_mode(state, Mode::main_menu);
        return;
    }
    if (state.mode == Mode::score && (state.input.confirm_pressed || state.input.pointer_released)) {
        if (state.campaign_level + 1 < static_cast<int>(state.levels.size())) {
            start_level(state, state.campaign_level + 1);
        } else {
            change_mode(state, Mode::campaign_complete);
        }
        return;
    }
    if (state.mode == Mode::campaign_complete &&
        (state.input.confirm_pressed || state.input.pointer_released || state.input.back_pressed)) {
        change_mode(state, Mode::main_menu);
    }
}

} // namespace

void change_mode(State& state, Mode mode) {
    state.mode = mode;
    state.mode_seconds = 0.0F;
    state.menu_choice = 0;
}

void start_level(State& state, int level_index) {
    state.campaign_level = level_index;
    const Level& level = state.levels[static_cast<std::size_t>(level_index)];
    state.match = {};
    state.match.nodes.reserve(level.nodes.size());
    for (const LevelNode& node : level.nodes) {
        state.match.nodes.push_back({
            .id = node.id,
            .kind = node.kind,
            .owner = node.owner,
            .soldiers = node.soldiers,
        });
    }
    for (int owner : {player_owner, enemy_owner}) ensure_headquarters(state.match, owner);
    state.relocating_headquarters = false;
    setup_camera(state, level);
    change_mode(state, Mode::level_card);
}

void restart_level(State& state) {
    start_level(state, state.campaign_level);
}

bool send_army(State& state, int source_id, int target_id, float fraction) {
    return send_army(state, source_id, target_id, fraction, true);
}

bool send_army(State& state, int source_id, int target_id, float fraction, bool assault) {
    NodeState* source = find_node(state.match, source_id);
    if (source == nullptr || source->owner == neutral_owner || source->soldiers < 2.0F) return false;
    const float soldiers = std::floor(source->soldiers * std::clamp(fraction, 0.1F, 0.9F));
    return launch_army(state, *source, target_id, soldiers, assault);
}

std::vector<int> find_path(const Level& level, int source_id, int target_id) {
    return path_between(level, source_id, target_id);
}

bool relocate_headquarters(State& state, int node_id) {
    NodeState* node = find_node(state.match, node_id);
    NodeState* old = headquarters_for(state.match, player_owner);
    if (node == nullptr || old == nullptr || node->owner != player_owner || node == old) return false;
    node->headquarters = true;
    node->rally_target = old->rally_target;
    old->headquarters = false;
    old->rally_target = -1;
    ++state.match.stats.headquarters_moves;
    return true;
}

bool set_rally_order(State& state, int source_id, int target_id) {
    NodeState* source = find_node(state.match, source_id);
    if (source == nullptr || !source->headquarters || source->owner != player_owner ||
        target_id < 0 || source_id == target_id) return false;
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    if (find_path(level, source_id, target_id).size() < 2) return false;
    source->rally_target = target_id;
    return true;
}

void clear_selection(State& state) {
    for (NodeState& node : state.match.nodes) node.selected = false;
}

void select_node(State& state, int node_id, bool add) {
    if (!add) clear_selection(state);
    NodeState* node = find_node(state.match, node_id);
    if (node != nullptr && node->owner == player_owner) node->selected = true;
}

void select_box(State& state, Vec2 a, Vec2 b) {
    clear_selection(state);
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    a = screen_to_world(a, state.camera);
    b = screen_to_world(b, state.camera);
    const float left = std::min(a.x, b.x);
    const float right = std::max(a.x, b.x);
    const float top = std::min(a.y, b.y);
    const float bottom = std::max(a.y, b.y);
    for (NodeState& node : state.match.nodes) {
        const Vec2 point = node_world_position(level, node.id);
        node.selected = node.owner == player_owner && point.x >= left && point.x <= right &&
                        point.y >= top && point.y <= bottom;
    }
}

int node_at_pointer(const State& state, Vec2 pointer) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    const Vec2 world = screen_to_world(pointer, state.camera);
    for (const NodeState& node : state.match.nodes) {
        const Vec2 point = node_world_position(level, node.id);
        if (std::hypot(point.x - world.x, point.y - world.y) <= 28.0F) return node.id;
    }
    return -1;
}

void handle_pointer_release(State& state) {
    if (state.mode != Mode::playing) return;
    const float drag = std::hypot(state.input.pointer.x - state.input.press_origin.x,
                                  state.input.pointer.y - state.input.press_origin.y);
    if (drag > 10.0F) {
        select_box(state, state.input.press_origin, state.input.pointer);
        return;
    }
    const int clicked = node_at_pointer(state, state.input.pointer);
    if (clicked < 0) {
        clear_selection(state);
        return;
    }
    NodeState* target = find_node(state.match, clicked);
    if (state.relocating_headquarters) {
        if (relocate_headquarters(state, clicked)) state.relocating_headquarters = false;
        return;
    }
    const bool has_selection = std::ranges::any_of(state.match.nodes, &NodeState::selected);
    if (target != nullptr && target->owner == player_owner && !has_selection) {
        select_node(state, clicked, false);
        return;
    }
    if (has_selection) {
        std::vector<int> sources;
        for (const NodeState& node : state.match.nodes) {
            if (node.selected && node.id != clicked) sources.push_back(node.id);
        }
        for (int source : sources) {
            (void)send_army(state, source, clicked, 0.5F, !state.input.direct_down);
        }
        clear_selection(state);
        return;
    }
    select_node(state, clicked, false);
}

void handle_secondary_release(State& state) {
    if (state.mode != Mode::playing) return;
    const float drag = std::hypot(state.input.pointer.x - state.input.secondary_origin.x,
                                  state.input.pointer.y - state.input.secondary_origin.y);
    if (drag <= 10.0F) {
        NodeState* clicked = find_node(state.match, node_at_pointer(state, state.input.pointer));
        if (clicked != nullptr && clicked->owner == player_owner && clicked->headquarters) {
            clicked->rally_target = -1;
        }
        clear_selection(state);
        return;
    }
    const int source_id = node_at_pointer(state, state.input.secondary_origin);
    const int target_id = node_at_pointer(state, state.input.pointer);
    (void)set_rally_order(state, source_id, target_id);
}

void step(State& state) {
    state.mode_seconds += step_seconds;
    if (state.mode == Mode::playing) step_camera(state);
    if (state.input.relocate_hq_pressed && state.mode == Mode::playing) {
        state.relocating_headquarters = !state.relocating_headquarters;
        clear_selection(state);
    }
    if (state.input.pointer_released) handle_pointer_release(state);
    if (state.input.secondary_released) handle_secondary_release(state);
    if (state.mode != Mode::playing) {
        step_frontend(state);
        return;
    }
    if (state.input.back_pressed) {
        change_mode(state, Mode::paused);
        return;
    }
    state.match.stats.elapsed_seconds += step_seconds;
    step_generation(state);
    step_armies(state);
    step_enemy(state);
    step_outcome(state);
}
