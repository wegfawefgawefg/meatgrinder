#include "mechanics.hpp"

#include "level.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace {

const NodeState* find_node(const Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

NodeState* find_node(Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

template <typename CanVisit>
std::vector<int> search_path(const Level& level, const Match& match, int owner, int start, int goal,
                             CanVisit can_visit) {
    constexpr int slot_count = 100;
    if (start < 0 || start >= slot_count || goal < 0 || goal >= slot_count) return {};
    std::vector<int> previous(slot_count, -1);
    std::queue<int> open;
    previous[static_cast<std::size_t>(start)] = start;
    open.push(start);
    while (!open.empty() && previous[static_cast<std::size_t>(goal)] < 0) {
        const int current = open.front();
        open.pop();
        const auto visit_links = [&](const std::vector<Link>& links, bool sea) {
            for (const Link& link : links) {
                const int next = link.a == current ? link.b : link.b == current ? link.a : -1;
                if (next < 0 || previous[static_cast<std::size_t>(next)] >= 0) continue;
                if (sea) {
                    const NodeState* a = find_node(match, link.a);
                    const NodeState* b = find_node(match, link.b);
                    if (a == nullptr || b == nullptr || a->kind != NodeKind::port ||
                        b->kind != NodeKind::port || a->owner != owner || b->owner != owner) {
                        continue;
                    }
                }
                if (!can_visit(next)) continue;
                previous[static_cast<std::size_t>(next)] = current;
                open.push(next);
            }
        };
        visit_links(level.links, false);
        visit_links(level.sea_links, true);
    }
    if (previous[static_cast<std::size_t>(goal)] < 0) return {};
    std::vector<int> path;
    for (int at = goal;; at = previous[static_cast<std::size_t>(at)]) {
        path.push_back(at);
        if (at == start) break;
    }
    std::ranges::reverse(path);
    return path;
}

void fire_cannons(State& state) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    for (const NodeState& cannon : state.match.nodes) {
        if (cannon.kind != NodeKind::cannon || cannon.owner < 0 || cannon.cannon_target < 0) continue;
        const NodeState* target = find_node(state.match, cannon.cannon_target);
        if (target == nullptr || target->owner < 0 || target->owner == cannon.owner ||
            target->soldiers < 1.0F) {
            continue;
        }
        if (node_position(level, cannon.cannon_target) < 0) continue;
        state.match.cannon_shots.push_back({
            .owner = cannon.owner,
            .source = cannon.id,
            .target = cannon.cannon_target,
        });
    }
}

void step_cannons(State& state) {
    state.match.cannon_clock += step_seconds;
    if (state.match.cannon_clock >= state.rules.cannon_seconds) {
        state.match.cannon_clock -= state.rules.cannon_seconds;
        fire_cannons(state);
    }
    for (CannonShot& shot : state.match.cannon_shots) {
        shot.previous_progress = shot.progress;
        shot.progress += step_seconds / std::max(0.1F, state.rules.cannon_shot_seconds);
        if (shot.progress < 1.0F) continue;
        NodeState* target = find_node(state.match, shot.target);
        if (target != nullptr && target->owner >= 0 && target->owner != shot.owner) {
            target->soldiers = std::max(0.0F, target->soldiers - 1.0F);
        }
    }
    std::erase_if(state.match.cannon_shots, [](const CannonShot& shot) {
        return shot.progress >= 1.0F;
    });
}

void produce_gold(State& state) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    for (const NodeState& mine : state.match.nodes) {
        if (mine.kind != NodeKind::mine || mine.owner < 0) continue;
        const auto headquarters = std::ranges::find_if(state.match.nodes, [&mine](const NodeState& node) {
            return node.owner == mine.owner && node.headquarters;
        });
        if (headquarters == state.match.nodes.end()) continue;
        std::vector<int> path = friendly_path(level, state.match, mine.owner, mine.id,
                                              headquarters->id);
        if (path.size() < 2) continue;
        state.match.gold_shipments.push_back({.owner = mine.owner, .path = std::move(path)});
    }
}

void step_gold(State& state) {
    state.match.mine_clock += step_seconds;
    if (state.match.mine_clock >= state.rules.mine_seconds) {
        state.match.mine_clock -= state.rules.mine_seconds;
        produce_gold(state);
    }
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    for (GoldShipment& gold : state.match.gold_shipments) {
        gold.previous_progress = gold.progress;
        const Vec2 from = node_world_position(level, gold.path[static_cast<std::size_t>(gold.leg)]);
        const Vec2 to = node_world_position(level, gold.path[static_cast<std::size_t>(gold.leg + 1)]);
        const float distance = std::max(1.0F, std::hypot(to.x - from.x, to.y - from.y));
        gold.progress += state.rules.gold_speed * step_seconds / distance;
        if (gold.progress < 1.0F) continue;
        NodeState* arrived = find_node(state.match, gold.path[static_cast<std::size_t>(gold.leg + 1)]);
        if (arrived == nullptr || arrived->owner != gold.owner) {
            gold.path.clear();
            continue;
        }
        const bool final = gold.leg + 2 >= static_cast<int>(gold.path.size());
        if (final) {
            if (arrived->headquarters) arrived->soldiers += 1.0F;
            gold.path.clear();
            continue;
        }
        ++gold.leg;
        gold.previous_progress = 0.0F;
        gold.progress = 0.0F;
    }
    std::erase_if(state.match.gold_shipments, [](const GoldShipment& gold) {
        return gold.path.empty();
    });
}

} // namespace

std::vector<int> routed_path(const Level& level, const Match& match, int owner, int start,
                             int goal) {
    return search_path(level, match, owner, start, goal, [](int) { return true; });
}

std::vector<int> friendly_path(const Level& level, const Match& match, int owner, int start,
                               int goal) {
    return search_path(level, match, owner, start, goal, [&match, owner](int node_id) {
        const NodeState* node = find_node(match, node_id);
        return node != nullptr && node->owner == owner;
    });
}

void step_specialists(State& state) {
    step_cannons(state);
    step_gold(state);
}
