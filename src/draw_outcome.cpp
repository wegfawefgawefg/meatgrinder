#include "draw_outcome.hpp"

#include "camera.hpp"
#include "game.hpp"
#include "level.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

void color(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    (void)SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void fill(SDL_Renderer* renderer, float x, float y, float width, float height) {
    const SDL_FRect rect{x, y, width, height};
    (void)SDL_RenderFillRect(renderer, &rect);
}

void outline(SDL_Renderer* renderer, float x, float y, float width, float height) {
    const SDL_FRect rect{x, y, width, height};
    (void)SDL_RenderRect(renderer, &rect);
}

void thick_line(SDL_Renderer* renderer, Vec2 a, Vec2 b, int thickness) {
    for (int offset = 0; offset < thickness; ++offset) {
        const float shift = static_cast<float>(offset) - static_cast<float>(thickness) * 0.5F;
        (void)SDL_RenderLine(renderer, a.x, a.y + shift, b.x, b.y + shift);
    }
}

void text(SDL_Renderer* renderer, float x, float y, std::string_view value, float scale = 2.0F) {
    (void)SDL_SetRenderScale(renderer, scale, scale);
    (void)SDL_RenderDebugText(renderer, x / scale, y / scale, std::string(value).c_str());
    (void)SDL_SetRenderScale(renderer, 1.0F, 1.0F);
}

void centered_text(SDL_Renderer* renderer, float y, std::string_view value, float scale = 2.0F) {
    const float width = static_cast<float>(value.size()) * 8.0F * scale;
    text(renderer, (static_cast<float>(layout_width) - width) * 0.5F, y, value, scale);
}

float smooth(float value) {
    value = std::clamp(value, 0.0F, 1.0F);
    return value * value * (3.0F - 2.0F * value);
}

float score_panel_bottom(float seconds) {
    if (seconds < 0.38F) {
        const float phase = std::clamp(seconds / 0.38F, 0.0F, 1.0F);
        return static_cast<float>(layout_height) * phase * phase * phase;
    }
    if (seconds < 0.54F) {
        return std::lerp(static_cast<float>(layout_height),
                         static_cast<float>(layout_height) - 34.0F,
                         smooth((seconds - 0.38F) / 0.16F));
    }
    if (seconds < 0.72F) {
        return std::lerp(static_cast<float>(layout_height) - 34.0F,
                         static_cast<float>(layout_height),
                         smooth((seconds - 0.54F) / 0.18F));
    }
    return static_cast<float>(layout_height);
}

int counted_value(int target, float seconds, int row) {
    const float start = 0.88F + static_cast<float>(row) * 0.20F;
    return static_cast<int>(std::lround(static_cast<float>(target) *
                            smooth((seconds - start) / 0.48F)));
}

bool row_visible(float seconds, int row) {
    return seconds >= 0.88F + static_cast<float>(row) * 0.20F;
}

} // namespace

