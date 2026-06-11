#pragma once

#include "gamemode.h"
#include "simulation/orbitalManager.h"
#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "components/PlanetComponent.h"
#include "moveset/cameraorbit.h"

class Game;

class ExploreMode : public GameMode {
private:
    Game& m_game;
    Input* m_input = nullptr;

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;
    Entity* orbitTarget = nullptr;
    OrbitalManager* m_orbitalManager = nullptr;
    size_t m_currentSatIndex = 0;
    Entity* m_earthEntity = nullptr;

public:
    ExploreMode(Game& game) : GameMode("assets/scenes/space.scene"), m_game(game) {}

    void OnEnter() override {
        Engine& engine = m_game.GetEngine();
        m_input = &engine.getInput();

        m_earthEntity = engine.getSceneManager().getActiveScene()->getEntityByName("Earth").get();
        if (m_earthEntity) {
            auto* planet = m_earthEntity->addComponent<PlanetComponent>();
            planet->period = 24.0f;
            planet->radius = 637.1f;
        }

        m_camera = engine.createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 10000.0f);
        engine.setActiveCamera(m_camera);

        m_orbitCamera = new OrbitCamera(m_camera);
        SetTarget(m_earthEntity, 3000.0f);

        m_orbitalManager = new OrbitalManager(engine);
        m_orbitalManager->initialize(m_earthEntity);
    }

    void Update() override {
        if (!m_orbitalManager || !orbitTarget) return;

        const float dt = m_game.GetEngine().getDeltaTime();
        m_earthEntity->getComponent<PlanetComponent>()->update(dt * m_game.timeScale);
        m_orbitalManager->update();

        if (m_input->isKeyDown(KEY_LALT)) {
            m_input->setCursorMode(false);
        } else {
            m_input->setCursorMode(true);
            m_orbitCamera->Update(m_input, dt);
            m_orbitCamera->ApplyScroll(m_input);
        }

        if (m_input->isKeyPressed(KEY_T)) {
            const auto& sats = m_orbitalManager->getSatelliteData();
            if (!sats.empty()) {
                m_currentSatIndex = (m_currentSatIndex + 1) % sats.size();
                Entity* next = m_orbitalManager->getEntityByIndex(m_currentSatIndex);
                if (next && !next->hasComponent<PlanetComponent>()) {
                    SetTarget(next, 2.0f);
                }
            }
        }
    }

    void LateUpdate() override {
        ImGui::Begin("Info");
        ImGui::Text("Current Target: %s", orbitTarget ? orbitTarget->name.c_str() : "None");
        ImGui::Text("Distance: %.1f km", orbitTarget ? (orbitTarget->transform.position - m_camera->transform.position).length() * 10.0f : 0.0f);
        ImGui::End();
    }

    void OnExit() override {
        delete m_orbitalManager;
        delete m_orbitCamera;
    }

private:
    void SetTarget(Entity* target, float initialRadius = 50.0f) {
        if (!target) return;
        
        orbitTarget = target;
        
        m_orbitCamera->SetTarget(target, initialRadius);

        if (target != m_earthEntity) {
            Vec3 earthToTarget = target->transform.position - m_earthEntity->transform.position;
            Vec3 direction = earthToTarget.normalize();
            m_orbitCamera->SetOrientation(std::atan2(direction.z, direction.x), std::asin(direction.y));
        }
    }
};