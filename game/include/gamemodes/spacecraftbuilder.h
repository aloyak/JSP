#pragma once

#include "gamemode.h"
#include "grid.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"

class Game;

class SpacecraftBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;

    Entity* m_target = nullptr;

    bool drawGrid = true;
public:
    SpacecraftBuilderMode(Game& game)
        : GameMode("assets/scenes/launchsite.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 30000.0f);
        m_game.GetEngine().setActiveCamera(m_camera);
        
        float distance = 3500.0f;
        float offset = distance / std::sqrt(3.0f);
        
        m_camera->transform.position = Vec3(offset, offset, offset);
        
        Vec3 orientation = Vec3(-35.264f, 45.0f, 0.0f);

        m_camera->transform.rotation = orientation;

        m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");
        
        m_orbitCamera = new OrbitCamera(m_camera);
        m_orbitCamera->SetTarget(m_target, distance);
        m_orbitCamera->SyncFromCurrentPosition();
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

        if (drawGrid) DrawGrid(m_game.GetEngine().getRenderer(), m_camera);
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
            ImGui::Separator();
            if (ImGui::BeginMenu("Spacecraft")) {
                if (ImGui::MenuItem("New Spacecraft")) {}
                if (ImGui::MenuItem("Save Spacecraft")) {}
                if (ImGui::MenuItem("Load Spacecraft")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo")) {}
                if (ImGui::MenuItem("Redo")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Show Grid", nullptr, &drawGrid)) {}
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor(2);
    }
};