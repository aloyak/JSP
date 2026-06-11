#include "game.h"
#include "engine/utils/logger.h"

Game::Game(Engine& engine) : m_engine(&engine), m_currentGameMode(nullptr) {
    m_engine->initUI(); // engine level ui

    m_engine->getRenderer().setupRenderTarget(600, 400);
    m_engine->getRenderer().setPixelArt(true, 16);

    m_engine->getWindow().setFullscreen(false);
    m_engine->getWindow().enableVSync(false);

    m_ui.initialize(); // custom game ui
}

void Game::Update() {
    m_engine->beginUI();
    if (m_currentGameMode) {
        m_currentGameMode->Update();
    }
    m_engine->endUI();
}

void Game::SetGameMode(std::unique_ptr<GameMode> newGameMode) {
    bool same = m_currentGameMode && 
        !newGameMode->GetScenePath().empty() && 
        m_currentGameMode->GetScenePath() == newGameMode->GetScenePath();

    if (m_currentGameMode) {
        if (!same && !m_currentGameMode->GetScenePath().empty()) {
            m_engine->getSceneManager().unload();
        }
        m_currentGameMode->OnExit();
    }

    m_lastGameMode = std::move(m_currentGameMode);
    m_currentGameMode = std::move(newGameMode);

    if (m_currentGameMode) {
        if (!same && !m_currentGameMode->GetScenePath().empty()) {
            m_engine->getSceneManager().load(m_currentGameMode->GetScenePath());
        }
        m_currentGameMode->OnEnter();
    }
}