#include "level.hpp"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {

NodeKind parse_kind(const std::string& name) {
    if (name == "producer") return NodeKind::producer;
    if (name == "stable") return NodeKind::stable;
    if (name == "port") return NodeKind::port;
    if (name == "cannon") return NodeKind::cannon;
    if (name == "fort") return NodeKind::fort;
    if (name == "mine") return NodeKind::mine;
    return NodeKind::node;
}

bool decode_level(const nlohmann::json& source, Level& level, std::string& error) {
    level.id = source.at("id").get<std::string>();
    level.name = source.at("name").get<std::string>();
    level.briefing = source.value("briefing", "");
    level.world = source.at("world").get<int>();
    level.map_x = source.at("map_x").get<int>();
    level.map_y = source.at("map_y").get<int>();
    level.prerequisites = source.value("requires", std::vector<std::string>{});
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
        const std::string kind_name = node.at("kind").get<std::string>();
        if (kind_name != "node" && kind_name != "producer" && kind_name != "stable" &&
            kind_name != "port" && kind_name != "cannon" && kind_name != "fort" &&
            kind_name != "mine") {
            error = level.name + " has an unknown node kind";
            return false;
        }
        LevelNode decoded{
            .id = node.at("id").get<int>(),
            .x = node.at("x").get<int>(),
            .y = node.at("y").get<int>(),
            .kind = parse_kind(kind_name),
            .owner = node.at("owner").get<int>(),
            .soldiers = node.at("soldiers").get<float>(),
            .headquarters = node.value("headquarters", false),
            .cannon_target = node.value("target", -1),
        };
        if (decoded.id < 0 || decoded.id >= 100) {
            error = level.name + " has a node id outside 0-99";
            return false;
        }
        if (!ids.insert(decoded.id).second) {
            error = level.name + " repeats a node id";
            return false;
        }
        if (decoded.owner < neutral_owner || decoded.owner > enemy_owner) {
            error = level.name + " has an invalid owner";
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
    for (const auto& link : source.value("sea_links", nlohmann::json::array())) {
        const Link decoded{link.at(0).get<int>(), link.at(1).get<int>()};
        if (!ids.contains(decoded.a) || !ids.contains(decoded.b) || decoded.a == decoded.b) {
            error = level.name + " has an invalid sea link";
            return false;
        }
        const auto a = std::ranges::find(level.nodes, decoded.a, &LevelNode::id);
        const auto b = std::ranges::find(level.nodes, decoded.b, &LevelNode::id);
        if (a->kind != NodeKind::port || b->kind != NodeKind::port) {
            error = level.name + " has a sea link outside two ports";
            return false;
        }
        level.sea_links.push_back(decoded);
    }
    for (const LevelNode& node : level.nodes) {
        if (node.cannon_target >= 0 && !ids.contains(node.cannon_target)) {
            error = level.name + " has an invalid cannon target";
            return false;
        }
        if (node.cannon_target >= 0 && node.kind != NodeKind::cannon) {
            error = level.name + " gives a target to a non-cannon";
            return false;
        }
    }
    for (int owner : {player_owner, enemy_owner}) {
        const int headquarters = static_cast<int>(std::ranges::count_if(
            level.nodes, [owner](const LevelNode& node) {
                return node.owner == owner && node.headquarters;
            }));
        if (headquarters != 1) {
            error = level.name + " must define one headquarters per side";
            return false;
        }
    }
    return !level.nodes.empty();
}

} // namespace

bool load_campaign(const std::filesystem::path& path, std::vector<Level>& levels,
                   std::vector<World>& worlds, std::string& error) {
    std::ifstream file{path};
    if (!file) {
        error = "could not open " + path.string();
        return false;
    }
    try {
        const nlohmann::json root = nlohmann::json::parse(file);
        if (root.at("format").get<int>() != 2) {
            error = "unsupported campaign format";
            return false;
        }
        levels.clear();
        worlds.clear();
        for (const auto& source : root.at("worlds")) {
            worlds.push_back({
                .id = source.at("id").get<std::string>(),
                .name = source.at("name").get<std::string>(),
                .theme = source.at("theme").get<std::string>(),
                .map_x = source.at("map_x").get<int>(),
                .map_y = source.at("map_y").get<int>(),
                .levels = {},
            });
        }
        for (const auto& source : root.at("levels")) {
            Level level;
            if (!decode_level(source, level, error)) {
                levels.clear();
                return false;
            }
            if (level.world < 0 || level.world >= static_cast<int>(worlds.size())) {
                error = level.name + " has an invalid world";
                levels.clear();
                worlds.clear();
                return false;
            }
            worlds[static_cast<std::size_t>(level.world)].levels.push_back(
                static_cast<int>(levels.size()));
            levels.push_back(std::move(level));
        }
        std::unordered_set<std::string> level_ids;
        for (const Level& level : levels) {
            if (!level_ids.insert(level.id).second) {
                error = "campaign repeats level id " + level.id;
                levels.clear();
                worlds.clear();
                return false;
            }
        }
        for (const Level& level : levels) {
            for (const std::string& prerequisite : level.prerequisites) {
                if (level_ids.contains(prerequisite)) continue;
                error = level.name + " has an unknown prerequisite";
                levels.clear();
                worlds.clear();
                return false;
            }
        }
    } catch (const std::exception& exception) {
        error = exception.what();
        levels.clear();
        worlds.clear();
        return false;
    }
    return !levels.empty() && !worlds.empty();
}

int node_position(const Level& level, int id) {
    const auto found = std::ranges::find(level.nodes, id, &LevelNode::id);
    return found == level.nodes.end() ? -1 : static_cast<int>(found - level.nodes.begin());
}

Vec2 node_world_position(const Level& level, int id) {
    const int position = node_position(level, id);
    if (position < 0) return {};
    const LevelNode& node = level.nodes[static_cast<std::size_t>(position)];
    constexpr float tile_size = 64.0F;
    return {(static_cast<float>(node.x) * 2.0F + 0.5F) * tile_size,
            (static_cast<float>(node.y) + 1.5F) * tile_size};
}
