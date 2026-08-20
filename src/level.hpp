#pragma once

#include "state.hpp"

#include <filesystem>
#include <string>

bool load_campaign(const std::filesystem::path& path, std::vector<Level>& levels,
                   std::string& error);
int node_position(const Level& level, int id);
Vec2 node_screen_position(const Level& level, int id);

