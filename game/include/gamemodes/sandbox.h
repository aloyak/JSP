#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"
#include "engine/utils/logger.h"

class Game;

struct GravityBody {
    std::string modelPath = "assets/models/earth.fbx";
    float mass = 1.0f;
    float radius = 1.0f;
    float period = 24.0f;
    std::string name = "Unnamed";
};

// TODO: set velocity after placement
enum class PlaceState {
    None,
    Positioning,
};

class SandboxMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    OrbitCamera* m_orbitCamera = nullptr;
    Entity* m_target = nullptr;
    Entity* m_camera = nullptr;

    bool movingEnabled = false;

    float moveSpeed = 500.0f;
    float startRadius = 2350.0f;
    
    bool drawGrid = true;
    
    bool isTransitioning = true;

    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 2.0f; 

    Vec3 m_startPosition;
    Vec3 m_startRotation;

    // sandbox specific
    Entity* selectedEntity = nullptr;
    std::vector<Entity*> planetList;
    bool physicsPaused = false;

    PlaceState m_placeState = PlaceState::None;
    int m_selectedBodyIndex = -1;
    
    Vec3 m_placePosition;
    Vec3 m_placeVelocity;
    Vec2 m_startMousePos;

    Entity* m_ghostEntity = nullptr;
    float m_velocityScale = 0.5f;

    bool m_prevLeftPressed = false;
    bool m_prevRightPressed = false;

    std::vector<GravityBody> gravityBodies = {
        { "assets/models/earth.fbx", 5.972e24f, 637.1f, 24.0f, "Earth" },
        { "assets/models/moon.fbx", 7.348e22f, 173.7f, 27.3f, "Moon" },
        { "assets/models/mars.fbx", 6.39e23f, 338.9f, 24.6f, "Mars" }
    };
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

    // TODO: this is useful, move to engine utils or something
    Vec3 GetMouseIntersection() {
        Vec2 mousePos = m_input.getMousePos();
        Vec2 winSize = m_game.GetEngine().getWindow().getSize();
        float ndcX = (2.0f * mousePos.x) / winSize.x - 1.0f;
        float ndcY = 1.0f - (2.0f * mousePos.y) / winSize.y;

        auto* camComp = m_camera->getComponent<CameraComponent>();
        Mat4 proj; camComp->getCamera().getProjectionMatrix(proj);
        float invProjX = 1.0f / proj[0][0]; 
        float invProjY = 1.0f / proj[1][1]; 

        Vec3 lookDir = (m_target->transform.position - m_camera->transform.position).normalize();
        Vec3 worldUp(0.0f, 1.0f, 0.0f);
        Vec3 rightDir = cross(lookDir, worldUp).normalize();
        Vec3 upDir = cross(rightDir, lookDir).normalize();

        Vec3 rayDir = (lookDir + rightDir * (ndcX * invProjX) + upDir * (ndcY * invProjY)).normalize();
        Vec3 rayOrigin = m_camera->transform.position;
        float planeY = m_target->transform.position.y;
        if (std::abs(rayDir.y) < 0.0001f) {
            return rayOrigin;
        }
        float t = (planeY - rayOrigin.y) / rayDir.y;
        return rayOrigin + rayDir * t;
    }

    bool IsPositionValid(const Vec3& pos, float ghostRadius) {
        for (Entity* planet : planetList) {
            if (planet) {
                float planetRadius = 1.0f;
                for (const auto& body : gravityBodies) {
                    if (planet->name == body.name) {
                        planetRadius = body.radius;
                        break;
                    }
                }
                
                planetRadius *= planet->transform.scale.x;
                
                if ((pos - planet->transform.position).length() < (ghostRadius + planetRadius + 50.0f)) {
                    return false;
                }
            }
        }
        return true;
    }

    void StartPlacement(int index) {
        if (m_placeState != PlaceState::None) {
            CancelPlacement();
        }
        m_selectedBodyIndex = index;
        m_placeState = PlaceState::Positioning;
        m_ghostEntity = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Ghost");
        m_ghostEntity->addComponent<RenderComponent>(gravityBodies[index].modelPath);
        m_ghostEntity->transform.scale = Vec3(1.246f, 1.246f, 1.246f);
    }

    void ConfirmPlacement() {
        m_ghostEntity->name = gravityBodies[m_selectedBodyIndex].name;
        m_ghostEntity->getComponent<RenderComponent>()->setBaseColor(Vec3(1.0f, 1.0f, 1.0f));
        m_ghostEntity->addComponent<PlanetComponent>();
        planetList.push_back(m_ghostEntity);
        m_ghostEntity = nullptr;
        m_placeState = PlaceState::None;
    }

    void CancelPlacement() {
        if (m_ghostEntity) {
            m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(m_ghostEntity);
            m_ghostEntity = nullptr;
        }
        m_placeState = PlaceState::None;
    }

    int TryPickPlanet() {
        Vec2 mousePos = m_input.getMousePos();
        Vec2 winSize  = m_game.GetEngine().getWindow().getSize();
        float ndcX = (2.0f * mousePos.x) / winSize.x - 1.0f;
        float ndcY = 1.0f - (2.0f * mousePos.y) / winSize.y;

        auto* camComp = m_camera->getComponent<CameraComponent>();
        Mat4 proj; camComp->getCamera().getProjectionMatrix(proj);
        float invProjX = 1.0f / proj[0][0];
        float invProjY = 1.0f / proj[1][1];

        Vec3 lookDir  = (m_target->transform.position - m_camera->transform.position).normalize();
        Vec3 worldUp(0.0f, 1.0f, 0.0f);
        Vec3 rightDir = cross(lookDir, worldUp).normalize();
        Vec3 upDir    = cross(rightDir, lookDir).normalize();
        Vec3 rayDir   = (lookDir + rightDir * (ndcX * invProjX) + upDir * (ndcY * invProjY)).normalize();
        Vec3 rayOrigin = m_camera->transform.position;

        int   bestIdx      = -1;
        float bestScore    = 1e9f; 

        for (int i = 0; i < (int)planetList.size(); i++) {
            Entity* planet = planetList[i];
            if (!planet) continue;

            float physicsRadius = 1.0f;
            for (const auto& body : gravityBodies) {
                if (planet->name == body.name) { 
                    physicsRadius = body.radius; 
                    break; 
                }
            }
            
            physicsRadius *= planet->transform.scale.x;

            Vec3  toCenter = planet->transform.position - rayOrigin;
            float t        = dot(toCenter, rayDir);
            if (t < 0.0f) continue;

            float worldDist = (rayOrigin + rayDir * t - planet->transform.position).length();

            Vec3  screenCenter = rayOrigin + rayDir * t;
            float distToCam    = toCenter.length();
            float ndcRadius    = (physicsRadius / std::max(distToCam, 1.0f)) * proj[0][0];
            float pixelRadius  = std::max(ndcRadius * winSize.x * 0.5f, 20.0f); 

            float pickThreshold = (pixelRadius / (winSize.x * 0.5f)) * distToCam / proj[0][0];

            if (worldDist < pickThreshold && t < bestScore) {
                bestScore = t;   
                bestIdx   = i;
            }
        }
        return bestIdx;
    }

    void HandlePlacementLogic() {
        bool leftPressed = m_input.isMouseButtonPressed(MOUSE_LEFT);
        bool rightPressed = m_input.isMouseButtonPressed(MOUSE_RIGHT);
        
        bool leftJustPressed = leftPressed && !m_prevLeftPressed;
        bool rightJustPressed = rightPressed && !m_prevRightPressed;

        if (m_placeState == PlaceState::Positioning) {
            Vec3 intersect = GetMouseIntersection();
            if (m_ghostEntity) {
                m_ghostEntity->transform.position = intersect;
                float ghostIntensity = sin((m_game.GetEngine().getTime() * 5.0f)) * 0.25f + 0.65f;
                
                float ghostRadius = gravityBodies[m_selectedBodyIndex].radius * m_ghostEntity->transform.scale.x;
                
                m_ghostEntity->getComponent<RenderComponent>()->setBaseColor(
                    IsPositionValid(intersect, ghostRadius) ? Vec3(0.2f, 0.8f, 0.2f) * ghostIntensity : Vec3(0.8f, 0.4f, 0.4f) * ghostIntensity
                );
            }
            if (!ImGui::GetIO().WantCaptureMouse) {
                if (leftJustPressed) {
                    ConfirmPlacement();
                    return;
                } else if (rightJustPressed) {
                    CancelPlacement();
                    return;
                }
            }
        } else if (m_placeState == PlaceState::None) {
            if (leftJustPressed && !ImGui::GetIO().WantCaptureMouse) {
                int idx = TryPickPlanet();
                if (idx >= 0) {
                    m_ghostEntity = planetList[idx];
                    planetList.erase(planetList.begin() + idx);
                    for (int i = 0; i < (int)gravityBodies.size(); i++) {
                        if (gravityBodies[i].name == m_ghostEntity->name) {
                            m_selectedBodyIndex = i;
                            break;
                        }
                    }
                    m_placeState = PlaceState::Positioning;
                }
            }
        }
    }

    void Update() override {
        float dt = m_game.GetEngine().getDeltaTime();

        if (drawGrid && movingEnabled) {
            DrawGrid(m_game.GetEngine().getRenderer());
        }

        if (!physicsPaused) {
            for (Entity* planet : planetList) {
                if (planet) {
                    auto* planetComp = planet->getComponent<PlanetComponent>();
                    if (planetComp) {
                        planetComp->update(dt * m_game.timeScale);
                    }
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

            bool isRightClicked = m_input.isMouseButtonPressed(MOUSE_RIGHT);

            if (m_placeState == PlaceState::Positioning || isRightClicked) {
                m_input.setCursorMode(true);
            } else {
                m_input.setCursorMode(false);
            }

            if (m_placeState == PlaceState::None && isRightClicked) {
                m_orbitCamera->Update(&m_input, dt);
                m_orbitCamera->ApplyScroll(&m_input);
            } else {
                m_orbitCamera->ApplyPosition();
                m_orbitCamera->ApplyScroll(&m_input);
            }

            HandlePlacementLogic();

            if (m_input.isKeyPressed(KEY_G)) {
                drawGrid = !drawGrid;
            }
            if (m_input.isKeyDown(KEY_LSHIFT)) {
                m_target->transform.position.y += 1.0f;
            }
            if (m_input.isKeyDown(KEY_LCTRL)) {
                m_target->transform.position.y -= 1.0f;   
            }
        }
        
        m_prevLeftPressed = m_input.isMouseButtonPressed(MOUSE_LEFT);
        m_prevRightPressed = m_input.isMouseButtonPressed(MOUSE_RIGHT);
    }

    void LateUpdate() override {
        DrawUI();
    }

    void DrawGrid(Renderer& renderer) {
        for (int i = -13; i <= 13; ++i) {
            Vec3 start = Vec3(i * 750.0f, m_target->transform.position.y, -9750.0f);
            Vec3 end   = Vec3(i * 750.0f, m_target->transform.position.y,  9750.0f);
            renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(), m_camera->transform, Vec3(.005f, .005f, .005f), 10.0f, false);
            start = Vec3(-9750.0f, m_target->transform.position.y, i * 750.0f);
            end   = Vec3( 9750.0f, m_target->transform.position.y, i * 750.0f);

            renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(), m_camera->transform, Vec3(.005f, .005f, .005f), 10.0f, false);
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

    void DrawUI() {
        if (isTransitioning) return;

        auto windowSize = m_game.GetEngine().getWindow().getSize();

        ImGui::SetNextWindowPos(
            ImVec2(windowSize.x * 0.5f, 5.0f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.0f) 
        );
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always); 

        ImGui::Begin("##PlayPause", nullptr,
            ImGuiWindowFlags_NoTitleBar      |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground    |
            ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("Play",  ImVec2(30, 30))) physicsPaused = false;
        ImGui::SameLine();
        if (ImGui::Button("Pause", ImVec2(30, 30))) physicsPaused = true;
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(30, 30))) {}

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

                    for (int i = 0; i < gravityBodies.size(); i++) {
                        const auto& body = gravityBodies[i];
                        if (ImGui::Button(body.name.c_str(), ImVec2(0, ImGui::GetContentRegionAvail().y))) {
                            StartPlacement(i);
                        }
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
                    // more options: gravity strength, reset grid height

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