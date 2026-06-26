#pragma once

#include "engine/engine.h"

#include "gamemode.h"
#include "ui/ui.h"

#include <memory>

enum class TransitionState {
    None,
    FadingOut,
    FadingIn
};

class Game {
public:
    Game(Engine& engine);
    
    void Update();
    void LateUpdate();
    void SetGameMode(std::unique_ptr<GameMode> newGameMode, bool forceReload = false, bool transition = true);

    Engine& GetEngine() { return *m_engine; }
    UI& GetUI() { return m_ui; }

    float timeScale = 1.0f;

private:
    Engine* m_engine;

    std::unique_ptr<GameMode> m_currentGameMode;
    std::unique_ptr<GameMode> m_lastGameMode;

    UI m_ui;

    std::unique_ptr<GameMode> m_nextGameMode;
    bool m_nextForceReload = false;
    bool m_pendingModeChange = false;

    TransitionState m_transitionState = TransitionState::None;
    float m_transitionAlpha = 0.0f;
    float m_transitionSpeed = 4.0f;

    void ActualSetGameMode(std::unique_ptr<GameMode> newGameMode, bool forceReload = false);
};