#pragma once

#include "state.hpp"

int level_index(const State& state, std::string_view id);
bool level_available(const State& state, int index);
bool world_available(const State& state, int index);
bool world_completed(const State& state, int index);
void record_level_result(State& state);
Vec2 world_map_position(const World& world);
Vec2 level_map_position(const Level& level);
bool step_campaign_navigation(State& state);
