#pragma once

#include "state.hpp"

struct SDL_Renderer;

void draw_campaign_screen(SDL_Renderer* renderer, const State& state, float alpha);
bool campaign_transition_revealing(const State& state, float alpha);
void draw_campaign_transition(SDL_Renderer* renderer, const State& state, float alpha);
