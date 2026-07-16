#pragma once

#include "gamemode.h"
#include "game.h"
#include "level.h"

#include <memory>

struct Save {
    std::string date;
    int level;
    // TODO: posisiton, rotation, whatever else
};

class CampaignMode : public GameMode {
public:
    CampaignMode(Game& game, Save save = Save{"", 0})
        : GameMode("Campaign", "assets/scenes/assembly.scene")
        , m_game(game)
        , m_save(save) {};
    virtual ~CampaignMode() = default;

    void OnEnter() override;
    void OnExit() override;
    void Update() override;
    void LateUpdate() override;

    Save& GetLastSave(); // read user/saves.json
    std::vector<Save> GetAllSaves();

    void AdvanceToNextLevel();
    
    Game& GetGame() { return m_game; }
    private:
    Game& m_game;
    Save m_save;
    
    int m_levelIndex = 0;
    Level* m_currentLevel = nullptr;
    std::vector<std::unique_ptr<Level>> m_levels;
    
    void registerLevels();
    void loadFirst();
};