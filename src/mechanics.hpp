#pragma once

#include "state.hpp"

std::vector<int> routed_path(const Level& level, const Match& match, int owner, int start,
                             int goal);
std::vector<int> friendly_path(const Level& level, const Match& match, int owner, int start,
                               int goal);
void step_specialists(State& state);
