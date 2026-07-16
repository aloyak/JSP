#pragma once

#include "level.h"
#include "game.h"

class CampaignMode;

class Level_1_1 : public Level {
public:
    Level_1_1(Game* game, CampaignMode* campaignMode, const std::string& name, const std::string& scenePath)
        : Level(name, scenePath), m_game(game), m_campaignMode(campaignMode) {}

    void OnEnter() override {
    }
    void Update() override {
        Logger::log("Level 1-1 Update");

        if (m_input.isKeyPressed(KEY_T)) {
            m_campaignMode->AdvanceToNextLevel();
        }
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