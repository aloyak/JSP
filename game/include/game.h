#pragma once

#include "engine/engine.h"
#include "engine/core/userSettings.h"

#include "gamemode.h"
#include "ui/ui.h"
#include "audio/audioManager.h"

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
    AudioManager& GetAudioManager() { return m_audio; }

    float timeScale = 1.0f;
    bool showSettings = false;
private:
    Engine* m_engine;
    AudioManager m_audio;

    std::unique_ptr<GameMode> m_currentGameMode;
    std::unique_ptr<GameMode> m_lastGameMode;

    UI m_ui;

    std::unique_ptr<GameMode> m_nextGameMode;
    bool m_nextForceReload = false;
    bool m_pendingModeChange = false;

    TransitionState m_transitionState = TransitionState::None;
    float m_transitionAlpha = 0.0f;
    float m_transitionSpeed = 8.0f;
    bool m_waitingForFadeIn = false;

    void ApplyGameMode(std::unique_ptr<GameMode> newGameMode, bool forceReload = false);
};