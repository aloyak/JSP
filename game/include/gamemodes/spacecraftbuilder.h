#pragma once

#include <algorithm>
#include "gamemode.h"
#include "moveset/cameraorbit.h"
#include "moveset/firstperson.h"

#include "ui/selector.h"

#include "engine/components/entity.h"
#include "engine/components/skyboxComponent.h"

class Game;

enum class MoveMode {
    CameraOrbit,
    CameraFirstPerson,
};

class SpacecraftBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;
    FirstPersonCamera* m_firstPersonCamera = nullptr;

    MoveMode m_moveMode = MoveMode::CameraOrbit;

    Entity* m_target = nullptr;
    Entity* m_skybox = nullptr;

    bool m_isTransitioning = false;
    MoveMode m_transitionTarget = MoveMode::CameraOrbit;
    float m_transitionElapsed = 0.0f;
    float m_transitionDuration = 0.5f;
    Vec3 m_transitionFromPos{0.0f};
    Vec3 m_transitionToPos{0.0f};
    Vec3 m_transitionFromRot{0.0f};
    Vec3 m_transitionToRot{0.0f};

    Vec3 m_firstPersonStartPos{0.0f, 3.0f, 0.0f};

    float m_orbitFov = 55.0f;
    float m_transitionFromFov = m_orbitFov;
    float m_transitionToFov = m_orbitFov;

    // fpv
    bool m_isSprinting = false;
    float m_sprintMultiplier = 1.5f;
    float m_baseMoveSpeed = 3.5f; // used as buffer, set to the value in firstperson.h

    float m_musicDelay = 2.5f;
