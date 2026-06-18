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
    m_engine->setTargetFps(240);

    //Logger::setVerbose(1); // TODO: SET ON RELEASE

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

void UI::drawSplashScreen(float bgAlpha, float logoAlpha, unsigned int textureId) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    Vec2 windowSize = m_game.GetEngine().getWindow().getSize();
    ImGui::SetNextWindowSize(ImVec2(windowSize.x, windowSize.y));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, bgAlpha));
    ImGui::Begin("Splash Screen", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    float originalWidth = 1999.0f;
    float originalHeight = 676.0f;
    float aspectRatio = originalWidth / originalHeight;

    float maxWidth = windowSize.x * 0.2f;
    float maxHeight = windowSize.y * 0.2f;

    float logoWidth = maxWidth;
    float logoHeight = logoWidth / aspectRatio;

    if (logoHeight > maxHeight) {
        logoHeight = maxHeight;
        logoWidth = logoHeight * aspectRatio;
    }

    ImGui::SetCursorPos(ImVec2((windowSize.x - logoWidth) * 0.5f, (windowSize.y - logoHeight) * 0.5f));
    ImGui::Image((void*)(intptr_t)textureId, ImVec2(logoWidth, logoHeight), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, logoAlpha), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::End();
    ImGui::PopStyleColor();
}

void UI::loadMainMenu() {
    m_game.SetGameMode(std::make_unique<MainMenuMode>(m_game, false), true);
}