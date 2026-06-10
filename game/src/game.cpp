#include "game.h"

Game::Game(Engine& engine) : m_engine(&engine), m_currentGameMode(nullptr) {
    m_engine->initUI();

    m_engine->getRenderer().setupRenderTarget(600, 400);
    m_engine->getRenderer().setPixelArt(true, 16);

    m_engine->getInput().setCursorMode(true);
    m_engine->getWindow().setFullscreen(false);
    m_engine->getWindow().enableVSync(false);

    m_engine->getRenderer().setMinimumAmbientLight(1.7f);
}

void Game::Update() {
    m_engine->beginUI();
    if (m_currentGameMode) {
        m_currentGameMode->Update();
    }
    m_engine->endUI();
}

void Game::SetGameMode(std::unique_ptr<GameMode> newGameMode) {
    if (m_currentGameMode) {
        if (!m_currentGameMode->GetScenePath().empty()) {
            m_engine->getSceneManager().unload();
        }
        m_currentGameMode->OnExit();
    }

    m_currentGameMode = std::move(newGameMode);

    if (m_currentGameMode) {
        if (!m_currentGameMode->GetScenePath().empty()) {
            m_engine->getSceneManager().load(m_currentGameMode->GetScenePath());
        }
        m_currentGameMode->OnEnter();
    }
}