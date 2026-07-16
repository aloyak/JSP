#pragma once

#include "gamemode.h"
#include "game.h"

struct Save {
    std::string section;
    std::string date;
    int level;
};

class CampaignMode : public GameMode {
public:
    CampaignMode(Game& game, Save save = Save{"", "", 0})
        : GameMode("Campaign", "assets/scenes/assembly.scene")
        , m_game(game)
        , m_save(save) {};
    virtual ~CampaignMode() = default;

    void OnEnter() override;
    void OnExit() override;
    void Update() override;
    void LateUpdate() override;

private:
    Game& m_game;
    Save m_save;
};