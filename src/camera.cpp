#include "camera.hpp"

#include "level.hpp"

#include <algorithm>
#include <cmath>

void setup_camera(State& state, const Level& level) {
    float left = 100000.0F;
    float right = -100000.0F;
    float top = 100000.0F;
    float bottom = -100000.0F;
    for (const LevelNode& node : level.nodes) {
        const Vec2 point = node_world_position(level, node.id);
        left = std::min(left, point.x - 64.0F);
        right = std::max(right, point.x + 64.0F);
        top = std::min(top, point.y - 64.0F);
        bottom = std::max(bottom, point.y + 64.0F);
    }
    const float fit_x = 1120.0F / std::max(1.0F, right - left);
    const float fit_y = 580.0F / std::max(1.0F, bottom - top);
    const float fit = std::clamp(std::min(fit_x, fit_y), 0.55F, 1.45F);
    const Vec2 center{(left + right) * 0.5F, (top + bottom) * 0.5F};
    state.camera = {
        .previous_center = {center.x + 180.0F / fit, center.y},
        .center = {center.x + 180.0F / fit, center.y},
        .target_center = center,
        .previous_zoom = fit * 0.88F,
        .zoom = fit * 0.88F,
        .target_zoom = fit,
    };
}

void step_camera(State& state) {
    Camera& camera = state.camera;
    camera.previous_center = camera.center;
    camera.previous_zoom = camera.zoom;

    const float pan_speed = 480.0F / std::max(0.1F, camera.zoom);
    camera.target_center.x += static_cast<float>(state.input.pan_right - state.input.pan_left) *
                              pan_speed * step_seconds;
    camera.target_center.y += static_cast<float>(state.input.pan_down - state.input.pan_up) *
                              pan_speed * step_seconds;
    const Vec2 dragged{
        state.input.camera_pan_delta.x / std::max(0.1F, camera.zoom),
        state.input.camera_pan_delta.y / std::max(0.1F, camera.zoom),
    };
    camera.target_center.x -= dragged.x;
    camera.target_center.y -= dragged.y;
    camera.center.x -= dragged.x;
    camera.center.y -= dragged.y;
    if (state.input.zoom_delta != 0.0F) {
        camera.target_zoom *= std::pow(1.12F, state.input.zoom_delta);
        camera.target_zoom = std::clamp(camera.target_zoom, 0.45F, 2.5F);
    }
    constexpr float response = 0.13F;
    camera.center.x += (camera.target_center.x - camera.center.x) * response;
    camera.center.y += (camera.target_center.y - camera.center.y) * response;
    camera.zoom += (camera.target_zoom - camera.zoom) * response;
}

Vec2 interpolated_camera_center(const Camera& camera, float alpha) {
    return {std::lerp(camera.previous_center.x, camera.center.x, alpha),
            std::lerp(camera.previous_center.y, camera.center.y, alpha)};
}

float interpolated_camera_zoom(const Camera& camera, float alpha) {
    return std::lerp(camera.previous_zoom, camera.zoom, alpha);
}

Vec2 world_to_screen(Vec2 world, Vec2 center, float zoom) {
    return {(world.x - center.x) * zoom + static_cast<float>(layout_width) * 0.5F,
            (world.y - center.y) * zoom + static_cast<float>(layout_height) * 0.5F + 12.0F};
}

Vec2 screen_to_world(Vec2 screen, const Camera& camera) {
    return {(screen.x - static_cast<float>(layout_width) * 0.5F) / camera.zoom + camera.center.x,
            (screen.y - static_cast<float>(layout_height) * 0.5F - 12.0F) / camera.zoom + camera.center.y};
}
