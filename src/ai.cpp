#include "ai.hpp"

#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace {

NodeState* find_node(Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

const NodeState* find_node(const Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

float route_defense(const Match& match, const std::vector<int>& path, int owner) {
    float defense = 0.0F;
    for (std::size_t index = 1; index < path.size(); ++index) {
        const NodeState* node = find_node(match, path[index]);
        if (node != nullptr && node->owner != owner) defense += node->soldiers + 1.0F;
    }
    return defense;
}

bool is_frontline(const Level& level, const Match& match, int node_id, int owner) {
    for (const Link& link : level.links) {
        const int neighbor_id = link.a == node_id ? link.b : link.b == node_id ? link.a : -1;
        const NodeState* neighbor = find_node(match, neighbor_id);
        if (neighbor != nullptr && neighbor->owner != owner) return true;
    }
    return false;
}

std::vector<int> owned_path_between(const Level& level, const Match& match, int start, int goal,
                                    int owner) {
    constexpr int slot_count = 100;
    std::vector<int> previous(slot_count, -1);
    std::queue<int> open;
    previous[static_cast<std::size_t>(start)] = start;
    open.push(start);
    while (!open.empty() && previous[static_cast<std::size_t>(goal)] < 0) {
        const int current = open.front();
        open.pop();
        for (const Link& link : level.links) {
            const int next = link.a == current ? link.b : link.b == current ? link.a : -1;
            const NodeState* node = find_node(match, next);
            if (next >= 0 && node != nullptr && node->owner == owner &&
                previous[static_cast<std::size_t>(next)] < 0) {
                previous[static_cast<std::size_t>(next)] = current;
                open.push(next);
            }
        }
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

float inbound_soldiers(const Match& match, int owner, int target_id) {
    float soldiers = 0.0F;
    for (const Army& army : match.armies) {
        if (army.owner == owner && !army.path.empty() && army.path.back() == target_id) {
            soldiers += army.soldiers;
        }
    }
    return soldiers;
}

bool reinforce_front(State& state, float attack_fraction) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    NodeState* staging = nullptr;
    float staging_need = 0.0F;
    float best_opportunity = -10000.0F;
    for (NodeState& candidate : state.match.nodes) {
        if (candidate.owner != enemy_owner ||
            !is_frontline(level, state.match, candidate.id, enemy_owner)) continue;
        float local_opportunity = -10000.0F;
        float desired = 0.0F;
        for (const Link& link : level.links) {
            const int neighbor_id = link.a == candidate.id ? link.b
                                  : link.b == candidate.id ? link.a
                                                           : -1;
            const NodeState* target = find_node(state.match, neighbor_id);
            if (target == nullptr || target->owner == enemy_owner) continue;
            float opportunity = target->owner == player_owner ? 30.0F : 5.0F;
            if (target->headquarters) opportunity += 100.0F;
            opportunity -= target->soldiers * 0.2F;
            if (opportunity > local_opportunity) {
                local_opportunity = opportunity;
                desired = (target->soldiers + 4.0F) /
                          std::max(0.25F, attack_fraction) * 1.1F;
            }
        }
        const float effective = candidate.soldiers +
                                inbound_soldiers(state.match, enemy_owner, candidate.id);
        const float need = std::max(0.0F, desired - effective);
        if (need >= 1.0F && local_opportunity > best_opportunity) {
            best_opportunity = local_opportunity;
            staging = &candidate;
            staging_need = need;
        }
    }
    if (staging == nullptr) return false;

    float reserve = 5.0F;
    if (state.match.ai_style == AiStyle::aggressive) reserve = 3.0F;
    else if (state.match.ai_style == AiStyle::turtle) reserve = 12.0F;
    else if (state.match.ai_style == AiStyle::swarm) reserve = 2.0F;
    NodeState* donor = nullptr;
    std::vector<int> donor_path;
    float best_donor_score = -10000.0F;
    for (NodeState& candidate : state.match.nodes) {
        if (candidate.owner != enemy_owner || candidate.id == staging->id ||
            candidate.soldiers < reserve + 1.0F ||
            is_frontline(level, state.match, candidate.id, enemy_owner)) continue;
        std::vector<int> path = owned_path_between(level, state.match, candidate.id, staging->id,
                                                   enemy_owner);
        if (path.size() < 2) continue;
        const float score = candidate.soldiers - static_cast<float>(path.size());
        if (score > best_donor_score) {
            best_donor_score = score;
            donor = &candidate;
            donor_path = std::move(path);
        }
    }
    if (donor == nullptr) return false;
    const float soldiers = std::min(std::floor(donor->soldiers - reserve),
                                    std::ceil(staging_need));
    return send_army_path(state, donor->id, std::move(donor_path), soldiers, true);
}

} // namespace

void step_enemy(State& state) {
    state.match.ai_clock += step_seconds;
    float interval = state.rules.enemy_think_seconds;
    float fraction = state.rules.enemy_aggression;
    float minimum = 7.0F;
    if (state.match.ai_style == AiStyle::aggressive) {
        interval *= 0.70F;
        fraction = std::max(fraction, 0.68F);
        minimum = 5.0F;
    } else if (state.match.ai_style == AiStyle::turtle) {
        interval *= 1.65F;
        fraction = 0.78F;
        minimum = 14.0F;
    } else if (state.match.ai_style == AiStyle::swarm) {
        interval *= 0.48F;
        fraction = 0.34F;
        minimum = 3.0F;
    }
    if (state.match.ai_clock < interval) return;
    state.match.ai_clock = 0.0F;
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    NodeState* best_source = nullptr;
    int best_target = -1;
    float best_score = -10000.0F;
    for (NodeState& source : state.match.nodes) {
        if (source.owner != enemy_owner || source.soldiers < minimum) continue;
        for (const NodeState& target : state.match.nodes) {
            if (target.owner == enemy_owner) continue;
            const std::vector<int> path = find_path(level, source.id, target.id);
            if (path.size() < 2) continue;
            const float committed = std::floor(source.soldiers * fraction);
            const float defense = route_defense(state.match, path, enemy_owner);
            if (state.match.ai_style == AiStyle::turtle && committed < defense * 1.25F) continue;
            if (state.match.ai_style == AiStyle::balanced && committed < defense * 0.45F) continue;
            if (state.match.ai_style == AiStyle::aggressive && committed < defense * 0.25F) continue;
            float score = committed - defense * 1.35F - static_cast<float>(path.size()) * 1.7F;
            if (target.owner == player_owner) score += 11.0F;
            if (target.headquarters) {
                score += state.match.ai_style == AiStyle::aggressive ? 42.0F : 24.0F;
            }
            if (state.match.ai_style == AiStyle::swarm) {
                score = 18.0F - target.soldiers * 2.0F - static_cast<float>(path.size());
                if (target.owner == player_owner) score += 6.0F;
            } else if (state.match.ai_style == AiStyle::turtle) {
                score += source.soldiers * 0.6F - defense;
            }
            if (score > best_score) {
                best_score = score;
                best_source = &source;
                best_target = target.id;
            }
        }
    }
    if (best_source != nullptr) {
        (void)send_army(state, best_source->id, best_target, fraction, true);
    } else {
        (void)reinforce_front(state, fraction);
    }
}
