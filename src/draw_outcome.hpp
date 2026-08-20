#pragma once

#include "state.hpp"

struct SDL_Renderer;

void draw_victory_effect(SDL_Renderer* renderer, const State& state, float alpha);
void draw_score_screen(SDL_Renderer* renderer, const State& state, float alpha);
