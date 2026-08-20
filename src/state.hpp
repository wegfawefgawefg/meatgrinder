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

struct Camera {
    Vec2 previous_center{};
    Vec2 center{};
    Vec2 target_center{};
    float previous_zoom{1.0F};
    float zoom{1.0F};
    float target_zoom{1.0F};
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

enum class AiStyle {
    balanced,
    aggressive,
    turtle,
    swarm,
};

enum class AiDifficulty {
    weak,
    normal,
    hard,
};

enum class AiBehavior {
    wait,
    attack,
    reinforce,
    expand,
};

enum class DispatchMode {
    one,
    half,
    all_but_one,
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
    bool selected{};
    bool headquarters{};
    int rally_target{-1};
    bool rally_assault{true};
    DispatchMode rally_dispatch{DispatchMode::half};
};

struct Army {
    int owner{};
    float soldiers{};
    std::vector<int> path;
    int leg{};
    float previous_progress{};
    float progress{};
    bool assault{true};
};

struct MatchStats {
    float elapsed_seconds{};
    int orders{};
    int soldiers_sent{};
    int soldiers_lost{};
    int nodes_captured{};
    int generations{};
};

struct AiMind {
    int objective{-1};
    float objective_score{};
    AiBehavior last_behavior{AiBehavior::wait};
    int decisions{};
    int orders{};
};

struct Match {
    std::vector<NodeState> nodes;
    std::vector<Army> armies;
    MatchStats stats;
    float ai_clock{};
    float generation_clock{};
    float outcome_clock{};
    int defeated_owner{neutral_owner};
    AiStyle ai_style{AiStyle::balanced};
    AiMind ai;
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
    bool camera_drag_down{};
    int camera_drag_button{};
    bool direct_down{};
    bool additive_down{};
    bool rally_orders_pressed{};
    bool clear_orders_pressed{};
    bool pan_up{};
    bool pan_down{};
    bool pan_left{};
    bool pan_right{};
    float zoom_delta{};
    Vec2 camera_pan_delta{};
    Vec2 pointer{};
    Vec2 press_origin{};
    int dispatch_choice{-1};
};

struct Rules {
    float army_speed{20.0F};
    float generation_seconds{4.0F};
    float enemy_think_seconds{0.85F};
    float enemy_aggression{0.48F};
};

struct State {
    Mode mode{Mode::main_menu};
    std::vector<Level> levels;
    Match match;
    InputState input;
    Rules rules;
    Camera camera;
    std::mt19937 random{0x4d454154U};
    int campaign_level{};
    int menu_choice{};
    int options_choice{};
    int campaign_score{};
    int completed_levels{};
    AiDifficulty ai_difficulty{AiDifficulty::normal};
    float mode_seconds{};
    bool fullscreen{};
    bool debug_open{};
    bool clearing_orders{};
    std::vector<int> rally_sources;
    DispatchMode dispatch_mode{DispatchMode::half};
};
