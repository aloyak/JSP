#pragma once

#include "engine/engine.h"
#include "gamemode.h"
#include <memory>

class Game {
public:
    Game(Engine& engine);
    
    void Update();
    void SetGameMode(std::unique_ptr<GameMode> newGameMode);

    Engine& GetEngine() { return *m_engine; }

    float timeScale = 1.0f;

private:
    Engine* m_engine;
    std::unique_ptr<GameMode> m_currentGameMode;
};