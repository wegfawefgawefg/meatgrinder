#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

constexpr int layout_width = 1280;
constexpr int layout_height = 720;
constexpr float step_rate = 60.0F;
constexpr float step_seconds = 1.0F / step_rate;
constexpr int neutral_owner = -1;
constexpr int player_owner = 0;
constexpr int enemy_owner = 1;

struct Vec2 {
    float x{};
    float y{};
};

enum class Mode {
    main_menu,
    options,
    level_card,
    playing,
    paused,
    defeat,
    score,
    campaign_complete,
};

enum class NodeKind {
    route,
    castle,
    stable,
    port,
    cannon,
    fort,
    mine,
};

struct LevelNode {
    int id{};
    int x{};
    int y{};
    NodeKind kind{};
    int owner{neutral_owner};
    float soldiers{};
};

struct Link {
    int a{};
    int b{};
};

struct Level {
    std::string name;
    std::string source_hash;
    std::vector<std::string> tiles;
    std::vector<LevelNode> nodes;
    std::vector<Link> links;
};

struct NodeState {
    int id{};
    NodeKind kind{};
    int owner{neutral_owner};
    float soldiers{};
    int tier{1};
    float production_clock{};
    bool selected{};
};

struct Army {
    int owner{};
    float soldiers{};
    std::vector<int> path;
    int leg{};
    float previous_progress{};
    float progress{};
};

struct MatchStats {
    float elapsed_seconds{};
    int orders{};
    int soldiers_sent{};
    int soldiers_lost{};
    int nodes_captured{};
    int promotions{};
};

struct Match {
    std::vector<NodeState> nodes;
    std::vector<Army> armies;
    MatchStats stats;
    float ai_clock{};
    float outcome_clock{};
};

struct InputState {
    bool close_requested{};
    bool toggle_debug_pressed{};
    bool toggle_fullscreen_pressed{};
    bool confirm_pressed{};
    bool back_pressed{};
    bool up_pressed{};
    bool down_pressed{};
    bool left_pressed{};
    bool right_pressed{};
    bool pointer_pressed{};
    bool pointer_released{};
    bool pointer_down{};
    bool secondary_pressed{};
    bool modifier_down{};
    Vec2 pointer{};
    Vec2 press_origin{};
};

struct Rules {
    float army_speed{150.0F};
    float production_scale{1.0F};
    float enemy_think_seconds{1.15F};
    float enemy_aggression{0.48F};
    int promotion_cost{20};
};

struct State {
    Mode mode{Mode::main_menu};
    std::vector<Level> levels;
    Match match;
    InputState input;
    Rules rules;
    std::mt19937 random{0x4d454154U};
    int campaign_level{};
    int menu_choice{};
    int options_choice{};
    int campaign_score{};
    int completed_levels{};
    float mode_seconds{};
    bool fullscreen{};
    bool debug_open{};
    bool paused_from_play{};
};

