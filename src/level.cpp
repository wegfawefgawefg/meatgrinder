#include "level.hpp"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {

NodeKind parse_kind(const std::string& name) {
    if (name == "castle") return NodeKind::castle;
    if (name == "stable") return NodeKind::stable;
    if (name == "port") return NodeKind::port;
    if (name == "cannon") return NodeKind::cannon;
    if (name == "fort") return NodeKind::fort;
    if (name == "mine") return NodeKind::mine;
    return NodeKind::route;
}

bool decode_level(const nlohmann::json& source, Level& level, std::string& error) {
    level.name = source.at("name").get<std::string>();
    level.source_hash = source.at("source").at("sha256").get<std::string>();
    level.tiles = source.at("tiles").get<std::vector<std::string>>();
    if (level.tiles.empty() || !std::ranges::all_of(level.tiles, [](const std::string& row) {
            return !row.empty();
        })) {
        error = level.name + " has an empty tile layer";
        return false;
    }
    const std::size_t tile_width = level.tiles.front().size();
    if (!std::ranges::all_of(level.tiles, [tile_width](const std::string& row) {
            return row.size() == tile_width;
        })) {
        error = level.name + " has inconsistent tile rows";
        return false;
    }

    std::unordered_set<int> ids;
    for (const auto& node : source.at("nodes")) {
        LevelNode decoded{
            .id = node.at("id").get<int>(),
            .x = node.at("x").get<int>(),
            .y = node.at("y").get<int>(),
            .kind = parse_kind(node.at("kind").get<std::string>()),
            .owner = node.at("owner").get<int>(),
            .soldiers = node.at("soldiers").get<float>(),
        };
        if (!ids.insert(decoded.id).second) {
            error = level.name + " repeats a node id";
            return false;
        }
        level.nodes.push_back(decoded);
    }
    for (const auto& link : source.at("links")) {
        const Link decoded{link.at(0).get<int>(), link.at(1).get<int>()};
        if (!ids.contains(decoded.a) || !ids.contains(decoded.b) || decoded.a == decoded.b) {
            error = level.name + " has an invalid link";
            return false;
        }
        level.links.push_back(decoded);
    }
    return !level.nodes.empty();
}

} // namespace

bool load_campaign(const std::filesystem::path& path, std::vector<Level>& levels,
                   std::string& error) {
    std::ifstream file{path};
    if (!file) {
        error = "could not open " + path.string();
        return false;
    }
    try {
        const nlohmann::json root = nlohmann::json::parse(file);
        if (root.at("format").get<int>() != 1) {
            error = "unsupported campaign format";
            return false;
        }
        levels.clear();
        for (const auto& source : root.at("levels")) {
            Level level;
            if (!decode_level(source, level, error)) {
                levels.clear();
                return false;
            }
            levels.push_back(std::move(level));
        }
    } catch (const std::exception& exception) {
        error = exception.what();
        levels.clear();
        return false;
    }
    return !levels.empty();
}

int node_position(const Level& level, int id) {
    const auto found = std::ranges::find(level.nodes, id, &LevelNode::id);
    return found == level.nodes.end() ? -1 : static_cast<int>(found - level.nodes.begin());
}

Vec2 node_screen_position(const Level& level, int id) {
    const int position = node_position(level, id);
    if (position < 0) return {};
    const LevelNode& node = level.nodes[static_cast<std::size_t>(position)];
    return {120.0F + static_cast<float>(node.x) * 115.0F,
            95.0F + static_cast<float>(node.y) * 58.0F};
}

