#pragma once

#include "state.hpp"

struct SDL_Renderer;
struct SDL_Window;

bool init_debug(SDL_Window* window, SDL_Renderer* renderer);
void draw_debug(State& state, SDL_Renderer* renderer);
void shutdown_debug();
