#include "progress.hpp"

#include "campaign.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

bool load_progress(const std::filesystem::path& path, State& state, std::string& error) {
    state.results.assign(state.levels.size(), {});
    if (!std::filesystem::exists(path)) return true;
    std::ifstream file{path};
    if (!file) {
        error = "could not open " + path.string();
        return false;
    }
    try {
        const nlohmann::json root = nlohmann::json::parse(file);
        for (const auto& saved : root.value("levels", nlohmann::json::array())) {
            const int index = level_index(state, saved.at("id").get<std::string>());
            if (index < 0) continue;
            state.results[static_cast<std::size_t>(index)] = {
                .completed = saved.value("completed", false),
                .best_score = saved.value("best_score", 0),
                .best_seconds = saved.value("best_seconds", 0.0F),
            };
        }
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
    return true;
}

bool save_progress(const std::filesystem::path& path, const State& state, std::string& error) {
    try {
        if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
        nlohmann::json levels = nlohmann::json::array();
        for (std::size_t index = 0; index < state.levels.size(); ++index) {
            const LevelResult& result = state.results[index];
            if (!result.completed) continue;
            levels.push_back({
                {"id", state.levels[index].id},
                {"completed", result.completed},
                {"best_score", result.best_score},
                {"best_seconds", result.best_seconds},
            });
        }
        std::ofstream file{path};
        if (!file) {
            error = "could not write " + path.string();
            return false;
        }
        file << nlohmann::json{{"format", 1}, {"levels", levels}}.dump(2) << '\n';
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
    return true;
}
