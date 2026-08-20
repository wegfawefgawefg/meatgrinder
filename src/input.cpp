#include "input.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>

namespace {

Vec2 logical_pointer(SDL_Renderer* renderer, float x, float y) {
    (void)SDL_RenderCoordinatesFromWindow(renderer, x, y, &x, &y);
    return {x, y};
}

} // namespace

void pump_input(InputState& input, SDL_Renderer* renderer) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            input.close_requested = true;
            continue;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            if (event.key.key == SDLK_F1) input.toggle_debug_pressed = true;
            if (event.key.key == SDLK_F11) input.toggle_fullscreen_pressed = true;
            if (ImGui::GetIO().WantCaptureKeyboard) continue;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) input.confirm_pressed = true;
            if (event.key.key == SDLK_ESCAPE) input.back_pressed = true;
            if (event.key.key == SDLK_UP || event.key.key == SDLK_W) input.up_pressed = true;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_S) input.down_pressed = true;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_A) input.left_pressed = true;
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_D) input.right_pressed = true;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            input.pointer = logical_pointer(renderer, event.motion.x, event.motion.y);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !ImGui::GetIO().WantCaptureMouse) {
            input.pointer = logical_pointer(renderer, event.button.x, event.button.y);
            if (event.button.button == SDL_BUTTON_LEFT) {
                input.pointer_pressed = true;
                input.pointer_down = true;
                input.press_origin = input.pointer;
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                input.secondary_pressed = true;
            }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && !ImGui::GetIO().WantCaptureMouse) {
            input.pointer = logical_pointer(renderer, event.button.x, event.button.y);
            if (event.button.button == SDL_BUTTON_LEFT) {
                input.pointer_released = true;
                input.pointer_down = false;
            }
        }
    }
    input.modifier_down = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
}

void consume_input(InputState& input) {
    input.toggle_debug_pressed = false;
    input.toggle_fullscreen_pressed = false;
    input.confirm_pressed = false;
    input.back_pressed = false;
    input.up_pressed = false;
    input.down_pressed = false;
    input.left_pressed = false;
    input.right_pressed = false;
    input.pointer_pressed = false;
    input.pointer_released = false;
    input.secondary_pressed = false;
}
