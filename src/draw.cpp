#include "draw.hpp"

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

void fill(SDL_Renderer* renderer, float x, float y, float w, float h) {
    const SDL_FRect rect{x, y, w, h};
    (void)SDL_RenderFillRect(renderer, &rect);
}

void outline(SDL_Renderer* renderer, float x, float y, float w, float h) {
    const SDL_FRect rect{x, y, w, h};
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

void draw_button(SDL_Renderer* renderer, float y, std::string_view label, bool selected) {
    color(renderer, selected ? 196 : 53, selected ? 55 : 61, selected ? 48 : 67);
    fill(renderer, 465.0F, y, 350.0F, 50.0F);
    color(renderer, selected ? 255 : 172, selected ? 235 : 178, selected ? 211 : 184);
    outline(renderer, 465.0F, y, 350.0F, 50.0F);
    centered_text(renderer, y + 17.0F, label);
}

const char* kind_mark(NodeKind kind) {
    switch (kind) {
    case NodeKind::castle: return "C";
    case NodeKind::stable: return "S";
    case NodeKind::port: return "P";
    case NodeKind::cannon: return "A";
    case NodeKind::fort: return "F";
    case NodeKind::mine: return "M";
    case NodeKind::route: return "+";
    }
    return "+";
}

void owner_color(SDL_Renderer* renderer, int owner, bool muted = false) {
    if (owner == player_owner) color(renderer, muted ? 51 : 65, muted ? 113 : 176, muted ? 130 : 205);
    else if (owner == enemy_owner) color(renderer, muted ? 135 : 212, muted ? 57 : 69, muted ? 54 : 55);
    else color(renderer, muted ? 103 : 151, muted ? 100 : 145, muted ? 90 : 126);
}

void draw_tiles(SDL_Renderer* renderer, const Level& level) {
    const float tile_w = static_cast<float>(layout_width) / static_cast<float>(level.tiles.front().size());
    const float tile_h = static_cast<float>(layout_height) / static_cast<float>(level.tiles.size());
    for (std::size_t y = 0; y < level.tiles.size(); ++y) {
        for (std::size_t x = 0; x < level.tiles[y].size(); ++x) {
            const char tile = level.tiles[y][x];
            if (tile == '2') color(renderer, 39, 58, 56);
            else if (tile == '1') color(renderer, 50, 57, 46);
            else color(renderer, 43, 46, 39);
            fill(renderer, static_cast<float>(x) * tile_w, static_cast<float>(y) * tile_h,
                 tile_w + 1.0F, tile_h + 1.0F);
        }
    }
}

void draw_routes(SDL_Renderer* renderer, const Level& level) {
    color(renderer, 116, 109, 91);
    for (const Link& link : level.links) {
        const Vec2 a = node_screen_position(level, link.a);
        const Vec2 b = node_screen_position(level, link.b);
        (void)SDL_RenderLine(renderer, a.x, a.y, b.x, b.y);
        (void)SDL_RenderLine(renderer, a.x + 1.0F, a.y, b.x + 1.0F, b.y);
    }
}

void draw_nodes(SDL_Renderer* renderer, const Level& level, const Match& match) {
    char count[32];
    for (const NodeState& node : match.nodes) {
        const Vec2 point = node_screen_position(level, node.id);
        owner_color(renderer, node.owner, true);
        fill(renderer, point.x - 20.0F, point.y - 16.0F, 40.0F, 32.0F);
        owner_color(renderer, node.owner);
        outline(renderer, point.x - 20.0F, point.y - 16.0F, 40.0F, 32.0F);
        if (node.selected) {
            color(renderer, 255, 228, 113);
            outline(renderer, point.x - 25.0F, point.y - 21.0F, 50.0F, 42.0F);
        }
        color(renderer, 242, 236, 212);
        text(renderer, point.x - 16.0F, point.y - 11.0F, kind_mark(node.kind), 1.0F);
        std::snprintf(count, sizeof(count), "%d", static_cast<int>(node.soldiers));
        text(renderer, point.x - 3.0F, point.y - 11.0F, count, 1.0F);
        if (node.tier > 1) {
            std::snprintf(count, sizeof(count), "^%d", node.tier);
            text(renderer, point.x - 9.0F, point.y + 20.0F, count, 1.0F);
        }
    }
}

void draw_armies(SDL_Renderer* renderer, const Level& level, const Match& match, float alpha) {
    char count[16];
    for (const Army& army : match.armies) {
        const Vec2 a = node_screen_position(level, army.path[static_cast<std::size_t>(army.leg)]);
        const Vec2 b = node_screen_position(level, army.path[static_cast<std::size_t>(army.leg + 1)]);
        const float progress = std::lerp(army.previous_progress, army.progress, alpha);
        const Vec2 point{std::lerp(a.x, b.x, progress), std::lerp(a.y, b.y, progress)};
        owner_color(renderer, army.owner);
        fill(renderer, point.x - 7.0F, point.y - 7.0F, 14.0F, 14.0F);
        std::snprintf(count, sizeof(count), "%d", static_cast<int>(army.soldiers));
        text(renderer, point.x + 9.0F, point.y - 5.0F, count, 1.0F);
    }
}

void draw_playing(SDL_Renderer* renderer, const State& state, float alpha) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    draw_tiles(renderer, level);
    draw_routes(renderer, level);
    draw_armies(renderer, level, state.match, alpha);
    draw_nodes(renderer, level, state.match);
    color(renderer, 231, 224, 196);
    char status[128];
    std::snprintf(status, sizeof(status), "LEVEL %d   TIME %02d:%02d   SCORE %d",
                  state.campaign_level + 1, static_cast<int>(state.match.stats.elapsed_seconds) / 60,
                  static_cast<int>(state.match.stats.elapsed_seconds) % 60, state.campaign_score);
    text(renderer, 24.0F, 18.0F, status, 1.5F);
    text(renderer, 24.0F, 688.0F, "DRAG SELECT  |  CLICK TARGET  |  SHIFT-CLICK PROMOTE", 1.0F);
    if (state.input.pointer_down) {
        color(renderer, 255, 228, 113);
        const Vec2 a = state.input.press_origin;
        const Vec2 b = state.input.pointer;
        outline(renderer, std::min(a.x, b.x), std::min(a.y, b.y), std::abs(a.x - b.x),
                std::abs(a.y - b.y));
    }
}

