#pragma once

#include "state.hpp"

#include <filesystem>
#include <string>

bool load_progress(const std::filesystem::path& path, State& state, std::string& error);
bool save_progress(const std::filesystem::path& path, const State& state, std::string& error);
