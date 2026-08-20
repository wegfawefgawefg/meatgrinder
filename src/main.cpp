#include "debug.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "input.hpp"
#include "level.hpp"
#include "progress.hpp"
#include "state.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;
constexpr float maximum_frame_seconds = 0.25F;

SDL_FRect presentation_rect(SDL_Renderer* renderer) {
    int width = layout_width;
    int height = layout_height;
    (void)SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
    const float scale = std::min(static_cast<float>(width) / static_cast<float>(layout_width),
                                 static_cast<float>(height) / static_cast<float>(layout_height));
    const float output_width = static_cast<float>(layout_width) * scale;
    const float output_height = static_cast<float>(layout_height) * scale;
    return {(static_cast<float>(width) - output_width) * 0.5F,
            (static_cast<float>(height) - output_height) * 0.5F, output_width, output_height};
}

} // namespace

int main(int argc, char** argv) {
    bool smoke = false;
    std::string capture_path;
    std::optional<int> preview_level;
    std::optional<int> preview_level_map;
    bool preview_world_map = false;
    int window_width = layout_width;
    int window_height = layout_height;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--smoke") == 0) smoke = true;
        else if (std::strcmp(argv[index], "--capture") == 0 && index + 1 < argc) capture_path = argv[++index];
        else if (std::strcmp(argv[index], "--level") == 0 && index + 1 < argc) preview_level = std::stoi(argv[++index]) - 1;
        else if (std::strcmp(argv[index], "--world-map") == 0) preview_world_map = true;
        else if (std::strcmp(argv[index], "--level-map") == 0 && index + 1 < argc) preview_level_map = std::stoi(argv[++index]) - 1;
        else if (std::strcmp(argv[index], "--width") == 0 && index + 1 < argc) window_width = std::stoi(argv[++index]);
        else if (std::strcmp(argv[index], "--height") == 0 && index + 1 < argc) window_height = std::stoi(argv[++index]);
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY;
    SDL_Window* window = SDL_CreateWindow("Meatgrinder", window_width, window_height, window_flags);
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
    SDL_Texture* scene = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET, layout_width, layout_height);
    if (scene == nullptr) {
        std::fprintf(stderr, "internal framebuffer creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    (void)SDL_SetTextureScaleMode(scene, SDL_SCALEMODE_NEAREST);
    if (!init_debug(window, renderer)) {
        std::fprintf(stderr, "ImGui init failed\n");
        SDL_DestroyTexture(scene);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    State state;
    std::string error;
    if (!load_campaign(std::string(MG_ASSET_ROOT) + "/levels/campaign.json", state.levels,
                       state.worlds, error)) {
        std::fprintf(stderr, "campaign load failed: %s\n", error.c_str());
        shutdown_debug();
        SDL_DestroyTexture(scene);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    char* preference_root = SDL_GetPrefPath("Meatgrinder", "Meatgrinder");
    const std::filesystem::path progress_path = preference_root != nullptr
                                                    ? std::filesystem::path{preference_root} / "progress.json"
                                                    : std::filesystem::path{"progress.json"};
    SDL_free(preference_root);
    if (!load_progress(progress_path, state, error)) {
        std::fprintf(stderr, "progress load failed: %s\n", error.c_str());
        error.clear();
        state.results.assign(state.levels.size(), {});
    }
    if (preview_level.has_value() && *preview_level >= 0 &&
        *preview_level < static_cast<int>(state.levels.size())) {
        start_level(state, *preview_level);
        change_mode(state, Mode::playing);
    } else if (preview_level_map.has_value() && *preview_level_map >= 0 &&
               *preview_level_map < static_cast<int>(state.worlds.size())) {
        state.selected_world = *preview_level_map;
        state.selected_level = state.worlds[static_cast<std::size_t>(*preview_level_map)].levels.front();
        change_mode(state, Mode::level_select);
    } else if (preview_world_map) {
        change_mode(state, Mode::world_select);
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
        if (state.progress_dirty) {
            if (!save_progress(progress_path, state, error)) {
                std::fprintf(stderr, "progress save failed: %s\n", error.c_str());
                error.clear();
            } else {
                state.progress_dirty = false;
            }
        }

        (void)SDL_SetRenderTarget(renderer, scene);
        draw(renderer, state, std::clamp(accumulator / step_seconds, 0.0F, 1.0F));
        (void)SDL_SetRenderTarget(renderer, nullptr);
        (void)SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        (void)SDL_RenderClear(renderer);
        const SDL_FRect output = presentation_rect(renderer);
        (void)SDL_RenderTexture(renderer, scene, nullptr, &output);
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
    SDL_DestroyTexture(scene);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
