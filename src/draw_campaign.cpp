#include "draw_campaign.hpp"

#include "campaign.hpp"

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

Vec2 zoomed(Vec2 point, Vec2 focus, float amount, float maximum) {
    const Vec2 center{static_cast<float>(layout_width) * 0.5F,
                      static_cast<float>(layout_height) * 0.5F};
    const float transition = (amount - 1.0F) / std::max(0.1F, maximum - 1.0F);
    const Vec2 anchor{std::lerp(focus.x, center.x, transition),
                      std::lerp(focus.y, center.y, transition)};
    return {(point.x - focus.x) * amount + anchor.x, (point.y - focus.y) * amount + anchor.y};
}

void line(SDL_Renderer* renderer, Vec2 a, Vec2 b, float width = 2.0F) {
    for (int offset = 0; offset < static_cast<int>(width); ++offset) {
        const float shift = static_cast<float>(offset) - width * 0.5F;
        (void)SDL_RenderLine(renderer, a.x, a.y + shift, b.x, b.y + shift);
    }
}

void world_select(SDL_Renderer* renderer, const State& state, bool transitioning) {
    color(renderer, 27, 31, 30);
    (void)SDL_RenderClear(renderer);
    const Vec2 focus = world_map_position(state.worlds[static_cast<std::size_t>(state.selected_world)]);
    const float t = transitioning ? smooth(state.mode_seconds / 0.7F) : 0.0F;
    const float zoom = std::lerp(1.0F, 4.0F, t);
    color(renderer, 76, 83, 72);
    for (std::size_t index = 1; index < state.worlds.size(); ++index) {
        const Vec2 a = zoomed(world_map_position(state.worlds[index - 1]), focus, zoom, 4.0F);
        const Vec2 b = zoomed(world_map_position(state.worlds[index]), focus, zoom, 4.0F);
        line(renderer, a, b, 3.0F);
    }
    for (int index = 0; index < static_cast<int>(state.worlds.size()); ++index) {
        const World& world = state.worlds[static_cast<std::size_t>(index)];
        const Vec2 point = zoomed(world_map_position(world), focus, zoom, 4.0F);
        const bool unlocked = world_available(state, index);
        const bool complete = world_completed(state, index);
        const bool selected = index == state.selected_world;
        if (!unlocked) color(renderer, 57, 59, 57);
        else if (complete) color(renderer, 62, 147, 91);
        else if (selected) color(renderer, 222, 190, 76);
        else color(renderer, 153, 64, 55);
        const float size = selected ? 78.0F : 62.0F;
        fill(renderer, point.x - size * 0.5F, point.y - size * 0.5F, size, size);
        color(renderer, 236, 226, 199);
        outline(renderer, point.x - size * 0.5F, point.y - size * 0.5F, size, size);
        char number[16];
        std::snprintf(number, sizeof(number), "%d", index + 1);
        text(renderer, point.x - 8.0F, point.y - 12.0F, number, 2.0F);
        if (!transitioning) {
            const float label_width = static_cast<float>(world.name.size()) * 8.0F;
            text(renderer, point.x - label_width * 0.5F, point.y + 42.0F, world.name, 1.0F);
        }
    }
    if (transitioning) return;
    color(renderer, 235, 226, 200);
    centered_text(renderer, 28.0F, "SELECT WORLD", 2.5F);
    const World& selected = state.worlds[static_cast<std::size_t>(state.selected_world)];
    color(renderer, 22, 24, 23, 240);
    fill(renderer, 300.0F, 615.0F, 680.0F, 72.0F);
    color(renderer, 229, 216, 181);
    centered_text(renderer, 630.0F, selected.theme, 1.5F);
    centered_text(renderer, 660.0F,
                  world_available(state, state.selected_world) ? "ENTER TO DEPLOY" : "LOCKED", 1.0F);
    text(renderer, 24.0F, 690.0F, "ESC BACK", 1.0F);
}

void level_links(SDL_Renderer* renderer, const State& state, const World& world, Vec2 focus,
                 float zoom) {
    color(renderer, 92, 91, 76);
    for (int index : world.levels) {
        const Level& level = state.levels[static_cast<std::size_t>(index)];
        const Vec2 to = zoomed(level_map_position(level), focus, zoom, 5.0F);
        for (const std::string& requirement : level.prerequisites) {
            const int required = level_index(state, requirement);
            if (required < 0 || state.levels[static_cast<std::size_t>(required)].world != level.world) continue;
            const Vec2 from = zoomed(level_map_position(state.levels[static_cast<std::size_t>(required)]),
                                     focus, zoom, 5.0F);
            line(renderer, from, to, 3.0F);
        }
    }
}

