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

    bool movingEnabled = true;
    float moveSpeed = 500.0f;
public:
    SandboxMode(Game& game)
        : GameMode("assets/scenes/menu.scene") // use the same as main menu, so it shouldnt be loaded when going from main menu
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Camera").get();
        if (m_camera) {
            m_orbitCamera = new OrbitCamera(m_camera);
        }

        m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");
        m_target->transform.position = Vec3(0, 0, 0);

        m_orbitCamera->SetTarget(m_target);
    }

    void OnExit() override {
        delete m_orbitCamera;
    }

    void Update() override {
        float dt = m_game.GetEngine().getDeltaTime();

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

            m_input.setCursorMode(true);
            
            if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
                m_orbitCamera->Update(&m_input, dt);
            } else {
                m_orbitCamera->ApplyPosition();
            }
        }
    }
};