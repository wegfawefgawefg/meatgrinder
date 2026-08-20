#include "input.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>

#include <algorithm>

namespace {

Vec2 logical_pointer(SDL_Renderer* renderer, float x, float y) {
    (void)SDL_RenderCoordinatesFromWindow(renderer, x, y, &x, &y);
    int output_width = layout_width;
    int output_height = layout_height;
    (void)SDL_GetCurrentRenderOutputSize(renderer, &output_width, &output_height);
    const float scale = std::min(static_cast<float>(output_width) / static_cast<float>(layout_width),
                                 static_cast<float>(output_height) / static_cast<float>(layout_height));
    const float left = (static_cast<float>(output_width) - static_cast<float>(layout_width) * scale) * 0.5F;
    const float top = (static_cast<float>(output_height) - static_cast<float>(layout_height) * scale) * 0.5F;
    return {(x - left) / scale, (y - top) / scale};
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
            if (event.key.key == SDLK_H) input.relocate_hq_pressed = true;
            if (event.key.key == SDLK_R) input.rally_orders_pressed = true;
            if (event.key.key == SDLK_C) input.clear_orders_pressed = true;
            if (event.key.key == SDLK_1) input.dispatch_choice = 0;
            if (event.key.key == SDLK_2) input.dispatch_choice = 1;
            if (event.key.key == SDLK_3) input.dispatch_choice = 2;
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
            }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && !ImGui::GetIO().WantCaptureMouse) {
            input.pointer = logical_pointer(renderer, event.button.x, event.button.y);
            if (event.button.button == SDL_BUTTON_LEFT) {
                input.pointer_released = true;
                input.pointer_down = false;
            }
        }
        if (event.type == SDL_EVENT_MOUSE_WHEEL && !ImGui::GetIO().WantCaptureMouse) {
            input.zoom_delta += event.wheel.y;
        }
    }
    input.direct_down = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
    input.additive_down = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    const bool* keys = SDL_GetKeyboardState(nullptr);
    input.pan_up = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    input.pan_down = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    input.pan_left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.pan_right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
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
    input.relocate_hq_pressed = false;
    input.rally_orders_pressed = false;
    input.clear_orders_pressed = false;
    input.dispatch_choice = -1;
    input.zoom_delta = 0.0F;
}
