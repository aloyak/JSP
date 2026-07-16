#pragma once

#include "level.h"
#include "game.h"

class CampaignMode;

class Level_1_2 : public Level {
public:
    Level_1_2(Game* game, CampaignMode* campaignMode, const std::string& name, const std::string& scenePath)
        : Level(name, scenePath), m_game(game), m_campaignMode(campaignMode) {}

    void OnEnter() override {
    }
    void Update() override {
        Logger::log("Level 1-2 Update");
    }
    void LateUpdate() override {
    }
    void OnExit() override {
    }

private:
    Game* m_game;
    CampaignMode* m_campaignMode;

    Input& m_input = m_game->GetEngine().getInput();
};