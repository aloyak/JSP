#include "gamemodes/campaign.h"

void CampaignMode::OnEnter() {
    m_game.showHelp = false;
    m_game.showSettings = false;

    m_game.timeScale = 1.0f;

    registerLevels();

    loadFirst();
}

#include "levels/level_1_1.h"
#include "levels/level_1_2.h"
void CampaignMode::registerLevels() {
    m_levels.push_back(
        std::make_unique<Level_1_1>(&m_game, this, "Level 1-1", "assets/scenes/space.scene")
    );
    m_levels.push_back(
        std::make_unique<Level_1_2>(&m_game, this, "Level 1-2", "assets/scenes/assembly.scene")
    );
}

void CampaignMode::OnExit() {}

void CampaignMode::Update() {
    if (m_currentLevel) m_currentLevel->Update();
    
    int flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("NOTE", nullptr, flags);
    ImGui::TextUnformatted("The Campaign mode is currently under development");
    ImGui::TextUnformatted("It's unfortunately not yet playable. Coming SOON!");
    if (ImGui::Button("Back to Main Menu")) {
        m_game.GetUI().loadMainMenu();
    }
    ImGui::End();
}

void CampaignMode::LateUpdate() {
    if (m_currentLevel) m_currentLevel->LateUpdate();
}

void CampaignMode::loadFirst() {
    m_levelIndex = 0;
    if (m_levels.empty()) return;

    m_currentLevel = m_levels[m_levelIndex].get();
    m_game.GetEngine().getSceneManager().load(m_currentLevel->GetScenePath());
    m_currentLevel->OnEnter();
}

void CampaignMode::AdvanceToNextLevel() {
    if (m_currentLevel) {
        m_currentLevel->OnExit();
    }

    int nextIndex = m_levelIndex + 1;
    if (nextIndex < (int)m_levels.size()) {
        if (m_levels[nextIndex]->GetScenePath() != m_levels[m_levelIndex]->GetScenePath()) {
            m_game.GetEngine().getSceneManager().load(m_levels[nextIndex]->GetScenePath());
        }
        m_levelIndex = nextIndex;
        m_currentLevel = m_levels[m_levelIndex].get();
        m_currentLevel->OnEnter();
    } else {
        m_currentLevel = nullptr;
    }
}

Save& CampaignMode::GetLastSave() { // user/saves.json
    // TODO: Implement save loading logic
    static Save lastSave{"", 0};
    return lastSave;
}

std::vector<Save> CampaignMode::GetAllSaves() {
    // TODO: Implement save loading logic
    return {};
}