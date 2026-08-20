#pragma once

#include "state.hpp"

void change_mode(State& state, Mode mode);
void start_level(State& state, int level_index);
void restart_level(State& state);
bool send_army(State& state, int source_id, int target_id, float fraction = 0.5F);
bool promote_node(State& state, int node_id);
void clear_selection(State& state);
void select_node(State& state, int node_id, bool add);
void select_box(State& state, Vec2 a, Vec2 b);
void handle_pointer_release(State& state);
void step(State& state);

