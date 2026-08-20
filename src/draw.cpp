#include "draw.hpp"

#include "camera.hpp"
#include "draw_campaign.hpp"
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

void fill(SDL_Renderer* renderer, float x, float y, float w, float h) {
    const SDL_FRect rect{x, y, w, h};
    (void)SDL_RenderFillRect(renderer, &rect);
}

void outline(SDL_Renderer* renderer, float x, float y, float w, float h) {
    const SDL_FRect rect{x, y, w, h};
    (void)SDL_RenderRect(renderer, &rect);
}

void thick_outline(SDL_Renderer* renderer, float x, float y, float w, float h, int thickness) {
    for (int inset = 0; inset < thickness; ++inset) {
        const float value = static_cast<float>(inset);
        outline(renderer, x + value, y + value, std::max(0.0F, w - value * 2.0F),
                std::max(0.0F, h - value * 2.0F));
    }
}

void thick_line(SDL_Renderer* renderer, Vec2 a, Vec2 b, float thickness) {
    const float length = std::max(1.0F, std::hypot(b.x - a.x, b.y - a.y));
    const Vec2 normal{-(b.y - a.y) / length, (b.x - a.x) / length};
    const int lines = std::max(1, static_cast<int>(std::ceil(thickness)));
    const float start = -static_cast<float>(lines - 1) * 0.5F;
    for (int index = 0; index < lines; ++index) {
        const float offset = start + static_cast<float>(index);
        (void)SDL_RenderLine(renderer, a.x + normal.x * offset, a.y + normal.y * offset,
                             b.x + normal.x * offset, b.y + normal.y * offset);
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

void draw_button(SDL_Renderer* renderer, float y, std::string_view label, bool selected) {
    color(renderer, selected ? 196 : 53, selected ? 55 : 61, selected ? 48 : 67);
    fill(renderer, 465.0F, y, 350.0F, 50.0F);
    color(renderer, selected ? 255 : 172, selected ? 235 : 178, selected ? 211 : 184);
    outline(renderer, 465.0F, y, 350.0F, 50.0F);
    centered_text(renderer, y + 17.0F, label);
}

const char* kind_mark(NodeKind kind) {
    switch (kind) {
    case NodeKind::producer: return "P";
    case NodeKind::stable: return "S";
    case NodeKind::port: return "P";
    case NodeKind::cannon: return "A";
    case NodeKind::fort: return "F";
    case NodeKind::mine: return "M";
    case NodeKind::node: return "+";
    }
    return "+";
}

const char* ai_style_name(AiStyle style) {
    switch (style) {
    case AiStyle::balanced: return "BALANCED";
    case AiStyle::aggressive: return "AGGRESSIVE";
    case AiStyle::turtle: return "TURTLE";
    case AiStyle::swarm: return "SWARM";
    }
    return "BALANCED";
}

const char* ai_difficulty_name(AiDifficulty difficulty) {
    if (difficulty == AiDifficulty::weak) return "WEAK";
    if (difficulty == AiDifficulty::hard) return "HARD";
    return "NORMAL";
}

const char* dispatch_name(DispatchMode mode) {
    if (mode == DispatchMode::one) return "1";
    if (mode == DispatchMode::half) return "HALF";
    return "ALL";
}

const char* rally_mark(const NodeState& node) {
    if (node.rally_dispatch == DispatchMode::one) return node.rally_assault ? "1A" : "1D";
    if (node.rally_dispatch == DispatchMode::half) return node.rally_assault ? "HA" : "HD";
    return node.rally_assault ? "AA" : "AD";
}

void owner_color(SDL_Renderer* renderer, int owner, bool muted = false) {
    if (owner == player_owner) color(renderer, muted ? 51 : 65, muted ? 113 : 176, muted ? 130 : 205);
    else if (owner == enemy_owner) color(renderer, muted ? 135 : 212, muted ? 57 : 69, muted ? 54 : 55);
    else color(renderer, muted ? 103 : 151, muted ? 100 : 145, muted ? 90 : 126);
}

void draw_tiles(SDL_Renderer* renderer, const Level& level, Vec2 center, float zoom) {
    constexpr float tile_size = 64.0F;
    const int source_height = static_cast<int>(level.tiles.size());
    const int source_width = static_cast<int>(level.tiles.front().size());
    for (int y = -16; y < 28; ++y) {
        for (int x = -20; x < 40; ++x) {
            const int source_y = (y % source_height + source_height) % source_height;
            const int source_x = (x % source_width + source_width) % source_width;
            const char tile = level.tiles[static_cast<std::size_t>(source_y)]
                                         [static_cast<std::size_t>(source_x)];
            if (tile == '2') color(renderer, 39, 58, 56);
            else if (tile == '1') color(renderer, 50, 57, 46);
            else color(renderer, 43, 46, 39);
            const Vec2 point = world_to_screen(
                {static_cast<float>(x) * tile_size, static_cast<float>(y) * tile_size}, center, zoom);
            fill(renderer, point.x, point.y, tile_size * zoom + 1.0F, tile_size * zoom + 1.0F);
        }
    }
}

void draw_routes(SDL_Renderer* renderer, const Level& level, Vec2 center, float zoom) {
    color(renderer, 116, 109, 91);
    for (const Link& link : level.links) {
        const Vec2 a = world_to_screen(node_world_position(level, link.a), center, zoom);
        const Vec2 b = world_to_screen(node_world_position(level, link.b), center, zoom);
        thick_line(renderer, a, b, 2.0F);
    }
    color(renderer, 62, 132, 153);
    for (const Link& link : level.sea_links) {
        const Vec2 a = world_to_screen(node_world_position(level, link.a), center, zoom);
        const Vec2 b = world_to_screen(node_world_position(level, link.b), center, zoom);
        for (int segment = 0; segment < 9; segment += 2) {
            const float begin = static_cast<float>(segment) / 9.0F;
            const float end = static_cast<float>(segment + 1) / 9.0F;
            thick_line(renderer, {std::lerp(a.x, b.x, begin), std::lerp(a.y, b.y, begin)},
                       {std::lerp(a.x, b.x, end), std::lerp(a.y, b.y, end)}, 3.0F);
        }
    }
}

void draw_rallies(SDL_Renderer* renderer, const Level& level, const State& state, Vec2 center,
                  float zoom) {
    color(renderer, 87, 205, 207);
    for (const NodeState& node : state.match.nodes) {
        if (node.rally_target < 0) continue;
        if (node.rally_assault) color(renderer, 87, 205, 207);
        else color(renderer, 239, 157, 77);
        const std::vector<int> path = find_route(state, node.owner, node.id, node.rally_target);
        for (std::size_t index = 1; index < path.size(); ++index) {
            const Vec2 a = world_to_screen(node_world_position(level, path[index - 1]), center, zoom);
            const Vec2 b = world_to_screen(node_world_position(level, path[index]), center, zoom);
            thick_line(renderer, a, b, 3.0F);
            const Vec2 mid{std::lerp(a.x, b.x, 0.58F), std::lerp(a.y, b.y, 0.58F)};
            fill(renderer, mid.x - 3.0F, mid.y - 3.0F, 6.0F, 6.0F);
        }
        const Vec2 source = world_to_screen(node_world_position(level, node.id), center, zoom);
        text(renderer, source.x - 8.0F, source.y + 24.0F, rally_mark(node), 1.0F);
    }
}

void draw_pending_rallies(SDL_Renderer* renderer, const State& state, const Level& level,
                          Vec2 center, float zoom) {
    if (state.rally_sources.empty()) return;
    const int target = node_at_pointer(state, state.input.pointer);
    if (state.input.direct_down) color(renderer, 239, 157, 77);
    else color(renderer, 255, 228, 113);
    for (int source_id : state.rally_sources) {
        const Vec2 source = world_to_screen(node_world_position(level, source_id), center, zoom);
        const std::vector<int> path = target >= 0 ? find_route(state, player_owner, source_id, target)
                                                  : std::vector<int>{};
        if (path.size() >= 2) {
            for (std::size_t index = 1; index < path.size(); ++index) {
                const Vec2 a = world_to_screen(node_world_position(level, path[index - 1]), center,
                                               zoom);
                const Vec2 b = world_to_screen(node_world_position(level, path[index]), center,
                                               zoom);
                thick_line(renderer, a, b, 4.0F);
            }
        } else if (target < 0) {
            thick_line(renderer, source, state.input.pointer, 2.0F);
        }
        text(renderer, source.x - 4.0F, source.y - 43.0F, "R", 1.0F);
    }
}

void draw_node_shape(SDL_Renderer* renderer, const NodeState& node, Vec2 point, float size) {
    owner_color(renderer, node.owner, true);
    const float left = point.x - size * 0.5F;
    const float top = point.y - size * 0.4F;
    if (node.kind == NodeKind::node) {
        fill(renderer, point.x - size * 0.34F, point.y - size * 0.34F, size * 0.68F, size * 0.68F);
    } else if (node.kind == NodeKind::producer) {
        fill(renderer, left, top, size, size * 0.8F);
        for (int tooth = 0; tooth < 3; ++tooth) {
            fill(renderer, left + static_cast<float>(tooth) * size * 0.38F, top - size * 0.16F,
                 size * 0.24F, size * 0.18F);
        }
    } else if (node.kind == NodeKind::fort) {
        fill(renderer, left, top - size * 0.12F, size, size * 0.92F);
        fill(renderer, left - size * 0.14F, top - size * 0.25F, size * 0.28F, size * 1.05F);
        fill(renderer, left + size * 0.86F, top - size * 0.25F, size * 0.28F, size * 1.05F);
    } else if (node.kind == NodeKind::stable) {
        fill(renderer, left, top, size, size * 0.8F);
        owner_color(renderer, node.owner);
        thick_line(renderer, {left - size * 0.08F, top}, {point.x, top - size * 0.36F}, 3.0F);
        thick_line(renderer, {point.x, top - size * 0.36F}, {left + size * 1.08F, top}, 3.0F);
    } else if (node.kind == NodeKind::port) {
        fill(renderer, left, top, size, size * 0.55F);
        fill(renderer, point.x - size * 0.12F, top + size * 0.5F, size * 0.24F, size * 0.42F);
        color(renderer, 65, 151, 174);
        thick_line(renderer, {left - size * 0.2F, top + size},
                   {left + size * 1.2F, top + size}, 3.0F);
    } else if (node.kind == NodeKind::cannon) {
        fill(renderer, left, top + size * 0.15F, size * 0.72F, size * 0.58F);
        owner_color(renderer, node.owner);
        thick_line(renderer, {point.x - size * 0.08F, point.y - size * 0.02F},
                   {point.x + size * 0.68F, point.y - size * 0.48F}, 7.0F);
    } else {
        fill(renderer, left, top + size * 0.12F, size, size * 0.68F);
        owner_color(renderer, node.owner);
        thick_line(renderer, {left, top + size * 0.12F}, {point.x, top - size * 0.35F}, 3.0F);
        thick_line(renderer, {point.x, top - size * 0.35F},
                   {left + size, top + size * 0.12F}, 3.0F);
    }
    owner_color(renderer, node.owner);
    outline(renderer, left, top, size, size * 0.8F);
}

void draw_nodes(SDL_Renderer* renderer, const Level& level, const Match& match, Vec2 center,
                float zoom) {
    char count[32];
    const float size = std::clamp(38.0F * zoom, 24.0F, 52.0F);
    const float font_scale = std::clamp(zoom, 0.8F, 1.35F);
    for (const NodeState& node : match.nodes) {
        const Vec2 point = world_to_screen(node_world_position(level, node.id), center, zoom);
        draw_node_shape(renderer, node, point, size);
        if (node.selected) {
            color(renderer, 255, 228, 113);
            thick_outline(renderer, point.x - size * 0.65F, point.y - size * 0.55F, size * 1.3F,
                          size * 1.1F, 3);
        }
        color(renderer, 242, 236, 212);
        text(renderer, point.x - size * 0.38F, point.y - 6.0F * font_scale, kind_mark(node.kind),
             font_scale);
        std::snprintf(count, sizeof(count), "%d", static_cast<int>(node.soldiers));
        text(renderer, point.x - size * 0.05F, point.y - 6.0F * font_scale, count, font_scale);
        if (node.headquarters) {
            color(renderer, 255, 228, 113);
            text(renderer, point.x - 8.0F * font_scale, point.y - size * 0.85F, "HQ", font_scale);
        }
    }
}

void draw_specialists(SDL_Renderer* renderer, const Level& level, const Match& match, Vec2 center,
                      float zoom, float alpha) {
    color(renderer, 177, 94, 62);
    for (const NodeState& node : match.nodes) {
        if (node.kind != NodeKind::cannon || node.cannon_target < 0) continue;
        const Vec2 a = world_to_screen(node_world_position(level, node.id), center, zoom);
        const Vec2 b = world_to_screen(node_world_position(level, node.cannon_target), center, zoom);
        for (int segment = 0; segment < 12; segment += 2) {
            const float begin = static_cast<float>(segment) / 12.0F;
            const float end = static_cast<float>(segment + 1) / 12.0F;
            thick_line(renderer, {std::lerp(a.x, b.x, begin), std::lerp(a.y, b.y, begin)},
                       {std::lerp(a.x, b.x, end), std::lerp(a.y, b.y, end)}, 1.0F);
        }
    }
    for (const CannonShot& shot : match.cannon_shots) {
        const Vec2 a = node_world_position(level, shot.source);
        const Vec2 b = node_world_position(level, shot.target);
        const float progress = std::clamp(std::lerp(shot.previous_progress, shot.progress, alpha),
                                          0.0F, 1.0F);
        Vec2 world{std::lerp(a.x, b.x, progress), std::lerp(a.y, b.y, progress)};
        world.y -= std::sin(progress * 3.14159265F) * 90.0F;
        const Vec2 point = world_to_screen(world, center, zoom);
        owner_color(renderer, shot.owner);
        fill(renderer, point.x - 5.0F, point.y - 5.0F, 10.0F, 10.0F);
    }
    for (const GoldShipment& gold : match.gold_shipments) {
        if (gold.path.empty()) continue;
        const Vec2 a = node_world_position(level, gold.path[static_cast<std::size_t>(gold.leg)]);
        const Vec2 b = node_world_position(level, gold.path[static_cast<std::size_t>(gold.leg + 1)]);
        const float progress = std::lerp(gold.previous_progress, gold.progress, alpha);
        const Vec2 point = world_to_screen({std::lerp(a.x, b.x, progress),
                                             std::lerp(a.y, b.y, progress)}, center, zoom);
        color(renderer, 238, 194, 64);
        fill(renderer, point.x - 7.0F, point.y - 7.0F, 14.0F, 14.0F);
        color(renderer, 255, 235, 145);
        outline(renderer, point.x - 7.0F, point.y - 7.0F, 14.0F, 14.0F);
    }
}

void draw_armies(SDL_Renderer* renderer, const Level& level, const Match& match, Vec2 center,
                 float zoom, float alpha) {
    char count[16];
    for (const Army& army : match.armies) {
        const Vec2 a = node_world_position(level, army.path[static_cast<std::size_t>(army.leg)]);
        const Vec2 b = node_world_position(level, army.path[static_cast<std::size_t>(army.leg + 1)]);
        const float progress = std::lerp(army.previous_progress, army.progress, alpha);
        const Vec2 world{std::lerp(a.x, b.x, progress), std::lerp(a.y, b.y, progress)};
        const Vec2 point = world_to_screen(world, center, zoom);
        owner_color(renderer, army.owner);
        const float size = std::clamp(13.0F * zoom, 9.0F, 18.0F);
        fill(renderer, point.x - size * 0.5F, point.y - size * 0.5F, size, size);
        std::snprintf(count, sizeof(count), "%d", static_cast<int>(army.soldiers));
        text(renderer, point.x + 9.0F, point.y - 5.0F, count, 1.0F);
    }
}

void draw_playing(SDL_Renderer* renderer, const State& state, float alpha) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    const Vec2 center = interpolated_camera_center(state.camera, alpha);
    const float zoom = interpolated_camera_zoom(state.camera, alpha);
    draw_tiles(renderer, level, center, zoom);
    draw_routes(renderer, level, center, zoom);
    draw_rallies(renderer, level, state, center, zoom);
    draw_pending_rallies(renderer, state, level, center, zoom);
    draw_specialists(renderer, level, state.match, center, zoom, alpha);
    draw_armies(renderer, level, state.match, center, zoom, alpha);
    draw_nodes(renderer, level, state.match, center, zoom);
    color(renderer, 25, 27, 25, 235);
    fill(renderer, 0.0F, 0.0F, static_cast<float>(layout_width), 62.0F);
    fill(renderer, 0.0F, 674.0F, static_cast<float>(layout_width), 46.0F);
    color(renderer, 231, 224, 196);
    char status[128];
    std::snprintf(status, sizeof(status), "%s   TIME %02d:%02d   SCORE %d",
                  level.id.c_str(), static_cast<int>(state.match.stats.elapsed_seconds) / 60,
                  static_cast<int>(state.match.stats.elapsed_seconds) % 60, state.campaign_score);
    text(renderer, 24.0F, 18.0F, status, 1.5F);
    std::snprintf(status, sizeof(status), "ENEMY %s / %s", ai_style_name(state.match.ai_style),
                  ai_difficulty_name(state.ai_difficulty));
    text(renderer, 515.0F, 22.0F, status, 1.0F);
    char commitment[32];
    std::snprintf(commitment, sizeof(commitment), "SEND %s", dispatch_name(state.dispatch_mode));
    text(renderer, 740.0F, 22.0F, commitment, 1.0F);
    const float generation_left = std::max(0.0F, state.rules.generation_seconds -
                                                    state.match.generation_clock);
    const float generation_fraction = 1.0F - generation_left / state.rules.generation_seconds;
    color(renderer, 67, 73, 70);
    fill(renderer, 980.0F, 18.0F, 270.0F, 24.0F);
    color(renderer, 214, 189, 80);
    fill(renderer, 980.0F, 18.0F, 270.0F * generation_fraction, 24.0F);
    color(renderer, 245, 235, 203);
    char generation[64];
    std::snprintf(generation, sizeof(generation), "NEXT GEN %.1fs", generation_left);
    text(renderer, 1055.0F, 22.0F, generation, 1.0F);
    const char* help = !state.rally_sources.empty()
                                 ? "RALLY TARGET: LEFT-CLICK NODE  |  CTRL=DIRECT  |  ESC LEAVES CLEARED"
                           : state.clearing_orders
                                 ? "CLEAR RALLY: CLICK OWNED BASE  |  ESC CANCELS"
                                 : "CLICK ASSAULT | SHIFT ADD/TOGGLE | CTRL DIRECT | R RALLY | 1 ONE 2 HALF 3 ALL | C CLEAR";
    text(renderer, 24.0F, 690.0F, help, 1.0F);
    if (state.input.pointer_down && state.rally_sources.empty() && !state.clearing_orders) {
        color(renderer, 255, 228, 113);
        const Vec2 a = state.input.press_origin;
        const Vec2 b = state.input.pointer;
        thick_outline(renderer, std::min(a.x, b.x), std::min(a.y, b.y), std::abs(a.x - b.x),
                      std::abs(a.y - b.y), 3);
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
    char difficulty[64];
    std::snprintf(difficulty, sizeof(difficulty), "DIFFICULTY: %s",
                  ai_difficulty_name(state.ai_difficulty));
    draw_button(renderer, 310.0F, difficulty, state.options_choice == 0);
    draw_button(renderer, 375.0F, state.fullscreen ? "FULLSCREEN: ON" : "FULLSCREEN: OFF",
                state.options_choice == 1);
    draw_button(renderer, 440.0F, "BACK", state.options_choice == 2);
}

void draw_level_card(SDL_Renderer* renderer, const State& state, float alpha) {
    color(renderer, 28, 29, 27);
    (void)SDL_RenderClear(renderer);
    const float t = state.mode_seconds + alpha * step_seconds;
    float x = 0.0F;
    if (t < 0.35F) x = std::lerp(static_cast<float>(layout_width), 0.0F, t / 0.35F);
    else if (t > 1.25F) x = std::lerp(0.0F, -static_cast<float>(layout_width), (t - 1.25F) / 0.35F);
    color(renderer, 190, 52, 45);
    fill(renderer, x, 260.0F, static_cast<float>(layout_width), 200.0F);
    color(renderer, 255, 237, 215);
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    char title[96];
    std::snprintf(title, sizeof(title), "%s  %s", level.id.c_str(), level.name.c_str());
    const float width = static_cast<float>(std::string_view(title).size()) * 8.0F * 4.0F;
    text(renderer, x + (static_cast<float>(layout_width) - width) * 0.5F, 325.0F, title, 4.0F);
    const float briefing_width = static_cast<float>(level.briefing.size()) * 8.0F;
    text(renderer, x + (static_cast<float>(layout_width) - briefing_width) * 0.5F, 410.0F,
         level.briefing, 1.0F);
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
        draw_button(renderer, 405.0F, "QUIT TO MAP", state.menu_choice == 1);
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
        std::snprintf(line, sizeof(line), "GEN CYCLES           %d", state.match.stats.generations);
        text(renderer, 430.0F, 430.0F, line, 2.0F);
        std::snprintf(line, sizeof(line), "LEVEL SCORE          %d", state.last_level_score);
        text(renderer, 430.0F, 485.0F, line, 2.0F);
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
    else if (state.mode == Mode::world_select || state.mode == Mode::world_unlock ||
             state.mode == Mode::world_transition ||
             state.mode == Mode::level_select || state.mode == Mode::level_transition) {
        draw_campaign_screen(renderer, state, alpha);
    }
    else if (state.mode == Mode::level_card) draw_level_card(renderer, state, alpha);
    else if (state.mode == Mode::playing) draw_playing(renderer, state, alpha);
    else draw_overlay_screen(renderer, state);
}
