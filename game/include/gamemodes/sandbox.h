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
    UI& m_ui = m_game.GetUI();

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

    // sandbox specific
    Entity* selectedEntity = nullptr;
    std::vector<Entity*> planetList;
    bool physicsPaused = false;

public:
    SandboxMode(Game& game)
        : GameMode("assets/scenes/menu.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        Entity* earth = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Earth").get();
        if (earth) {
            planetList.push_back(earth);
        }

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

        for (Entity* planet : planetList) {
            if (planet) {
                auto* planetComp = planet->getComponent<PlanetComponent>();
                if (planetComp) {
                    planetComp->update(dt * m_game.timeScale);
                }
            }
        }

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

            moveSpeed = 1000.0f * m_orbitCamera->GetRadius() * 0.5f * dt;

            if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
                m_input.setCursorMode(true);
                m_orbitCamera->Update(&m_input, dt);
                m_orbitCamera->ApplyScroll(&m_input);
            } else {
                m_input.setCursorMode(false);
                m_orbitCamera->ApplyPosition();
                m_orbitCamera->ApplyScroll(&m_input);
            }

            if (m_input.isKeyPressed(KEY_G)) {
                drawGrid = !drawGrid;
            }
        }
    }

    void LateUpdate() override {
        if (drawGrid && movingEnabled) {
            Renderer& renderer = m_game.GetEngine().getRenderer();

            for (int i = -13; i <= 13; ++i) {
                Vec3 start = Vec3(i * 750.0f, 0, -9750.0f);
                Vec3 end   = Vec3(i * 750.0f, 0,  9750.0f);
                renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(), m_camera->transform, Vec3(.15f, .15f, .15f), 1.0f);
                start = Vec3(-9750.0f, 0, i * 750.0f);
                end   = Vec3( 9750.0f, 0, i * 750.0f);

                renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(), m_camera->transform, Vec3(.15f, .15f, .15f), 1.0f);
            }
        }

        DrawUI();
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

    void DrawUI() {
        if (isTransitioning) return;

        auto windowSize = m_game.GetEngine().getWindow().getSize();

        ImGui::SetNextWindowPos(
            ImVec2(windowSize.x * 0.5f, 5.0f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.0f)   // pivot: top-center
        );
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always); // auto-size

        ImGui::Begin("##PlayPause", nullptr,
            ImGuiWindowFlags_NoTitleBar      |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground    |
            ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button(">",  ImVec2(30, 30))) physicsPaused = false;
        ImGui::SameLine();
        if (ImGui::Button("||", ImVec2(30, 30))) physicsPaused = true;

        ImGui::End();

        ImGuiID sandboxID = ImGui::GetID("Sandbox");
        bool isCollapsed  = ImGui::GetStateStorage()->GetBool(sandboxID, false);

        float panelHeight   = isCollapsed
                                ? ImGui::GetFrameHeight() 
                                : 160.0f; 

        ImGui::SetNextWindowPos(
            ImVec2(windowSize.x * 0.5f, (float)windowSize.y),
            ImGuiCond_Always,
            ImVec2(0.5f, 1.0f) 
        );
        ImGui::SetNextWindowSize(ImVec2(500, panelHeight), ImGuiCond_Always);

        int flags = ImGuiWindowFlags_NoMove          |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoResize;

        ImGui::Begin("Sandbox", nullptr, flags);

        if (!ImGui::IsWindowCollapsed())
        {
            float contentHeight = ImGui::GetContentRegionAvail().y;

            ImGui::BeginGroup();
            if (ImGui::Button("=", ImVec2(70, contentHeight)))
                ImGui::OpenPopup("Options");
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            if (ImGui::BeginTabBar("SandboxTabBar"))
            {
                if (ImGui::BeginTabItem("Entities"))
                {
                    float childHeight = ImGui::GetContentRegionAvail().y;
                    ImGui::BeginChild("EntityScrollList",
                        ImVec2(0, childHeight),
                        ImGuiChildFlags_None,
                        ImGuiWindowFlags_HorizontalScrollbar);

                    for (int i = 0; i < 10; i++) {
                        char label[32];
                        sprintf(label, "Entity %d", i);
                        if (ImGui::Button(label, ImVec2(0, ImGui::GetContentRegionAvail().y))) {}
                        ImGui::SameLine();
                    }

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Properties"))
                {
                    char  nameBuf[64] = "Default Name";
                    float color[3]    = { 1.0f, 1.0f, 1.0f };
                    float mass        = 1.0f;
                    float radius      = 1.0f;

                    float totalWidth = ImGui::GetContentRegionAvail().x;
                    float spacing    = ImGui::GetStyle().ItemSpacing.x;

                    ImGui::SetNextItemWidth((totalWidth - spacing) * 0.75f);
                    ImGui::InputText("##NameField", nameBuf, IM_ARRAYSIZE(nameBuf),
                        ImGuiInputTextFlags_ReadOnly);

                    ImGui::SameLine();

                    ImGui::SetNextItemWidth((totalWidth - spacing) * 0.25f);
                    ImGui::ColorEdit3("##ColorField", color, ImGuiColorEditFlags_NoInputs);

                    float halfWidth = (totalWidth - spacing) * 0.5f;

                    ImGui::SetNextItemWidth(halfWidth);
                    ImGui::InputFloat("##MassField", &mass, 0.0f, 0.0f,
                        "Mass: %.2f", ImGuiInputTextFlags_ReadOnly);

                    ImGui::SameLine();

                    ImGui::SetNextItemWidth(halfWidth);
                    ImGui::InputFloat("##RadiusField", &radius, 0.0f, 0.0f,
                        "Radius: %.2f", ImGuiInputTextFlags_ReadOnly);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings"))
                {
                    if (ImGui::Button("Grid [G]")) drawGrid = !drawGrid;
                    ImGui::SliderFloat("##TimeScale", &m_game.timeScale,
                        1.0f, 10000.0f, "Time Scale: %.2f",
                        ImGuiSliderFlags_Logarithmic);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
            ImGui::EndGroup();
        }

        m_ui.showQuickOptions(); // Show options popup if open

        ImGui::End();
    }
};