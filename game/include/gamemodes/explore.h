#pragma once

#include "gamemode.h"
#include "simulation/orbitalManager.h"
#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "engine/utils/logger.h"

#include <cmath>
#include <algorithm>

class Game;

class ExploreMode : public GameMode {
private:
    Game& m_game;
    Input* m_input = nullptr;

    Entity* m_camera = nullptr;
    Entity* orbitTarget = nullptr;
    OrbitalManager* m_orbitalManager = nullptr;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_radius = 250.0f;
    
    size_t m_currentSatIndex = 0;

public:
    ExploreMode(Game& game) : GameMode("assets/scenes/space.scene"), m_game(game) {}

    void OnEnter() override {
        Engine& engine = m_game.GetEngine();
        m_input = &engine.getInput();

        auto* earthEntity = engine.getSceneManager().getActiveScene()->getEntityByName("Earth").get();
        
        if (!earthEntity) {
            Logger::error("Could not find 'Earth' entity in scene — check the entity name in the scene file.");
            return;
        }
        orbitTarget = earthEntity;

        m_orbitalManager = new OrbitalManager(engine);
        m_orbitalManager->initialize(orbitTarget);

        m_camera = engine.createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 10000.0f);
        engine.setActiveCamera(m_camera);
    }

    void Update() override {
        if (!m_orbitalManager || !orbitTarget) return;

        m_orbitalManager->update();
        OrbitMouse(m_game.GetEngine().getDeltaTime());

        if (m_input->isKeyPressed(KEY_T)) {
            const auto& sats = m_orbitalManager->getSatelliteData();
            if (!sats.empty()) {
                m_currentSatIndex = (m_currentSatIndex + 1) % sats.size();
                
                Entity* next = m_orbitalManager->getEntityByIndex(m_currentSatIndex);
                
                if (next) {
                    orbitTarget = next;
                    Logger::info("Switched orbit target to {}", sats[m_currentSatIndex].name);
                }
            }
        }
    }

    void OrbitMouse(float deltaTime) {
        const float mouseSensitivity  = 0.001f;
        const float scrollSensitivity = 12.0f;
        const float pitchLimit        = 89.5f * (M_PI / 180.0f); 
        
        Vec2 delta = m_input->getMouseDelta();
        m_yaw   += delta.x * mouseSensitivity;
        m_pitch -= delta.y * mouseSensitivity; 
        
        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);
        
        m_radius -= m_input->getScrollDelta().y * scrollSensitivity;
        m_radius  = std::clamp(m_radius, 637.2f, 2500.0f);

        Vec3 target = orbitTarget->transform.position;
        m_camera->transform.position = Vec3(
            target.x + m_radius * cosf(m_pitch) * cosf(m_yaw),
            target.y + m_radius * sinf(m_pitch),
            target.z + m_radius * cosf(m_pitch) * sinf(m_yaw)
        );

        m_camera->getComponent<CameraComponent>()->lookAt(*orbitTarget);
    }

    void OnExit() override {
        delete m_orbitalManager;
    }
};