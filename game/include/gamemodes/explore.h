#pragma once

#include "game.h"
#include "gamemode.h"

#include "engine/components/entity.h"
#include "engine/render/postprocess/postProcessor.h"

#include "moveset/freecamera.h"
#include "registries/halcyonRegistry.h"

class ExploreMode : public GameMode {
public:
    ExploreMode(Game& game)
        : GameMode("Explore", "assets/scenes/halcyon.scene")
        , m_game(game) {}

    void OnEnter() override;
    void OnExit() override;
    void Update() override;
    void LateUpdate() override;

private:
    void setupPlanets();
    void setupBlackHole();
    void updateOrbits();

    Game& m_game;

    Entity* m_camera = nullptr;
    
    Entity* m_blackHole = nullptr;
    PostProcessor* m_blackHoleShader = nullptr;

    std::vector<Planet> m_planets;


    // DEBUG: Free camera used only for debugging, could be kept as a feature maybe
    FreeCamera* m_freeCamera = nullptr;
};