public:
    SpacecraftBuilderMode(Game& game)
        : GameMode("assets/scenes/builder_wip.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
        m_camera->addComponent<CameraComponent>(m_orbitFov, 1600.0f / 900.0f, 0.1f, 1000.0f);
        m_game.GetEngine().setActiveCamera(m_camera);

        m_game.GetEngine().getRenderer().setMinimumAmbientLight(0.2f);
        
        float distance = 20.0f;
        float offset = distance / std::sqrt(3.0f);
        
        m_camera->transform.position = Vec3(offset, offset, offset);
        
        Vec3 orientation = Vec3(-35.264f, 45.0f, 0.0f);

        m_camera->transform.rotation = orientation;

        m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");
        m_target->transform.position = Vec3(0.0f, 5.0f, 0.0f);

        m_skybox = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Skybox").get();
        
        m_orbitCamera = new OrbitCamera(m_camera);
        m_orbitCamera->SetTarget(m_target, distance);
        m_orbitCamera->SyncFromCurrentPosition();
        m_orbitCamera->SetMaxRadius(25.0f);
        m_orbitCamera->ApplyPosition();

        m_firstPersonCamera = new FirstPersonCamera(m_camera);
        m_baseMoveSpeed = m_firstPersonCamera->getMoveSpeed();
    }

    void OnExit() override {
        m_game.GetAudioManager().stopMusic(1.5f);
        delete m_orbitCamera;
        delete m_firstPersonCamera;
    }

    void StartTransitionTo(MoveMode mode) {
        m_transitionFromPos = m_camera->transform.position;
        m_transitionFromRot = m_camera->transform.rotation;
        m_transitionFromFov = m_camera->getComponent<CameraComponent>()->getFOV();

        if (mode == MoveMode::CameraFirstPerson) {
            m_transitionToPos = m_firstPersonStartPos;
            m_transitionToRot = Vec3(0.0f, m_camera->transform.rotation.y, 0.0f);
            m_transitionToFov = m_firstPersonCamera->getFov();
        } else {
            m_transitionToPos = m_orbitCamera->GetComputedPosition();
            m_transitionToRot = m_camera->transform.rotation; // orbit drives via lookAt, not rotation
            m_transitionToFov = m_orbitFov;
        }

        m_transitionTarget = mode;
        m_transitionElapsed = 0.0f;
        m_isTransitioning = true;
    }

    void Update() override {
        if (m_musicDelay > 0.0f) {
            m_musicDelay -= m_game.GetEngine().getDeltaTime();
            if (m_musicDelay <= 0.0f) {
                m_game.GetAudioManager().playMusic("assets/audio/terminal_pt2.wav", 10.0f, true, -0.5f);
            }
        }

        float deltaTime = m_game.GetEngine().getDeltaTime();

        if (m_isTransitioning) {
            m_transitionElapsed += deltaTime;
            float t = std::clamp(m_transitionElapsed / m_transitionDuration, 0.0f, 1.0f);
            float s = t * t * (3.0f - 2.0f * t);

            m_camera->transform.position = m_transitionFromPos + (m_transitionToPos - m_transitionFromPos) * s;
            m_camera->getComponent<CameraComponent>()->setFOV(m_transitionFromFov + (m_transitionToFov - m_transitionFromFov) * s);

            if (m_transitionTarget == MoveMode::CameraFirstPerson) {
                m_camera->transform.rotation = m_transitionFromRot + (m_transitionToRot - m_transitionFromRot) * s;
            } else {
                m_camera->getComponent<CameraComponent>()->lookAt(*m_target);
            }

            if (t >= 1.0f) {
                m_isTransitioning = false;
                m_moveMode = m_transitionTarget;
                m_camera->getComponent<CameraComponent>()->setFOV(m_transitionToFov);

                if (m_moveMode == MoveMode::CameraFirstPerson) {
                    m_firstPersonCamera->SetPlayerPosition(m_transitionToPos);
                } else {
                    m_orbitCamera->ApplyPosition();
                }
            }
        } else if (m_moveMode == MoveMode::CameraOrbit) {
            if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
                m_input.setCursorMode(true);
                m_orbitCamera->Update(&m_input, deltaTime);
                m_orbitCamera->ApplyScroll(&m_input);
            } else {
                m_input.setCursorMode(false);
            }
        } else if (m_moveMode == MoveMode::CameraFirstPerson) {
            if (!m_input.isKeyDown(KEY_LALT)) m_input.setCursorMode(true);
            else m_input.setCursorMode(false);
        
            auto& settings = m_game.settingsManager.Get();
            m_firstPersonCamera->setSensitivity(settings.mouseSens);
            m_firstPersonCamera->setHeadBobAmplitude(settings.headbobIntensity);

            if (m_input.isKeyDown(KEY_LSHIFT)) {
                m_isSprinting = true;
                m_firstPersonCamera->setMoveSpeed(m_baseMoveSpeed * m_sprintMultiplier);
            } else {
                m_isSprinting = false;
                m_firstPersonCamera->setMoveSpeed(m_baseMoveSpeed);
            }

            m_firstPersonCamera->Update(&m_input, deltaTime, m_game.GetEngine().getTime());
        }

        if (m_input.isKeyDown(KEY_LSHIFT)) m_target->transform.position.y += .015f;
        if (m_input.isKeyDown(KEY_LCTRL)) m_target->transform.position.y -= .015f;

        m_target->transform.position.y = std::clamp(m_target->transform.position.y, 0.0f, 60.0f);

        m_camera->transform.position.y = std::clamp(m_camera->transform.position.y, 2.0f, 300.0f);
    }

    void LateUpdate() override {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if (ImGui::BeginMainMenuBar()) {
            if (m_ui.beginMenu("Game")) {
                if (m_ui.menuItem("Main Menu")) m_ui.loadMainMenu();
                if (m_ui.menuItem("Settings"))  m_game.showSettings = !m_game.showSettings;
                if (m_ui.menuItem("Quit")) m_game.GetEngine().stop();
                ImGui::EndMenu();
            }
            if (m_ui.beginMenu("Spacecraft")) {
                if (m_ui.menuItem("New Spacecraft")) m_game.SetGameMode(std::make_unique<SpacecraftBuilderMode>(m_game), true, true);
                if (m_ui.menuItem("Save Spacecraft")) {}
                if (m_ui.menuItem("Load Spacecraft")) {}
                ImGui::EndMenu();
            }
            if (m_ui.beginMenu("View")) {
                
                ImGui::EndMenu();
            }
            bool wasTransitioning = m_isTransitioning;
            if (wasTransitioning) ImGui::BeginDisabled();
            if (m_moveMode == MoveMode::CameraOrbit) {
                if (m_ui.menuItem("First Person View")) StartTransitionTo(MoveMode::CameraFirstPerson);
            } else if (m_moveMode == MoveMode::CameraFirstPerson) {
                if (m_ui.menuItem("Orbit Camera View")) StartTransitionTo(MoveMode::CameraOrbit);
            }
            if (wasTransitioning) ImGui::EndDisabled();
        }
        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor(2);
    }
};