void draw_victory_effect(SDL_Renderer* renderer, const State& state, float alpha) {
    if (state.match.defeated_headquarters < 0) return;
    const float seconds = state.mode_seconds + alpha * step_seconds;
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    const Vec2 center = interpolated_camera_center(state.camera, alpha);
    const float zoom = interpolated_camera_zoom(state.camera, alpha);
    const Vec2 point = world_to_screen(
        node_world_position(level, state.match.defeated_headquarters), center, zoom);
    const float burst = smooth(seconds / 0.72F);

    for (int ray = 0; ray < 14; ++ray) {
        const float angle = static_cast<float>(ray) * 6.2831853F / 14.0F;
        const float inner = 18.0F + burst * 24.0F;
        const float outer = 24.0F + burst * (82.0F + static_cast<float>(ray % 3) * 13.0F);
        color(renderer, ray % 2 == 0 ? 255 : 218, ray % 2 == 0 ? 218 : 74, 72);
        thick_line(renderer,
                   {point.x + std::cos(angle) * inner, point.y + std::sin(angle) * inner},
                   {point.x + std::cos(angle) * outer, point.y + std::sin(angle) * outer}, 4);
    }

    for (int piece = 0; piece < 18; ++piece) {
        const float angle = static_cast<float>(piece) * 6.2831853F / 18.0F + 0.17F;
        const float speed = 66.0F + static_cast<float>((piece * 17) % 55);
        const float travel = std::min(seconds, 1.25F);
        const float x = point.x + std::cos(angle) * speed * travel;
        const float y = point.y + std::sin(angle) * speed * travel + 58.0F * travel * travel;
        if (piece % 3 == 0) color(renderer, 255, 228, 126);
        else if (piece % 3 == 1) color(renderer, 190, 52, 45);
        else color(renderer, 63, 59, 52);
        const float size = 5.0F + static_cast<float>(piece % 4) * 2.0F;
        fill(renderer, x - size * 0.5F, y - size * 0.5F, size, size);
    }

    const float ring = 28.0F + burst * 105.0F;
    color(renderer, 255, 235, 185);
    outline(renderer, point.x - ring, point.y - ring, ring * 2.0F, ring * 2.0F);
    if (seconds < 0.24F) {
        const float core = std::max(0.0F, 58.0F * (1.0F - seconds / 0.24F));
        color(renderer, 255, 246, 213);
        fill(renderer, point.x - core, point.y - core, core * 2.0F, core * 2.0F);
    }
}

void draw_score_screen(SDL_Renderer* renderer, const State& state, float alpha) {
    const float seconds = state.mode_seconds + alpha * step_seconds;
    const float bottom = score_panel_bottom(seconds);
    color(renderer, 30, 31, 29);
    fill(renderer, 0.0F, 0.0F, static_cast<float>(layout_width), bottom);
    color(renderer, 190, 52, 45);
    fill(renderer, 0.0F, std::max(0.0F, bottom - 10.0F), static_cast<float>(layout_width), 10.0F);
    if (seconds < 0.38F) return;

    const float bounce = bottom - static_cast<float>(layout_height);
    color(renderer, 232, 220, 190);
    centered_text(renderer, 105.0F + bounce, "FRONT SECURED", 3.0F);
    char line[128];
    if (row_visible(seconds, 0)) {
        const int elapsed = counted_value(static_cast<int>(state.match.stats.elapsed_seconds),
                                          seconds, 0);
        std::snprintf(line, sizeof(line), "TIME                 %02d:%02d", elapsed / 60,
                      elapsed % 60);
        text(renderer, 430.0F, 250.0F, line, 2.0F);
    }
    if (row_visible(seconds, 1)) {
        std::snprintf(line, sizeof(line), "ORDERS               %d",
                      counted_value(state.match.stats.orders, seconds, 1));
        text(renderer, 430.0F, 295.0F, line, 2.0F);
    }
    if (row_visible(seconds, 2)) {
        std::snprintf(line, sizeof(line), "SOLDIERS SENT        %d",
                      counted_value(state.match.stats.soldiers_sent, seconds, 2));
        text(renderer, 430.0F, 340.0F, line, 2.0F);
    }
    if (row_visible(seconds, 3)) {
        std::snprintf(line, sizeof(line), "GROUND TAKEN         %d",
                      counted_value(state.match.stats.nodes_captured, seconds, 3));
        text(renderer, 430.0F, 385.0F, line, 2.0F);
    }
    if (row_visible(seconds, 4)) {
        std::snprintf(line, sizeof(line), "GEN CYCLES           %d",
                      counted_value(state.match.stats.generations, seconds, 4));
        text(renderer, 430.0F, 430.0F, line, 2.0F);
    }
    if (row_visible(seconds, 5)) {
        std::snprintf(line, sizeof(line), "LEVEL SCORE          %d",
                      counted_value(state.last_level_score, seconds, 5));
        text(renderer, 430.0F, 485.0F, line, 2.0F);
    }
    if (seconds >= score_input_seconds) {
        centered_text(renderer, 585.0F, "PRESS TO CONTINUE", 1.5F);
    }
}
