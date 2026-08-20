#include "ai.hpp"

#include "game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <queue>

namespace {

struct DecisionTuning {
    float interval_scale{};
    float score_noise{};
    float switch_margin{};
    int action_budget{};
    int idle_bonus{};
};

struct Action {
    AiBehavior behavior{AiBehavior::wait};
    int source{-1};
    float soldiers{};
    float score{};
    bool urgent{};
    std::vector<int> path;
};

constexpr std::array<AiBehavior, 4> behaviors{
    AiBehavior::attack,
    AiBehavior::reinforce,
    AiBehavior::expand,
    AiBehavior::wait,
};

NodeState* find_node(Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

const NodeState* find_node(const Match& match, int id) {
    const auto found = std::ranges::find(match.nodes, id, &NodeState::id);
    return found == match.nodes.end() ? nullptr : &*found;
}

DecisionTuning decision_tuning(AiDifficulty difficulty) {
    if (difficulty == AiDifficulty::weak) return {1.55F, 13.0F, 18.0F, 1, 45};
    if (difficulty == AiDifficulty::hard) return {0.72F, 1.5F, 7.0F, 2, -10};
    return {1.0F, 5.0F, 12.0F, 1, 0};
}

int behavior_weight(AiStyle style, AiBehavior behavior) {
    if (style == AiStyle::aggressive) {
        if (behavior == AiBehavior::attack) return 55;
        if (behavior == AiBehavior::reinforce) return 20;
        if (behavior == AiBehavior::expand) return 15;
        return 10;
    }
    if (style == AiStyle::turtle) {
        if (behavior == AiBehavior::attack) return 15;
        if (behavior == AiBehavior::reinforce) return 55;
        if (behavior == AiBehavior::expand) return 10;
        return 20;
    }
    if (style == AiStyle::swarm) {
        if (behavior == AiBehavior::attack) return 60;
        if (behavior == AiBehavior::reinforce) return 15;
        if (behavior == AiBehavior::expand) return 15;
        return 10;
    }
    if (behavior == AiBehavior::attack) return 35;
    if (behavior == AiBehavior::reinforce) return 35;
    if (behavior == AiBehavior::expand) return 20;
    return 10;
}

float attack_fraction(const State& state) {
    if (state.match.ai_style == AiStyle::aggressive) {
        return std::max(state.rules.enemy_aggression, 0.68F);
    }
    if (state.match.ai_style == AiStyle::turtle) return 0.78F;
    if (state.match.ai_style == AiStyle::swarm) return 0.34F;
    return state.rules.enemy_aggression;
}

float minimum_force(AiStyle style) {
    if (style == AiStyle::aggressive) return 5.0F;
    if (style == AiStyle::turtle) return 14.0F;
    if (style == AiStyle::swarm) return 3.0F;
    return 7.0F;
}

float rear_reserve(AiStyle style) {
    if (style == AiStyle::aggressive) return 3.0F;
    if (style == AiStyle::turtle) return 12.0F;
    if (style == AiStyle::swarm) return 2.0F;
    return 5.0F;
}

float route_defense(const Match& match, const std::vector<int>& path, int owner) {
    float defense = 0.0F;
    for (std::size_t index = 1; index < path.size(); ++index) {
        const NodeState* node = find_node(match, path[index]);
        if (node != nullptr && node->owner != owner) defense += node->soldiers + 1.0F;
    }
    return defense;
}

bool viable_attack(AiStyle style, float committed, float defense) {
    if (style == AiStyle::turtle) return committed >= defense * 1.25F;
    if (style == AiStyle::balanced) return committed >= defense * 0.45F;
    if (style == AiStyle::aggressive) return committed >= defense * 0.25F;
    return true;
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

float threatening_soldiers(const Match& match, int owner, int target_id) {
    float soldiers = 0.0F;
    for (const Army& army : match.armies) {
        if (army.owner != owner || army.path.empty()) continue;
        const std::size_t begin = std::min(army.path.size(), static_cast<std::size_t>(army.leg + 1));
        for (std::size_t index = begin; index < army.path.size(); ++index) {
            if (army.path[index] != target_id) continue;
            if (army.assault || index + 1 == army.path.size()) soldiers += army.soldiers;
            break;
        }
    }
    return soldiers;
}

float strategic_score(const Match& match, const NodeState& target) {
    float score = target.owner == player_owner ? 30.0F : 8.0F;
    if (target.headquarters) score += 45.0F;
    score -= target.soldiers * 0.25F;
    score -= inbound_soldiers(match, enemy_owner, target.id) * 0.15F;
    return score;
}

void update_objective(State& state, float switch_margin) {
    const NodeState* current = find_node(state.match, state.match.ai.objective);
    const bool current_valid = current != nullptr && current->owner != enemy_owner;
    float current_score = current_valid ? strategic_score(state.match, *current) : -10000.0F;
    const NodeState* best = nullptr;
    float best_score = -10000.0F;
    for (const NodeState& candidate : state.match.nodes) {
        if (candidate.owner == enemy_owner) continue;
        const float score = strategic_score(state.match, candidate);
        if (score > best_score) {
            best = &candidate;
            best_score = score;
        }
    }
    if (!current_valid || best_score > current_score + switch_margin) {
        state.match.ai.objective = best != nullptr ? best->id : -1;
        current_score = best_score;
    }
    state.match.ai.objective_score = current_score;
}

std::vector<Action> attack_actions(const State& state, const std::vector<int>& used_sources) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    const float fraction = attack_fraction(state);
    const float minimum = minimum_force(state.match.ai_style);
    std::vector<Action> actions;
    for (const NodeState& source : state.match.nodes) {
        if (source.owner != enemy_owner || source.soldiers < minimum ||
            std::ranges::find(used_sources, source.id) != used_sources.end()) continue;
        for (const NodeState& target : state.match.nodes) {
            if (target.owner == enemy_owner) continue;
            std::vector<int> path = find_path(level, source.id, target.id);
            if (path.size() < 2) continue;
            const float committed = std::floor(source.soldiers * fraction);
            const float defense = route_defense(state.match, path, enemy_owner);
            if (!viable_attack(state.match.ai_style, committed, defense)) continue;
            float score = committed - defense * 1.35F -
                          static_cast<float>(path.size()) * 1.7F + strategic_score(state.match, target);
            if (target.id == state.match.ai.objective) score += 18.0F;
            if (state.match.ai_style == AiStyle::swarm) {
                score = 18.0F - target.soldiers * 2.0F -
                        static_cast<float>(path.size());
                if (target.owner == player_owner) score += 6.0F;
                if (target.id == state.match.ai.objective) score += 18.0F;
            } else if (state.match.ai_style == AiStyle::turtle) {
                score += source.soldiers * 0.6F - defense;
            }
            actions.push_back({
                .behavior = target.owner == player_owner ? AiBehavior::attack : AiBehavior::expand,
                .source = source.id,
                .soldiers = committed,
                .score = score,
                .path = std::move(path),
            });
        }
    }
    return actions;
}

std::vector<Action> reinforce_actions(const State& state, const std::vector<int>& used_sources) {
    const Level& level = state.levels[static_cast<std::size_t>(state.campaign_level)];
    const float fraction = attack_fraction(state);
    const float reserve = rear_reserve(state.match.ai_style);
    std::vector<Action> actions;
    for (const NodeState& staging : state.match.nodes) {
        if (staging.owner != enemy_owner) continue;
        const float hostile_inbound = threatening_soldiers(state.match, player_owner, staging.id);
        if (!is_frontline(level, state.match, staging.id, enemy_owner) && hostile_inbound < 1.0F) {
            continue;
        }
        int objective = -1;
        float opportunity = -10000.0F;
        float desired = hostile_inbound * 1.25F + 4.0F;
        for (const Link& link : level.links) {
            const int neighbor_id = link.a == staging.id ? link.b
                                  : link.b == staging.id ? link.a
                                                         : -1;
            const NodeState* target = find_node(state.match, neighbor_id);
            if (target == nullptr || target->owner == enemy_owner) continue;
            const float candidate_score = strategic_score(state.match, *target);
            if (candidate_score > opportunity) {
                opportunity = candidate_score;
                objective = target->id;
                desired = std::max(desired, (target->soldiers + 4.0F) /
                                                   std::max(0.25F, fraction) * 1.1F);
            }
        }
        const float friendly_inbound = inbound_soldiers(state.match, enemy_owner, staging.id);
        const float need = desired - staging.soldiers - friendly_inbound;
        if (need < 1.0F) continue;
        for (const NodeState& donor : state.match.nodes) {
            if (donor.owner != enemy_owner || donor.id == staging.id ||
                donor.soldiers < reserve + 1.0F ||
                is_frontline(level, state.match, donor.id, enemy_owner) ||
                std::ranges::find(used_sources, donor.id) != used_sources.end()) continue;
            std::vector<int> path = owned_path_between(level, state.match, donor.id, staging.id,
                                                       enemy_owner);
            if (path.size() < 2) continue;
            const float soldiers = std::min(std::floor(donor.soldiers - reserve),
                                            std::ceil(need));
            float score = opportunity + soldiers * 0.2F -
                          static_cast<float>(path.size()) * 1.5F;
            if (objective == state.match.ai.objective) score += 18.0F;
            const bool urgent = hostile_inbound >= 1.0F;
            if (urgent) score += 1000.0F;
            actions.push_back({
                .behavior = AiBehavior::reinforce,
                .source = donor.id,
                .soldiers = soldiers,
                .score = score,
                .urgent = urgent,
                .path = std::move(path),
            });
        }
    }
    return actions;
}

std::vector<Action> collect_actions(const State& state, const std::vector<int>& used_sources) {
    std::vector<Action> actions = attack_actions(state, used_sources);
    std::vector<Action> reinforcement = reinforce_actions(state, used_sources);
    actions.insert(actions.end(), std::make_move_iterator(reinforcement.begin()),
                   std::make_move_iterator(reinforcement.end()));
    return actions;
}

bool behavior_available(const std::vector<Action>& actions, AiBehavior behavior) {
    return behavior == AiBehavior::wait ||
           std::ranges::any_of(actions, [behavior](const Action& action) {
               return action.behavior == behavior;
           });
}

AiBehavior roll_behavior(State& state, const std::vector<Action>& actions,
                         const DecisionTuning& tuning) {
    int total = 0;
    std::array<int, behaviors.size()> weights{};
    for (std::size_t index = 0; index < behaviors.size(); ++index) {
        const AiBehavior behavior = behaviors[index];
        if (!behavior_available(actions, behavior)) continue;
        int weight = behavior_weight(state.match.ai_style, behavior);
        if (behavior == AiBehavior::wait) weight = std::max(0, weight + tuning.idle_bonus);
        weights[index] = weight;
        total += weight;
    }
    if (total <= 0) return AiBehavior::wait;
    std::uniform_int_distribution<int> roll(1, total);
    int value = roll(state.random);
    for (std::size_t index = 0; index < behaviors.size(); ++index) {
        value -= weights[index];
        if (value <= 0) return behaviors[index];
    }
    return AiBehavior::wait;
}

const Action* best_action(State& state, const std::vector<Action>& actions, AiBehavior behavior,
                          float noise) {
    const Action* best = nullptr;
    float best_score = -10000.0F;
    std::uniform_real_distribution<float> error(-noise, noise);
    for (const Action& action : actions) {
        if (action.behavior != behavior) continue;
        const float score = action.score + error(state.random);
        if (score > best_score) {
            best = &action;
            best_score = score;
        }
    }
    return best;
}

const Action* urgent_action(const std::vector<Action>& actions) {
    const Action* best = nullptr;
    for (const Action& action : actions) {
        if (action.urgent && (best == nullptr || action.score > best->score)) best = &action;
    }
    return best;
}

bool execute_action(State& state, const Action& action) {
    return send_army_path(state, action.source, action.path, action.soldiers, true);
}

float decision_interval(const State& state, const DecisionTuning& tuning) {
    float interval = state.rules.enemy_think_seconds;
    if (state.match.ai_style == AiStyle::aggressive) interval *= 0.70F;
    else if (state.match.ai_style == AiStyle::turtle) interval *= 1.65F;
    else if (state.match.ai_style == AiStyle::swarm) interval *= 0.48F;
    return interval * tuning.interval_scale;
}

} // namespace

void run_enemy_decision(State& state) {
    const DecisionTuning tuning = decision_tuning(state.ai_difficulty);
    update_objective(state, tuning.switch_margin);
    ++state.match.ai.decisions;
    std::vector<int> used_sources;
    for (int order = 0; order < tuning.action_budget; ++order) {
        const std::vector<Action> actions = collect_actions(state, used_sources);
        if (actions.empty()) {
            state.match.ai.last_behavior = AiBehavior::wait;
            return;
        }
        const Action* selected = urgent_action(actions);
        AiBehavior behavior = AiBehavior::reinforce;
        if (selected == nullptr) {
            behavior = roll_behavior(state, actions, tuning);
            if (behavior == AiBehavior::wait) {
                state.match.ai.last_behavior = AiBehavior::wait;
                return;
            }
            selected = best_action(state, actions, behavior, tuning.score_noise);
        }
        if (selected == nullptr || !execute_action(state, *selected)) return;
        state.match.ai.last_behavior = selected->behavior;
        ++state.match.ai.orders;
        used_sources.push_back(selected->source);
    }
}

void step_enemy(State& state) {
    const DecisionTuning tuning = decision_tuning(state.ai_difficulty);
    state.match.ai_clock += step_seconds;
    if (state.match.ai_clock < decision_interval(state, tuning)) return;
    state.match.ai_clock = 0.0F;
    run_enemy_decision(state);
}
