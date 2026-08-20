#include "game.hpp"
#include "level.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    State state;
    std::string error;
    assert(load_campaign(std::string(MG_ASSET_ROOT) + "/levels/campaign.json", state.levels, error));
    assert(state.levels.size() == 10);
    assert(state.levels.front().nodes.size() == 20);
    assert(state.levels.back().nodes.size() == 52);

    start_level(state, 0);
    assert(state.mode == Mode::level_card);
    assert(state.match.nodes.size() == state.levels.front().nodes.size());

    int source = -1;
    int target = -1;
    for (const Link& link : state.levels.front().links) {
        const auto a = state.match.nodes[static_cast<std::size_t>(node_position(state.levels.front(), link.a))];
        if (a.owner == player_owner) {
            source = link.a;
            target = link.b;
            break;
        }
        const auto b = state.match.nodes[static_cast<std::size_t>(node_position(state.levels.front(), link.b))];
        if (b.owner == player_owner) {
            source = link.b;
            target = link.a;
            break;
        }
    }
    assert(source >= 0 && target >= 0);
    assert(send_army(state, source, target));
    assert(state.match.armies.size() == 1);
    assert(state.match.stats.orders == 1);

    for (int frame = 0; frame < 600; ++frame) step(state);
    assert(state.match.stats.elapsed_seconds > 0.0F);
    std::cout << "loaded 10 levels and stepped a troop dispatch\n";
    return 0;
}
