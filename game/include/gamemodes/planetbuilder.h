#pragma once

#include "gamemode.h"
#include "grid.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"

#include <cstring>

class Game;

class PlanetBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;

    Entity* planet = nullptr;
    float planetMass = 100.0f;
public:
    PlanetBuilderMode(Game& game)
        : GameMode("assets/scenes/space.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 30000.0f);
        m_game.GetEngine().setActiveCamera(m_camera);
        
        float distance = 1600.0f;
        float offset = distance / std::sqrt(3.0f);
        
        m_camera->transform.position = Vec3(offset, offset, offset);
        
        Vec3 orientation = Vec3(-35.264f, 45.0f, 0.0f);

        m_camera->transform.rotation = orientation;

        planet = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Planet");

        auto* renderComponent = planet->addComponent<RenderComponent>("assets/models/planetbase.fbx");
        renderComponent->setDiffuseTexturePath("assets/textures/base.png");
        auto* planetComponent = planet->addComponent<PlanetComponent>(1.0f, 637.1);
        planetComponent->adjustScale();
        
        m_orbitCamera = new OrbitCamera(m_camera);
        m_orbitCamera->SetTarget(planet, distance);
        m_orbitCamera->SetMaxRadius(3700.0f);
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
            if (ImGui::BeginMenu("Planet")) {
                if (ImGui::MenuItem("New Planet")) {}
                if (ImGui::MenuItem("Save Planet")) {}
                if (ImGui::MenuItem("Load Planet")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo")) {}
                if (ImGui::MenuItem("Redo")) {}
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor(2);

        DrawUI();
    }

    void DrawUI() {
        ImGui::Begin("Planet Builder", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Planet Properties");
        ImGui::Separator();

        ImGui::PushItemWidth(250.0f);

        if (planet) {
            char nameBuffer[128];
            std::strncpy(nameBuffer, planet->name.c_str(), sizeof(nameBuffer));
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                planet->name = std::string(nameBuffer);
            }

            auto* planetComponent = planet->getComponent<PlanetComponent>();
            if (planetComponent) {
                float radius = planetComponent->getRadius();
                if (ImGui::SliderFloat("Radius", &radius, 50.0f, 1500.0f)) {
                    planetComponent->setRadius(radius);
                    planetComponent->adjustScale();
                }
                
                if (ImGui::SliderFloat("Mass", &planetMass, 1.0f, 10000.0f));

                float period = planetComponent->getPeriod();
                if (ImGui::SliderFloat("Rotation Period", &period, 0.01f, 100.0f)) {
                    planetComponent->setPeriod(period);
                }
            }
        }

        ImGui::PopItemWidth();

        ImGui::End();
    }
};