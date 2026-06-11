#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"
#include "engine/utils/logger.h"

class Game;

class SandboxMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();

    OrbitCamera* m_orbitCamera = nullptr;
    Entity* m_target = nullptr;
    Entity* m_camera = nullptr;

    bool movingEnabled = false;
    bool isTransitioning = true;

    float moveSpeed = 500.0f;
    float startRadius = 2350.0f;
    bool drawGrid = true;

    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 2.0f; 

    Vec3 m_startPosition;
    Vec3 m_startRotation;

public:
    SandboxMode(Game& game)
        : GameMode("assets/scenes/menu.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Camera").get();
        if (m_camera) {
            m_orbitCamera = new OrbitCamera(m_camera);
            m_startPosition = m_camera->transform.position;
            m_startRotation = m_camera->transform.rotation;
        }

        Entity* light = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Sun").get();
        if (light) {
            m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(light);
        }

        m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");
        m_target->transform.position = Vec3(0, 0, 0);

        m_orbitCamera->SetTarget(m_target, startRadius);
    }

    void OnExit() override {
        delete m_orbitCamera;
    }

    void Update() override {
        float dt = m_game.GetEngine().getDeltaTime();

        if (isTransitioning) 
            Transition(dt);

        if (movingEnabled) {
            Vec3 camForward = m_camera->transform.forward();
            Vec3 forwardXZ = Vec3(camForward.x, 0.0f, camForward.z).normalize();
            
            if (forwardXZ.length() < 0.0001f) forwardXZ = Vec3(0, 0, 1); 
            
            Vec3 rightXZ = Vec3(forwardXZ.z, 0.0f, -forwardXZ.x);
            
            Vec3 movement(0, 0, 0);
            if (m_input.isKeyDown(KEY_W)) movement += forwardXZ;
            if (m_input.isKeyDown(KEY_S)) movement -= forwardXZ;
            if (m_input.isKeyDown(KEY_A)) movement += rightXZ;
            if (m_input.isKeyDown(KEY_D)) movement -= rightXZ;
            
            m_target->transform.position += movement * dt * moveSpeed;

            if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
                m_input.setCursorMode(true);
                m_orbitCamera->Update(&m_input, dt);
                m_orbitCamera->ApplyScroll(&m_input);
            } else {
                m_input.setCursorMode(false);
                m_orbitCamera->ApplyPosition();
                m_orbitCamera->ApplyScroll(&m_input);
            }

            
        }
    }

    void LateUpdate() override {
        if (drawGrid && movingEnabled) {
            Renderer& renderer = m_game.GetEngine().getRenderer();

            for (int i = -100; i <= 100; ++i) {
                Vec3 start = Vec3(i * 750.0f, 0, -10000.0f);
                Vec3 end   = Vec3(i * 750.0f, 0,  10000.0f);
                renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(), m_camera->transform, Vec3(.15f, .15f, .15f), 1.0f);
                start = Vec3(-10000.0f, 0, i * 750.0f);
                end   = Vec3( 10000.0f, 0, i * 750.0f);

                renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(), m_camera->transform, Vec3(.15f, .15f, .15f), 1.0f);
            }
        }
    }

    void Transition(float dt) {
        if (!m_orbitCamera) return;

        m_transitionProgress += dt / m_transitionDuration;
        if (m_transitionProgress > 1.0f) m_transitionProgress = 1.0f;

        Vec3 targetPosition(startRadius, 0.0f);
        Vec3 targetRotation(0.0f, 180.0f, 0.0f);

        m_camera->transform.position = Vec3::lerp(m_startPosition, targetPosition, m_transitionProgress);
        m_camera->transform.rotation = Vec3::lerp(m_startRotation, targetRotation, m_transitionProgress);

        if (m_transitionProgress >= 1.0f) {
            movingEnabled = true;
            isTransitioning = false;
        }
    }
};