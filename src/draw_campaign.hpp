#pragma once

#include "state.hpp"

struct SDL_Renderer;

void draw_campaign_screen(SDL_Renderer* renderer, const State& state);
