#include "game.h"
#include "engine/utils/logger.h"
#include "gamemodes/mainmenu.h"

Game::Game(Engine& engine) : m_engine(&engine), m_ui(*this), m_currentGameMode(nullptr) {
    m_engine->initUI(); 

    m_engine->getRenderer().setupRenderTarget(600, 400);
    m_engine->getRenderer().setPixelArt(true, 16);

    m_engine->getWindow().setFullscreen(true);
    m_engine->getWindow().enableVSync(false);
    // m_engine->getWindow().allowResize(false); // TODO

    m_ui.initialize(); 
}

void Game::Update() {
    m_engine->beginUI();
    if (m_currentGameMode) {
        m_currentGameMode->Update();
    }
}

void Game::LateUpdate() {
    if (m_currentGameMode) {
        m_currentGameMode->LateUpdate();
    }

    // DEBUG: style editor
    //ImGui::ShowStyleEditor();

    m_engine->endUI();
}

void Game::SetGameMode(std::unique_ptr<GameMode> newGameMode, bool forceReload) {
    bool same = m_currentGameMode && 
        !newGameMode->GetScenePath().empty() && 
        m_currentGameMode->GetScenePath() == newGameMode->GetScenePath();

    if (m_currentGameMode) {
        if ((!same || forceReload) && !m_currentGameMode->GetScenePath().empty() && (forceReload || m_engine->getSceneManager().getActiveScene() != nullptr)) {
            m_engine->getSceneManager().unload();
        }
        m_currentGameMode->OnExit();
    }

    m_lastGameMode = std::move(m_currentGameMode);
    m_currentGameMode = std::move(newGameMode);

    if (m_currentGameMode) {
        if ((!same || forceReload) && !m_currentGameMode->GetScenePath().empty() && (forceReload || m_engine->getSceneManager().getActiveScene() == nullptr)) {
            m_engine->getSceneManager().load(m_currentGameMode->GetScenePath());
        }
        m_currentGameMode->OnEnter();
    }
}

// NOTE: unused right now!
void UI::showQuickOptions() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::BeginPopupModal("Options", nullptr, flags)) {
        float windowWidth = ImGui::GetWindowSize().x;

        this->setFont(1); 
        float textWidth = ImGui::CalcTextSize("Options").x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("Options");
        this->resetFont();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 110.0f;
        float buttonHeight = 75.0f;
        int totalButtons = 3;
        float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        
        float totalButtonsWidth = (buttonWidth * totalButtons) + (itemSpacing * (totalButtons - 1));
        
        ImGui::SetCursorPosX((windowWidth - totalButtonsWidth) * 0.5f);

        if (ImGui::Button("Resume", ImVec2(buttonWidth, buttonHeight))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Main Menu", ImVec2(buttonWidth, buttonHeight))) {
            this->loadMainMenu();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Quit Game", ImVec2(buttonWidth, buttonHeight))) {
            m_game.GetEngine().stop();
        }

        ImGui::EndPopup();
    }
}

void UI::loadMainMenu() {
    m_game.SetGameMode(std::make_unique<MainMenuMode>(m_game), true);
}