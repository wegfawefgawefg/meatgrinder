#pragma once

#include "state.hpp"

void setup_camera(State& state, const Level& level);
void step_camera(State& state);
Vec2 interpolated_camera_center(const Camera& camera, float alpha);
float interpolated_camera_zoom(const Camera& camera, float alpha);
Vec2 world_to_screen(Vec2 world, Vec2 center, float zoom);
Vec2 screen_to_world(Vec2 screen, const Camera& camera);

