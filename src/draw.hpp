#pragma once

#include "state.hpp"

struct SDL_Renderer;

void draw(SDL_Renderer* renderer, const State& state, float alpha);

