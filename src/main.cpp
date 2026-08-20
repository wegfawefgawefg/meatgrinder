#include "debug.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "input.hpp"
#include "level.hpp"
#include "state.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;
constexpr float maximum_frame_seconds = 0.25F;

} // namespace

int main(int argc, char** argv) {
    bool smoke = false;
    std::string capture_path;
    std::optional<int> preview_level;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--smoke") == 0) smoke = true;
        else if (std::strcmp(argv[index], "--capture") == 0 && index + 1 < argc) capture_path = argv[++index];
        else if (std::strcmp(argv[index], "--level") == 0 && index + 1 < argc) preview_level = std::stoi(argv[++index]) - 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Meatgrinder", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::fprintf(stderr, "window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    (void)SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        std::fprintf(stderr, "renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    (void)SDL_SetRenderVSync(renderer, 1);
    (void)SDL_SetRenderLogicalPresentation(renderer, layout_width, layout_height,
                                           SDL_LOGICAL_PRESENTATION_LETTERBOX);
    if (!init_debug(window, renderer)) {
        std::fprintf(stderr, "ImGui init failed\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    State state;
    std::string error;
    if (!load_campaign(std::string(MG_ASSET_ROOT) + "/levels/campaign.json", state.levels, error)) {
        std::fprintf(stderr, "campaign load failed: %s\n", error.c_str());
        shutdown_debug();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (preview_level.has_value() && *preview_level >= 0 &&
        *preview_level < static_cast<int>(state.levels.size())) {
        start_level(state, *preview_level);
        change_mode(state, Mode::playing);
    }

    auto previous = Clock::now();
    float accumulator = 0.0F;
    int rendered_frames = 0;
    bool applied_fullscreen = false;
    while (!state.input.close_requested) {
        const auto now = Clock::now();
        const float elapsed = std::chrono::duration<float>(now - previous).count();
        previous = now;
        accumulator += std::min(elapsed, maximum_frame_seconds);

        pump_input(state.input, renderer);
        if (state.input.toggle_debug_pressed) state.debug_open = !state.debug_open;
        if (state.input.toggle_fullscreen_pressed) state.fullscreen = !state.fullscreen;
        state.input.toggle_debug_pressed = false;
        state.input.toggle_fullscreen_pressed = false;
        if (state.fullscreen != applied_fullscreen) {
            (void)SDL_SetWindowFullscreen(window, state.fullscreen);
            applied_fullscreen = state.fullscreen;
        }
        while (accumulator >= step_seconds) {
            step(state);
            consume_input(state.input);
            accumulator -= step_seconds;
        }

        draw(renderer, state, std::clamp(accumulator / step_seconds, 0.0F, 1.0F));
        draw_debug(state, renderer);
        if (!capture_path.empty()) {
            SDL_Surface* capture = SDL_RenderReadPixels(renderer, nullptr);
            if (capture == nullptr || !SDL_SaveBMP(capture, capture_path.c_str())) {
                std::fprintf(stderr, "capture failed: %s\n", SDL_GetError());
            }
            SDL_DestroySurface(capture);
            state.input.close_requested = true;
        }
        SDL_RenderPresent(renderer);
        ++rendered_frames;
        if (smoke && rendered_frames >= 4) state.input.close_requested = true;
        if (!smoke) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    shutdown_debug();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
