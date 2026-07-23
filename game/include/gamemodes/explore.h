#pragma once

#include "game.h"
#include "gamemode.h"

#include "engine/components/entity.h"
#include "engine/components/rigidbodyComponent.h"
#include "engine/render/postprocess/postProcessor.h"

#include "moveset/freecamera.h"
#include "moveset/gravitybased.h"
#include "registries/halcyonRegistry.h"

// NOTE: Planets right now have a spheric collision shape, but this should be change to a chunked dynamic collider

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
    void updateOrbits(float timeScale, float dt);
    void updateLods(float dt);
    void updateGravityVector();
    void updateGlobalLighting();

    void resetOrigin();

    // DEBUG
    void drawDebugInfo();

    Game& m_game;
    float m_gravityScale = 60.0f; // DEBUG: High for now

    Entity* m_camera = nullptr;
    
    Entity* m_blackHole = nullptr;
    PostProcessor* m_blackHoleShader = nullptr;

    std::vector<Planet> m_planets;

    // Accumulated orbital time in seconds, advanced by dt * timeScale each frame
    double m_orbitTime = 0.0;


    float LODDistance = 2000.0f;

    float m_playerMaxOriginDist = 15000.0f; // triggers floating origin reset

    // DEBUG: Free camera used only for debugging, could be kept as a feature maybe
    bool m_useFreeCamera = true;
    FreeCamera* m_freeCamera = nullptr;
    GravityBasedCamera* m_gravityBasedCamera = nullptr; // actual physics-based player camera
};