void draw_main_menu(SDL_Renderer* renderer, const State& state) {
    color(renderer, 30, 31, 29);
    (void)SDL_RenderClear(renderer);
    color(renderer, 232, 220, 190);
    centered_text(renderer, 155.0F, "MEATGRINDER", 4.0F);
    color(renderer, 164, 156, 137);
    centered_text(renderer, 225.0F, "A TERRITORIAL WAR GAME", 1.5F);
    draw_button(renderer, 330.0F, "PLAY", state.menu_choice == 0);
    draw_button(renderer, 395.0F, "OPTIONS", state.menu_choice == 1);
    draw_button(renderer, 460.0F, "QUIT", state.menu_choice == 2);
}

void draw_options(SDL_Renderer* renderer, const State& state) {
    color(renderer, 30, 31, 29);
    (void)SDL_RenderClear(renderer);
    color(renderer, 232, 220, 190);
    centered_text(renderer, 170.0F, "OPTIONS", 3.0F);
    draw_button(renderer, 310.0F,
                state.rules.enemy_aggression < 0.55F ? "ENEMY: DELIBERATE" : "ENEMY: AGGRESSIVE",
                state.options_choice == 0);
    draw_button(renderer, 375.0F, state.fullscreen ? "FULLSCREEN: ON" : "FULLSCREEN: OFF",
                state.options_choice == 1);
    draw_button(renderer, 440.0F, "BACK", state.options_choice == 2);
}

