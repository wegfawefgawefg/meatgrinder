#include "debug.hpp"

#include "game.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imgui.h>

bool init_debug(SDL_Window* window, SDL_Renderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    return ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) && ImGui_ImplSDLRenderer3_Init(renderer);
}

void draw_debug(State& state, SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    if (state.debug_open) {
        ImGui::SetNextWindowSize(ImVec2(360.0F, 470.0F), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Meatgrinder developer tools", &state.debug_open)) {
            ImGui::Text("Level %d / %d", state.campaign_level + 1, static_cast<int>(state.levels.size()));
            ImGui::Text("Nodes: %d", static_cast<int>(state.match.nodes.size()));
            ImGui::Text("Armies: %d", static_cast<int>(state.match.armies.size()));
            ImGui::SeparatorText("Rules");
            ImGui::SliderFloat("Army speed", &state.rules.army_speed, 10.0F, 180.0F, "%.0f px/s");
            ImGui::SliderFloat("GEN interval", &state.rules.generation_seconds, 3.0F, 30.0F, "%.1f s");
            ImGui::SliderFloat("GEN scale", &state.rules.generation_scale, 0.25F, 3.0F);
            ImGui::SliderFloat("AI interval", &state.rules.enemy_think_seconds, 0.15F, 4.0F, "%.2f s");
            ImGui::SliderFloat("AI dispatch", &state.rules.enemy_aggression, 0.1F, 0.9F);
            ImGui::SeparatorText("Match");
            ImGui::Text("Next GEN: %.2f", state.rules.generation_seconds -
                                               state.match.generation_clock);
            ImGui::Text("Camera: %.0f, %.0f @ %.2f", state.camera.center.x,
                        state.camera.center.y, state.camera.zoom);
            if (ImGui::Button("Restart level")) restart_level(state);
            ImGui::SameLine();
            if (ImGui::Button("Win now")) {
                for (NodeState& node : state.match.nodes) node.owner = player_owner;
                state.match.armies.clear();
            }
            if (ImGui::Button("Lose now")) {
                for (NodeState& node : state.match.nodes) node.owner = enemy_owner;
                state.match.armies.clear();
            }
            ImGui::SeparatorText("Campaign jump");
            for (int index = 0; index < static_cast<int>(state.levels.size()); ++index) {
                ImGui::PushID(index);
                if (ImGui::SmallButton(state.levels[static_cast<std::size_t>(index)].name.c_str())) {
                    start_level(state, index);
                }
                ImGui::PopID();
                if (index % 3 != 2) ImGui::SameLine();
            }
        }
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void shutdown_debug() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
