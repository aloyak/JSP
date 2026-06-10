#pragma once

#include "gamemode.h"
#include "simulation/orbitalManager.h"
#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "components/PlanetComponent.h"

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

    Entity* m_earthEntity = nullptr;

public:
    ExploreMode(Game& game)
        : GameMode("assets/scenes/space.scene")
        , m_game(game) {}

    void OnEnter() override {
        Engine& engine = m_game.GetEngine();
        m_input = &engine.getInput();

        m_earthEntity =engine.getSceneManager().getActiveScene()->getEntityByName("Earth").get();

        if (m_earthEntity) {
            auto* planet = m_earthEntity->addComponent<PlanetComponent>();
            planet->period = 24.0f;
            planet->radius = 637.1f;
        }

        orbitTarget = m_earthEntity;

        m_orbitalManager = new OrbitalManager(engine);
        m_orbitalManager->initialize(orbitTarget);

        m_camera = engine.createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 10000.0f);
        engine.setActiveCamera(m_camera);
    }

    void Update() override {
        if (!m_orbitalManager || !orbitTarget)
            return;

        const float dt = m_game.GetEngine().getDeltaTime();

        m_earthEntity->getComponent<PlanetComponent>()->update(dt * m_game.timeScale);
        m_orbitalManager->update();

        if (m_input->isKeyDown(KEY_LALT)) {
            m_input->setCursorMode(false);
        }
        else {
            m_input->setCursorMode(true);
            OrbitMouse(dt);
        }

        if (m_input->isKeyPressed(KEY_T)) {
            const auto& sats = m_orbitalManager->getSatelliteData();
            if (!sats.empty()) {
                m_currentSatIndex = (m_currentSatIndex + 1) % sats.size();

                Entity* next = m_orbitalManager->getEntityByIndex(m_currentSatIndex);
                if (next) SetTarget(next);
            }
        }

        ImGui::Begin("Info");
        ImGui::Text("Current Target: %s", orbitTarget ? orbitTarget->name.c_str() : "None");
        ImGui::Text("Distance: %.1f km", orbitTarget ? (orbitTarget->transform.position -m_camera->transform.position).length() *10.0f : 0.0f);
        ImGui::End();
    }

    void OnExit() override {
        delete m_orbitalManager;
        m_orbitalManager = nullptr;
    }

private:
    void SetTarget(Entity* target) {
        if (!target)
            return;

        if (target->getComponent<PlanetComponent>()) {
            m_radius =
                target->getComponent<PlanetComponent>()->radius + 5.0f;
        }
        else {
            m_radius = 75.0f;
        }

        orbitTarget = target;

        if (m_camera && m_earthEntity && orbitTarget != m_earthEntity) {
            Vec3 earthToTarget = orbitTarget->transform.position - m_earthEntity->transform.position;
            float distance = earthToTarget.length();

            if (distance > 0.0001f) {
                Vec3 direction = earthToTarget.normalize();

                m_yaw = std::atan2(direction.z, direction.x);
                m_pitch = std::asin(direction.y);

                m_camera->transform.position = m_earthEntity->transform.position - direction * m_radius;
                m_camera->getComponent<CameraComponent>()->lookAt(*orbitTarget);
            }
        }
    }

    void OrbitMouse(float deltaTime) {
        const float mouseSensitivity  = 0.001f;
        const float scrollSensitivity = 12.0f;
        const float pitchLimit        = 89.5f * (M_PI / 180.0f);

        Vec2 delta = m_input->getMouseDelta();

        m_yaw += delta.x * mouseSensitivity;
        m_pitch -= delta.y * mouseSensitivity;

        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

        m_radius -= m_input->getScrollDelta().y * scrollSensitivity;

        float minRadius = (orbitTarget == m_earthEntity) ? m_earthEntity->getComponent<PlanetComponent>()->radius + 5.0f : 5.5f;

        m_radius = std::clamp(m_radius, minRadius, 2500.0f);

        Vec3 target = orbitTarget->transform.position;

        m_camera->transform.position = Vec3(
            target.x + m_radius * cosf(m_pitch) * cosf(m_yaw),
            target.y + m_radius * sinf(m_pitch),
            target.z + m_radius * cosf(m_pitch) * sinf(m_yaw));

        m_camera->getComponent<CameraComponent>()->lookAt(*orbitTarget);
    }
};