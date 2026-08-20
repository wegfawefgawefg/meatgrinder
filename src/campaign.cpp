#include "campaign.hpp"

#include "game.hpp"

#include <algorithm>
#include <cmath>

int level_index(const State& state, std::string_view id) {
    const auto found = std::ranges::find(state.levels, id, &Level::id);
    return found == state.levels.end() ? -1 : static_cast<int>(found - state.levels.begin());
}

bool level_available(const State& state, int index) {
    if (index < 0 || index >= static_cast<int>(state.levels.size())) return false;
    if (index < static_cast<int>(state.results.size()) &&
        state.results[static_cast<std::size_t>(index)].completed) {
        return true;
    }
    const Level& level = state.levels[static_cast<std::size_t>(index)];
    for (const std::string& requirement : level.prerequisites) {
        const int required = level_index(state, requirement);
        if (required < 0 || required >= static_cast<int>(state.results.size()) ||
            !state.results[static_cast<std::size_t>(required)].completed) {
            return false;
        }
    }
    return true;
}

bool world_available(const State& state, int index) {
    if (index < 0 || index >= static_cast<int>(state.worlds.size())) return false;
    const World& world = state.worlds[static_cast<std::size_t>(index)];
    return std::ranges::any_of(world.levels, [&state](int level) {
        return level_available(state, level);
    });
}

bool world_completed(const State& state, int index) {
    if (index < 0 || index >= static_cast<int>(state.worlds.size())) return false;
    const World& world = state.worlds[static_cast<std::size_t>(index)];
    if (world.levels.empty() || world.levels.back() >= static_cast<int>(state.results.size())) return false;
    const int final_level = world.levels.back();
    return state.results[static_cast<std::size_t>(final_level)].completed;
}

void record_level_result(State& state) {
    if (state.campaign_level < 0 || state.campaign_level >= static_cast<int>(state.results.size())) {
        return;
    }
    LevelResult& result = state.results[static_cast<std::size_t>(state.campaign_level)];
    result.completed = true;
    result.best_score = std::max(result.best_score, state.last_level_score);
    if (result.best_seconds <= 0.0F || state.match.stats.elapsed_seconds < result.best_seconds) {
        result.best_seconds = state.match.stats.elapsed_seconds;
    }
    state.progress_dirty = true;
}

Vec2 world_map_position(const World& world) {
    return {160.0F + static_cast<float>(world.map_x) * 120.0F,
            80.0F + static_cast<float>(world.map_y) * 85.0F};
}

Vec2 level_map_position(const Level& level) {
    return {180.0F + static_cast<float>(level.map_x) * 120.0F,
            150.0F + static_cast<float>(level.map_y) * 130.0F};
}

namespace {

void cycle_world(State& state, int direction) {
    if (state.worlds.empty()) return;
    for (std::size_t count = 0; count < state.worlds.size(); ++count) {
        const int size = static_cast<int>(state.worlds.size());
        state.selected_world = (state.selected_world + direction + size) % size;
        if (world_available(state, state.selected_world)) return;
    }
}

void cycle_level(State& state, int direction) {
    const World& world = state.worlds[static_cast<std::size_t>(state.selected_world)];
    if (world.levels.empty()) return;
    const auto found = std::ranges::find(world.levels, state.selected_level);
    int position = found == world.levels.end() ? 0 : static_cast<int>(found - world.levels.begin());
    const int size = static_cast<int>(world.levels.size());
    position = (position + direction + size) % size;
    state.selected_level = world.levels[static_cast<std::size_t>(position)];
}

int world_at_pointer(const State& state) {
    for (int index = 0; index < static_cast<int>(state.worlds.size()); ++index) {
        const Vec2 point = world_map_position(state.worlds[static_cast<std::size_t>(index)]);
        if (std::hypot(point.x - state.input.pointer.x, point.y - state.input.pointer.y) < 62.0F) {
            return index;
        }
    }
    return -1;
}

int level_at_pointer(const State& state) {
    const World& world = state.worlds[static_cast<std::size_t>(state.selected_world)];
    for (int index : world.levels) {
        const Vec2 point = level_map_position(state.levels[static_cast<std::size_t>(index)]);
        if (std::hypot(point.x - state.input.pointer.x, point.y - state.input.pointer.y) < 34.0F) {
            return index;
        }
    }
    return -1;
}

} // namespace

bool step_campaign_navigation(State& state) {
    if (state.mode == Mode::world_select) {
        if (state.input.left_pressed || state.input.up_pressed) cycle_world(state, -1);
        if (state.input.right_pressed || state.input.down_pressed) cycle_world(state, 1);
        bool activate = state.input.confirm_pressed;
        if (state.input.pointer_released) {
            const int clicked = world_at_pointer(state);
            if (clicked >= 0) {
                state.selected_world = clicked;
                activate = true;
            }
        }
        if (state.input.back_pressed) {
            change_mode(state, Mode::main_menu);
        } else if (activate && world_available(state, state.selected_world)) {
            change_mode(state, Mode::world_transition);
        }
        return true;
    }
    if (state.mode == Mode::world_unlock) {
        if (state.mode_seconds >= 1.4F || state.input.confirm_pressed ||
            state.input.pointer_released) {
            change_mode(state, Mode::world_transition);
        }
        return true;
    }
    if (state.mode == Mode::world_transition) {
        if (state.mode_seconds < 0.7F) return true;
        const World& world = state.worlds[static_cast<std::size_t>(state.selected_world)];
        state.selected_level = world.levels.front();
        for (int index : world.levels) {
            if (level_available(state, index) &&
                !state.results[static_cast<std::size_t>(index)].completed) {
                state.selected_level = index;
                break;
            }
        }
        change_mode(state, Mode::level_select);
        return true;
    }
    if (state.mode == Mode::level_select) {
        if (state.input.left_pressed || state.input.up_pressed) cycle_level(state, -1);
        if (state.input.right_pressed || state.input.down_pressed) cycle_level(state, 1);
        bool activate = state.input.confirm_pressed;
        if (state.input.pointer_released) {
            const int clicked = level_at_pointer(state);
            if (clicked >= 0) {
                state.selected_level = clicked;
                activate = true;
            }
        }
        if (state.input.back_pressed) {
            change_mode(state, Mode::world_select);
        } else if (activate && level_available(state, state.selected_level)) {
            change_mode(state, Mode::level_transition);
        }
        return true;
    }
    if (state.mode == Mode::level_transition) {
        if (state.mode_seconds >= 0.7F) start_level(state, state.selected_level);
        return true;
    }
    return false;
}
