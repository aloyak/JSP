#pragma once

#include "gamemode.h"
#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "engine/components/rendererComponent.h"
#include <cmath>
#include <algorithm>

#include "engine/utils/logger.h"

class Game;

class ExploreMode : public GameMode {
private:
    Game& m_game;
    Entity* m_camera = nullptr;
    Entity* orbitTarget = nullptr;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_radius = 250.0f;

public:
    ExploreMode(Game& game) : GameMode("assets/scenes/space.scene"), m_game(game) {}

    void OnEnter() override {
        Engine& engine = m_game.GetEngine();

        // 1. Fetch TLE Data from CelesTrak and cache here
        // (Call your caching file network checks here)

        orbitTarget = engine.getSceneManager().getActiveScene()->getEntityByName("Earth").get();

        // 3. Spawn real-life satellites using the parsed TLE data
        // for(const auto& satellite : cachedSatellites) { ... spawn entities ... }

        m_camera = engine.createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 10000.0f);
        engine.setActiveCamera(m_camera);
    }

    void Update() override {
        OrbitMouse(m_game.GetEngine().getDeltaTime());
    }

    void OrbitMouse(float deltaTime) {
        Input& input = m_game.GetEngine().getInput();

        const float baseRotationSpeed = 0.05f * deltaTime; 
        const float mouseSensitivity  = 0.001f;
        const float scrollSensitivity = 12.0f;
        const float pitchLimit        = 89.5f * (M_PI / 180.0f); 
        
        Vec2 delta = input.getMouseDelta();
        m_yaw   += delta.x * mouseSensitivity + baseRotationSpeed;
        m_pitch -= delta.y * mouseSensitivity; 
        
        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);
        
        m_radius -= input.getScrollDelta().y * scrollSensitivity;
        m_radius  = std::clamp(m_radius, 637.2f, 2500.0f);

        Vec3 target = orbitTarget->transform.position;
        m_camera->transform.position = Vec3(
            target.x + m_radius * cosf(m_pitch) * cosf(m_yaw),
            target.y + m_radius * sinf(m_pitch),
            target.z + m_radius * cosf(m_pitch) * sinf(m_yaw)
        );

        m_camera->getComponent<CameraComponent>()->lookAt(*orbitTarget);
    }

    void SetOrbitTarget(Entity& target) {
        orbitTarget = &target;
    }

    void OnExit() override {
    }
};