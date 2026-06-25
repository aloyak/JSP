#pragma once

#include <algorithm>
#include "gamemode.h"
#include "moveset/cameraorbit.h"

#include "ui/selector.h"

#include "engine/components/entity.h"
#include "engine/components/skyboxComponent.h"

class Game;

class SpacecraftBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;

    Entity* m_target = nullptr;

    Entity* m_skybox = nullptr;
public:
    SpacecraftBuilderMode(Game& game)
        : GameMode("assets/scenes/builder_wip.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 30000.0f);
        m_game.GetEngine().setActiveCamera(m_camera);
        
        float distance = 10.0f;
        float offset = distance / std::sqrt(3.0f);
        
        m_camera->transform.position = Vec3(offset, offset, offset);
        
        Vec3 orientation = Vec3(-35.264f, 45.0f, 0.0f);

        m_camera->transform.rotation = orientation;

        m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");

        m_skybox = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Skybox").get();
        
        m_orbitCamera = new OrbitCamera(m_camera);
        m_orbitCamera->SetTarget(m_target, distance);
        m_orbitCamera->SyncFromCurrentPosition();
        m_orbitCamera->SetMaxRadius(35.0f);
        m_orbitCamera->ApplyPosition();
    }

    void OnExit() override {
        delete m_orbitCamera;
    }

    void Update() override {
        if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
            m_input.setCursorMode(true);
            m_orbitCamera->Update(&m_input, m_game.GetEngine().getDeltaTime());
            m_orbitCamera->ApplyScroll(&m_input);
        } else {
            m_input.setCursorMode(false);
        }

        if (m_input.isKeyDown(KEY_LSHIFT)) m_target->transform.position.y += .015f;
        if (m_input.isKeyDown(KEY_LCTRL)) m_target->transform.position.y -= .015f;
        // clamp y axis to 0, 300
        m_target->transform.position.y = std::clamp(m_target->transform.position.y, 0.0f, 60.0f);

        float transitionDepth = 5.0f;
        float intensity = std::clamp((m_camera->transform.position.y + transitionDepth) / transitionDepth, 0.0f, 1.0f);
        m_skybox->getComponent<SkyboxComponent>()->setColorTint(Vec3(intensity, intensity, intensity));
    }

    void LateUpdate() override {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Game")) {
                if (ImGui::MenuItem("Main Menu")) {
                    m_ui.loadMainMenu();
                }
                if (ImGui::MenuItem("Quit")) {
                    m_game.GetEngine().stop();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Spacecraft")) {
                if (ImGui::MenuItem("New Spacecraft")) {}
                if (ImGui::MenuItem("Save Spacecraft")) {}
                if (ImGui::MenuItem("Load Spacecraft")) {}
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor(2);
    }
};