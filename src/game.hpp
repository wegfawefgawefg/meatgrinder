#pragma once

#include "state.hpp"

void change_mode(State& state, Mode mode);
void start_level(State& state, int level_index);
void restart_level(State& state);
bool send_army(State& state, int source_id, int target_id, float fraction = 0.5F);
bool send_army(State& state, int source_id, int target_id, float fraction, bool assault);
bool send_army(State& state, int source_id, int target_id, DispatchMode mode, bool assault = true);
std::vector<int> find_path(const Level& level, int source_id, int target_id);
bool relocate_headquarters(State& state, int node_id);
bool set_rally_order(State& state, int source_id, int target_id, bool assault = true,
                     DispatchMode mode = DispatchMode::half);
bool begin_selected_rally_orders(State& state);
bool clear_selected_rallies(State& state);
void clear_selection(State& state);
void select_node(State& state, int node_id, bool add);
void select_box(State& state, Vec2 a, Vec2 b, bool add = false);
int node_at_pointer(const State& state, Vec2 pointer);
void handle_pointer_release(State& state);
void step(State& state);