void draw_level_card(SDL_Renderer* renderer, const State& state) {
    color(renderer, 28, 29, 27);
    (void)SDL_RenderClear(renderer);
    const float t = state.mode_seconds;
    float x = 0.0F;
    if (t < 0.35F) x = std::lerp(static_cast<float>(layout_width), 0.0F, t / 0.35F);
    else if (t > 1.25F) x = std::lerp(0.0F, -static_cast<float>(layout_width), (t - 1.25F) / 0.35F);
    color(renderer, 190, 52, 45);
    fill(renderer, x, 260.0F, static_cast<float>(layout_width), 200.0F);
    color(renderer, 255, 237, 215);
    char title[64];
    std::snprintf(title, sizeof(title), "LEVEL %d", state.campaign_level + 1);
    const float width = static_cast<float>(std::string_view(title).size()) * 8.0F * 4.0F;
    text(renderer, x + (static_cast<float>(layout_width) - width) * 0.5F, 325.0F, title, 4.0F);
}

void draw_overlay_screen(SDL_Renderer* renderer, const State& state) {
    color(renderer, 30, 31, 29);
    (void)SDL_RenderClear(renderer);
    color(renderer, 232, 220, 190);
    if (state.mode == Mode::paused) {
        centered_text(renderer, 220.0F, "PAUSED", 4.0F);
        centered_text(renderer, 350.0F, "PRESS ENTER OR ESCAPE TO RETURN", 1.5F);
    } else if (state.mode == Mode::defeat) {
        centered_text(renderer, 180.0F, "THE LINE BROKE", 3.0F);
        draw_button(renderer, 340.0F, "RETRY", state.menu_choice == 0);
        draw_button(renderer, 405.0F, "QUIT TO MENU", state.menu_choice == 1);
    } else if (state.mode == Mode::score) {
        centered_text(renderer, 105.0F, "FRONT SECURED", 3.0F);
        char line[128];
        std::snprintf(line, sizeof(line), "TIME                 %02d:%02d",
                      static_cast<int>(state.match.stats.elapsed_seconds) / 60,
                      static_cast<int>(state.match.stats.elapsed_seconds) % 60);
        text(renderer, 430.0F, 250.0F, line, 2.0F);
        std::snprintf(line, sizeof(line), "ORDERS               %d", state.match.stats.orders);
        text(renderer, 430.0F, 295.0F, line, 2.0F);
        std::snprintf(line, sizeof(line), "SOLDIERS SENT        %d", state.match.stats.soldiers_sent);
        text(renderer, 430.0F, 340.0F, line, 2.0F);
        std::snprintf(line, sizeof(line), "GROUND TAKEN         %d", state.match.stats.nodes_captured);
        text(renderer, 430.0F, 385.0F, line, 2.0F);
        std::snprintf(line, sizeof(line), "CAMPAIGN SCORE       %d", state.campaign_score);
        text(renderer, 430.0F, 445.0F, line, 2.0F);
        centered_text(renderer, 585.0F, "PRESS TO CONTINUE", 1.5F);
    } else {
        centered_text(renderer, 170.0F, "THE WAR IS OVER", 3.0F);
        centered_text(renderer, 300.0F, "EVERY FRONT SECURED", 2.0F);
        char line[96];
        std::snprintf(line, sizeof(line), "FINAL SCORE  %d", state.campaign_score);
        centered_text(renderer, 390.0F, line, 2.0F);
        centered_text(renderer, 560.0F, "PRESS TO RETURN", 1.5F);
    }
}

} // namespace

void draw(SDL_Renderer* renderer, const State& state, float alpha) {
    if (state.mode == Mode::main_menu) draw_main_menu(renderer, state);
    else if (state.mode == Mode::options) draw_options(renderer, state);
    else if (state.mode == Mode::level_card) draw_level_card(renderer, state);
    else if (state.mode == Mode::playing) draw_playing(renderer, state, alpha);
    else draw_overlay_screen(renderer, state);
}