void level_select(SDL_Renderer* renderer, const State& state, bool transitioning) {
    color(renderer, 31, 34, 30);
    (void)SDL_RenderClear(renderer);
    const World& world = state.worlds[static_cast<std::size_t>(state.selected_world)];
    const Level& selected_level = state.levels[static_cast<std::size_t>(state.selected_level)];
    const Vec2 focus = level_map_position(selected_level);
    const float t = transitioning ? smooth(state.mode_seconds / 0.7F) : 0.0F;
    const float zoom = std::lerp(1.0F, 5.0F, t);
    level_links(renderer, state, world, focus, zoom);
    for (int index : world.levels) {
        const Level& level = state.levels[static_cast<std::size_t>(index)];
        const LevelResult& result = state.results[static_cast<std::size_t>(index)];
        const bool available = level_available(state, index);
        const bool selected = index == state.selected_level;
        const Vec2 point = zoomed(level_map_position(level), focus, zoom, 5.0F);
        if (result.completed) color(renderer, 55, 157, 89);
        else if (!available) color(renderer, 66, 68, 65);
        else if (selected) color(renderer, 224, 190, 72);
        else color(renderer, 181, 67, 56);
        const float size = selected ? 54.0F : 44.0F;
        fill(renderer, point.x - size * 0.5F, point.y - size * 0.5F, size, size);
        color(renderer, 239, 230, 203);
        outline(renderer, point.x - size * 0.5F, point.y - size * 0.5F, size, size);
        text(renderer, point.x - 12.0F, point.y - 5.0F, level.id, 1.0F);
        if (result.completed) text(renderer, point.x + 13.0F, point.y - 25.0F, "CHECK", 0.75F);
        if (!transitioning) {
            const float label_width = static_cast<float>(level.name.size()) * 8.0F;
            text(renderer, point.x - label_width * 0.5F, point.y + 33.0F, level.name, 1.0F);
        }
    }
    if (transitioning) return;
    color(renderer, 235, 226, 200);
    centered_text(renderer, 28.0F, world.name, 2.5F);
    color(renderer, 22, 24, 23, 242);
    fill(renderer, 245.0F, 590.0F, 790.0F, 100.0F);
    color(renderer, 230, 219, 187);
    centered_text(renderer, 606.0F, selected_level.briefing, 1.0F);
    const LevelResult& result = state.results[static_cast<std::size_t>(state.selected_level)];
    char record[128];
    if (result.completed) {
        std::snprintf(record, sizeof(record), "BEST SCORE %d  /  BEST TIME %02d:%02d",
                      result.best_score, static_cast<int>(result.best_seconds) / 60,
                      static_cast<int>(result.best_seconds) % 60);
    } else {
        std::snprintf(record, sizeof(record), "%s",
                      level_available(state, state.selected_level) ? "ENTER TO DEPLOY" : "LOCKED");
    }
    centered_text(renderer, 654.0F, record, 1.0F);
    text(renderer, 24.0F, 690.0F, "ESC WORLDS", 1.0F);
}

} // namespace

void draw_campaign_screen(SDL_Renderer* renderer, const State& state) {
    if (state.mode == Mode::world_select || state.mode == Mode::world_zoom ||
        state.mode == Mode::world_unlock) {
        world_select(renderer, state, state.mode == Mode::world_zoom);
        if (state.mode == Mode::world_unlock) {
            color(renderer, 20, 22, 21, 245);
            fill(renderer, 310.0F, 265.0F, 660.0F, 170.0F);
            color(renderer, 229, 190, 71);
            centered_text(renderer, 300.0F, "NEW WORLD UNLOCKED", 2.5F);
            centered_text(renderer, 365.0F,
                          state.worlds[static_cast<std::size_t>(state.selected_world)].name, 2.0F);
            centered_text(renderer, 407.0F, "PRESS TO ENTER", 1.0F);
        }
    } else {
        level_select(renderer, state, state.mode == Mode::level_zoom);
    }
}
