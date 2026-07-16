#include "gamemodes/campaign.h"

void CampaignMode::OnEnter() {
    m_game.timeScale = 1.0f;
}

void CampaignMode::OnExit() {}
void CampaignMode::Update() {
    int flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("NOTE", nullptr, flags);
    ImGui::TextUnformatted("The Campaign mode is currently under development");
    ImGui::TextUnformatted("It's unfortunately not yet playable. Coming SOON!");
    if (ImGui::Button("Back to Main Menu")) {
        m_game.GetUI().loadMainMenu();
    }
    ImGui::End();
}
void CampaignMode::LateUpdate() {}