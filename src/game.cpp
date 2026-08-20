#include "game.hpp"

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

float production_for(const NodeState& node) {
    float base = 0.30F;
    if (node.kind == NodeKind::castle) base = 0.75F;
    if (node.kind == NodeKind::stable) base = 0.55F;
    if (node.kind == NodeKind::mine) base = 0.90F;
    return base * static_cast<float>(node.tier);
}

void resolve_arrival(State& state, Army& army) {
    Match& match = state.match;
    NodeState* target = find_node(match, army.path.back());
    if (target == nullptr) return;
    if (target->owner == army.owner) {
        target->soldiers += army.soldiers;
        return;
    }

    const float defense = target->soldiers * (1.0F + 0.18F * static_cast<float>(target->tier - 1));
    if (army.soldiers > defense) {
        if (army.owner == player_owner) ++match.stats.nodes_captured;
        if (target->owner == player_owner) match.stats.soldiers_lost += static_cast<int>(target->soldiers);
        target->owner = army.owner;
        target->soldiers = std::max(1.0F, army.soldiers - defense);
        target->tier = 1;
    } else {
        const float killed = army.soldiers / (1.0F + 0.18F * static_cast<float>(target->tier - 1));
        target->soldiers = std::max(0.0F, target->soldiers - killed);
        if (army.owner == player_owner) match.stats.soldiers_lost += static_cast<int>(army.soldiers);
    }
}

void step_armies(State& state) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    for (Army& army : state.match.armies) {
        army.previous_progress = army.progress;
        const int from_id = army.path[static_cast<std::size_t>(army.leg)];
        const int to_id = army.path[static_cast<std::size_t>(army.leg + 1)];
        const Vec2 from = node_screen_position(level, from_id);
        const Vec2 to = node_screen_position(level, to_id);
        const float distance = std::hypot(to.x - from.x, to.y - from.y);
        army.progress += state.rules.army_speed * step_seconds / std::max(1.0F, distance);
        if (army.progress < 1.0F) continue;
        if (army.leg + 2 < static_cast<int>(army.path.size())) {
            ++army.leg;
            army.previous_progress = 0.0F;
            army.progress = 0.0F;
        } else {
            resolve_arrival(state, army);
            army.soldiers = 0.0F;
        }
    }
    std::erase_if(state.match.armies, [](const Army& army) { return army.soldiers <= 0.0F; });
}

void step_production(State& state) {
    for (NodeState& node : state.match.nodes) {
        if (node.owner == neutral_owner) continue;
        node.soldiers += production_for(node) * state.rules.production_scale * step_seconds;
    }
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
    change_mode(state, Mode::level_card);
}

void restart_level(State& state) {
    start_level(state, state.campaign_level);
}

bool send_army(State& state, int source_id, int target_id, float fraction) {
    NodeState* source = find_node(state.match, source_id);
    if (source == nullptr || source->owner == neutral_owner || source->soldiers < 2.0F) return false;
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    std::vector<int> path = path_between(level, source_id, target_id);
    if (path.size() < 2) return false;
    const float soldiers = std::floor(source->soldiers * std::clamp(fraction, 0.1F, 0.9F));
    if (soldiers < 1.0F) return false;
    source->soldiers -= soldiers;
    state.match.armies.push_back({
        .owner = source->owner,
        .soldiers = soldiers,
        .path = std::move(path),
    });
    if (source->owner == player_owner) {
        ++state.match.stats.orders;
        state.match.stats.soldiers_sent += static_cast<int>(soldiers);
    }
    return true;
}

bool promote_node(State& state, int node_id) {
    NodeState* node = find_node(state.match, node_id);
    if (node == nullptr || node->owner != player_owner || node->tier >= 3 ||
        node->soldiers < static_cast<float>(state.rules.promotion_cost + 1)) return false;
    node->soldiers -= static_cast<float>(state.rules.promotion_cost);
    ++node->tier;
    ++state.match.stats.promotions;
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
    const float left = std::min(a.x, b.x);
    const float right = std::max(a.x, b.x);
    const float top = std::min(a.y, b.y);
    const float bottom = std::max(a.y, b.y);
    for (NodeState& node : state.match.nodes) {
        const Vec2 point = node_screen_position(level, node.id);
        node.selected = node.owner == player_owner && point.x >= left && point.x <= right &&
                        point.y >= top && point.y <= bottom;
    }
}

void handle_pointer_release(State& state) {
    if (state.mode != Mode::playing) return;
    const float drag = std::hypot(state.input.pointer.x - state.input.press_origin.x,
                                  state.input.pointer.y - state.input.press_origin.y);
    if (drag > 10.0F) {
        select_box(state, state.input.press_origin, state.input.pointer);
        return;
    }
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    int clicked = -1;
    for (const NodeState& node : state.match.nodes) {
        const Vec2 point = node_screen_position(level, node.id);
        if (std::hypot(point.x - state.input.pointer.x, point.y - state.input.pointer.y) <= 24.0F) {
            clicked = node.id;
            break;
        }
    }
    if (clicked < 0) {
        clear_selection(state);
        return;
    }
    NodeState* target = find_node(state.match, clicked);
    if (state.input.modifier_down && target != nullptr && target->owner == player_owner) {
        (void)promote_node(state, clicked);
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
        for (int source : sources) (void)send_army(state, source, clicked);
        clear_selection(state);
        return;
    }
    select_node(state, clicked, false);
}

void step(State& state) {
    state.mode_seconds += step_seconds;
    if (state.input.secondary_pressed && state.mode == Mode::playing) clear_selection(state);
    if (state.input.pointer_released) handle_pointer_release(state);
    if (state.mode != Mode::playing) {
        step_frontend(state);
        return;
    }
    if (state.input.back_pressed) {
        change_mode(state, Mode::paused);
        return;
    }
    state.match.stats.elapsed_seconds += step_seconds;
    step_production(state);
    step_armies(state);
    step_enemy(state);
    step_outcome(state);
}
