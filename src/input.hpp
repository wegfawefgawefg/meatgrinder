#pragma once

#include "state.hpp"

struct SDL_Renderer;

void pump_input(InputState& input, SDL_Renderer* renderer);
void consume_input(InputState& input);

