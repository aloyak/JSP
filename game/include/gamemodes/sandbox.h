#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/utils/logger.h"

#include "components/PlanetComponent.h"
#include "planetRegistry.h"
#include "ui/selector.h"
#include "engine/utils/path.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>

class Game;

enum class PlaceState {
    None,
    Positioning,
    VelocityDrag,
};

enum class ToolMode {
    Selection,
    Reallocation,
    Velocity,
};

class SandboxMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Selector m_selector      = Selector(m_game, "user/planets", ".planet");
    Selector m_loadSelector  = Selector(m_game, "user/sandbox", ".sandbox");

    bool m_showSaveDialog = false;
    char m_saveNameBuf[128] = {};
    bool m_pendingLoad = false;

    OrbitCamera* m_orbitCamera = nullptr;
    Entity* m_target = nullptr;
    Entity* m_camera = nullptr;

    bool movingEnabled = false;

    float moveSpeed = 500.0f;
    float startRadius = 2350.0f;
    
    bool drawGrid = false;
    bool drawTrails = true;
    bool m_showSandbox = true;

    bool isTransitioning = true;

    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 2.0f; 

    Vec3 m_startPosition;
    Vec3 m_startRotation;

    Entity* selectedEntity = nullptr;
    float m_selectedColor[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<Entity*> planetList;

    bool physicsPaused = false;
    bool simulationRunning = false;
    float m_massScale = 1.0f;

    struct PlanetSnapshot {
        Entity* entity = nullptr;
        Vec3 position;
        Vec3 velocity;
        Vec3 scale;
        float atmosphereThickness = 0.0f;
        bool  hasAtmosphere = false;
    };
    std::vector<PlanetSnapshot> m_simSnapshot;
    bool m_hasSnapshot = false;

    ToolMode m_toolMode = ToolMode::Selection;   // active tool

    PlaceState m_placeState = PlaceState::None;
    int m_selectedBodyIndex = -1;
    
    Vec3 m_placePosition;
    Vec3 m_placeVelocity;
    Vec2 m_startMousePos;

    Entity* m_ghostEntity = nullptr;
    float m_velocityScale = 0.5f;
    bool m_reallocatingExisting = false;
    Vec3 m_preReallocColor = Vec3(1.0f, 1.0f, 1.0f);

    bool m_prevLeftPressed = false;
    bool m_prevRightPressed = false;

    struct DestroyAnimation {
        Vec3   center;
        float  planetRadius;
        float  elapsed   = 0.0f;
        float  duration  = 0.6f;
        bool   done      = false;
        Entity* entity   = nullptr;
    };

    std::vector<DestroyAnimation> m_destroyAnims;

    std::unordered_map<Entity*, std::vector<Vec3>> m_trails;

    Entity* m_velocityTarget = nullptr; 
    Vec2   m_velDragStart;
    Vec3   m_velDragWorldOrigin;
    bool   m_velDragging = false; 
    static constexpr float kVelocityPixelScale = 0.0014f;
    static constexpr float kGravConst          = 0.02f; 
    static constexpr float kGravSoftening      = 100.0f;  // prevents singularity at close range
    static constexpr int   kMaxTrailPoints     = 300;

    bool  m_focusTransitioning = false;
    Vec3  m_focusStartPos;
    Vec3  m_focusEndPos;
    float m_focusProgress = 0.0f;
    float m_focusDuration = 0.6f;

    bool  m_followingPlanet = false;   // camera tracks selectedEntity during simulation

    PlanetRegistry m_registry;

    bool showPlanetsWindow = false;

    Entity* m_centralBody = nullptr;
public:
    SandboxMode(Game& game)
        : GameMode("assets/scenes/menu.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 2000.0f;

        Entity* earth = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Earth").get();
        if (earth) {
            if (const GravityBody* def = m_registry.find("Earth")) {
                def->apply(*earth->getComponent<PlanetComponent>());
            }
            earth->getComponent<PlanetComponent>()->getPlanetParams().velocity = Vec3(0.2f, 0.0f, 0.3f);
            planetList.push_back(earth);
        }

        m_centralBody = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Sun").get();
        if (m_centralBody) {
            m_centralBody->addComponent<PlanetComponent>(m_game, 0.0f, 900.0f, 100000.0f);
            auto* sunComp = m_centralBody->getComponent<PlanetComponent>();
            sunComp->initialize();
            sunComp->getPlanetParams().locked = true;
            sunComp->getPlanetParams().sunDir = Vec3(0.0f, 0.0f, 0.0f);
            sunComp->getPlanetParams().sunIntensity = 40.0f;
            sunComp->setHasAtmosphere(true);
            sunComp->getAtmosphere().thickness = 550.0f;
            sunComp->getAtmosphere().edgeFalloff = 1200.0f;
            sunComp->getAtmosphere().rayleighCoeff = Vec3(0.003f, 0.001f, 0.0f);
            planetList.push_back(m_centralBody);
        }

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Camera").get();
        if (m_camera) {
            m_orbitCamera = new OrbitCamera(m_camera);
            m_startPosition = m_camera->transform.position;
            m_startRotation = m_camera->transform.rotation;
        }

        m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");
        m_target->transform.position = Vec3(0, 0, 0);

        m_orbitCamera->SetTarget(m_target, startRadius);
        m_orbitCamera->SetMaxRadius(35000.0f);
    }

    void OnExit() override {
        delete m_orbitCamera;
    }

    // Helpers
    Vec3 GetMouseIntersection() {
        auto* camComp = m_camera->getComponent<CameraComponent>();
        if (!camComp) return m_target->transform.position;

        Vec2 mousePos = m_input.getMousePos();
        Ray ray = m_game.GetEngine().getRenderer().pickRay(mousePos.x, mousePos.y,
                                                           camComp->getCamera(),
                                                           m_camera->transform);

        float planeY = m_target->transform.position.y;
        if (std::abs(ray.direction.y) < 0.0001f) return ray.origin;
        
        float t = (planeY - ray.origin.y) / ray.direction.y;
        return ray.origin + ray.direction * t;
    }

    bool IsPositionValid(const Vec3& pos, float ghostRadius) {
        for (Entity* planet : planetList) {
            if (planet) {
                float planetRadius = planet->transform.scale.x;
                if ((pos - planet->transform.position).length() < (ghostRadius + planetRadius + 50.0f))
                    return false;
            }
        }
        return true;
    }

    const GravityBody* FindBodyDef(const std::string& name) const {
        return m_registry.find(name);
    }

    void SelectPlanet(Entity* planet) {
        if (!planet) return;

        m_toolMode = ToolMode::Selection;
        selectedEntity = planet;

        auto* rc = selectedEntity->getComponent<RenderComponent>();
        if (rc) {
            Vec3 col = rc->getBaseColor();
            m_selectedColor[0] = col.x;
            m_selectedColor[1] = col.y;
            m_selectedColor[2] = col.z;
        }

        m_followingPlanet    = true;
        m_focusStartPos      = m_target->transform.position;
        m_focusEndPos        = selectedEntity->transform.position;
        m_focusProgress      = 0.0f;
        m_focusTransitioning = true;
    }

    // TODO: move this to engine or a utility class, since it's useful in general
    bool WorldToScreen(const Vec3& worldPos, ImVec2& outScreen) const {
        Vec2 winSize  = m_game.GetEngine().getWindow().getSize();
        auto* camComp = m_camera->getComponent<CameraComponent>();
        Mat4 proj; camComp->getCamera().getProjectionMatrix(proj);

        Vec3 toPoint  = worldPos - m_camera->transform.position;
        Vec3 lookDir  = (m_target->transform.position - m_camera->transform.position).normalize();
        Vec3 worldUp(0.0f, 1.0f, 0.0f);
        Vec3 rightDir = cross(lookDir, worldUp).normalize();
        Vec3 upDir    = cross(rightDir, lookDir).normalize();

        float depth = dot(toPoint, lookDir);
        if (depth <= 0.0f) return false;

        float projX = dot(toPoint, rightDir) / depth * proj[0][0];
        float projY = dot(toPoint, upDir)    / depth * proj[1][1];

        outScreen.x = (projX + 1.0f) * 0.5f * winSize.x;
        outScreen.y = (1.0f - projY) * 0.5f * winSize.y;
        return true;
    }

    // Placement
    void StartPlacement(int index) {
        if (m_placeState != PlaceState::None) CancelPlacement();
        m_selectedBodyIndex = index;
        m_placeState = PlaceState::Positioning;
        m_reallocatingExisting = false;

        GravityBody& body = m_registry.bodies[index];

        m_ghostEntity = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity(body.name);

        if (body.isCustom) {
            m_ghostEntity->addComponent<RenderComponent>(
                body.modelPath,
                "assets/shaders/builtin/vert.glsl",
                "assets/shaders/builtin/frag.glsl",
                true);
            auto* planetComp = m_ghostEntity->addComponent<PlanetComponent>(m_game);

            std::shared_ptr<Texture> dummyTexture;
            planetComp->loadFromFile(body.customPath, dummyTexture);
        } else {
            m_ghostEntity->addComponent<RenderComponent>(body.modelPath);
            auto* planetComp = m_ghostEntity->addComponent<PlanetComponent>(m_game);
            body.apply(*planetComp);
            planetComp->initialize();
        }
    }

    void ConfirmPlacement() {
        bool validBodyDef = (m_selectedBodyIndex >= 0 &&
                             m_selectedBodyIndex < (int)m_registry.bodies.size());

        auto* rc = m_ghostEntity->getComponent<RenderComponent>();

        auto* planet = m_ghostEntity->getComponent<PlanetComponent>();
        if (!planet && validBodyDef) {
            GravityBody& body = m_registry.bodies[m_selectedBodyIndex];
            planet = m_ghostEntity->addComponent<PlanetComponent>(m_game);
            body.apply(*planet);
            planet->initialize();
        }

        planetList.push_back(m_ghostEntity);

        if (m_reallocatingExisting) {
            auto* finishedRc = m_ghostEntity->getComponent<RenderComponent>();
            if (finishedRc) finishedRc->setBaseColor(m_preReallocColor);
        }

        bool wasNewPlacement = !m_reallocatingExisting;
        Entity* placedEntity = m_ghostEntity;   // save before clearing

        m_ghostEntity = nullptr;
        m_reallocatingExisting = false;
        m_placeState = PlaceState::None;

        if (wasNewPlacement) {
            m_toolMode           = ToolMode::Velocity;
            m_velocityTarget     = placedEntity;
            m_velDragWorldOrigin = placedEntity->transform.position;
            ImVec2 screenCenter;
            if (WorldToScreen(m_velDragWorldOrigin, screenCenter))
                m_velDragStart = Vec2(screenCenter.x, screenCenter.y);
            else
                m_velDragStart = m_input.getMousePos();
            m_velDragging = true;
        }
    }

    void CancelPlacement() {
        if (m_ghostEntity) {
            if (m_reallocatingExisting) {
                auto* existingRc = m_ghostEntity->getComponent<RenderComponent>();
                if (existingRc) existingRc->setBaseColor(m_preReallocColor);
                planetList.push_back(m_ghostEntity);
            } else {
                m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(m_ghostEntity);
            }
            m_ghostEntity = nullptr;
        }
        m_reallocatingExisting = false;
        m_placeState = PlaceState::None;
    }

    // Planet picking
    int TryPickPlanet() {
        auto* camComp = m_camera->getComponent<CameraComponent>();
        if (!camComp) return -1;

        Vec2 mousePos = m_input.getMousePos();
        Ray ray = m_game.GetEngine().getRenderer().pickRay(mousePos.x, mousePos.y,
                                                           camComp->getCamera(),
                                                           m_camera->transform);

        Vec2 winSize = m_game.GetEngine().getWindow().getSize();
        
        const Mat4& proj = *reinterpret_cast<const Mat4*>(camComp->getCamera().getProjectionMatrix());

        int   bestIdx   = -1;
        float bestScore = 1e9f;

        for (int i = 0; i < (int)planetList.size(); i++) {
            Entity* planet = planetList[i];
            if (!planet) continue;

            float physicsRadius = planet->transform.scale.x;
            Vec3  toCenter = planet->transform.position - ray.origin;
            float t        = dot(toCenter, ray.direction);
            if (t < 0.0f) continue;

            float worldDist = (ray.origin + ray.direction * t - planet->transform.position).length();
            float distToCam = toCenter.length();
            
            float ndcRadius = (physicsRadius / std::max(distToCam, 1.0f)) * proj[0][0];
            float pixelRadius = std::max(ndcRadius * winSize.x * 0.5f, 20.0f);
            float pickThreshold = (pixelRadius / (winSize.x * 0.5f)) * distToCam / proj[0][0];

            if (worldDist < pickThreshold && t < bestScore) {
                bestScore = t;
                bestIdx   = i;
            }
        }
        return bestIdx;
    }

    // interaction logic per tool
    void HandlePlacementLogic() {
        bool leftPressed = m_input.isMouseButtonPressed(MOUSE_LEFT);
        bool rightPressed = m_input.isMouseButtonPressed(MOUSE_RIGHT);

        bool leftJustPressed = leftPressed && !m_prevLeftPressed;
        bool rightJustPressed = rightPressed && !m_prevRightPressed;

        if (m_placeState == PlaceState::Positioning && m_ghostEntity) {
            Vec3 intersect = GetMouseIntersection();
            float ghostRadius = m_ghostEntity->transform.scale.x;
            bool valid = IsPositionValid(intersect, ghostRadius);

            m_ghostEntity->transform.position = intersect;
            float ghostIntensity = sin((m_game.GetEngine().getTime() * 5.0f)) * 0.25f + .5f;
            auto* ghostRc = m_ghostEntity->getComponent<RenderComponent>();
            if (ghostRc) {
                ghostRc->setBaseColor(
                    valid ? Vec3(0.5f, 1.0f, 0.5f) * ghostIntensity
                          : Vec3(1.0f, 0.4f, 0.4f) * ghostIntensity);
            }

            if (!ImGui::GetIO().WantCaptureMouse) {
                if (leftJustPressed && valid) { ConfirmPlacement(); return; }
                if (rightJustPressed)         { CancelPlacement();  return; }
            }
            return;
        }

        if (ImGui::GetIO().WantCaptureMouse) return;

        // active tool
        switch (m_toolMode) { 

        case ToolMode::Selection:
            if (leftJustPressed) {
                int idx = TryPickPlanet();
                if (idx >= 0) {
                    SelectPlanet(planetList[idx]);
                } else {
                    selectedEntity = nullptr;   // click empty space to deselect
                    m_followingPlanet = false;
                }
            }
            break;

        case ToolMode::Reallocation:
            if (m_placeState == PlaceState::None && leftJustPressed) {
                int idx = TryPickPlanet();
                if (idx >= 0) {
                    m_ghostEntity = planetList[idx];
                    planetList.erase(planetList.begin() + idx);
                    m_selectedBodyIndex = -1;
                    for (int i = 0; i < (int)m_registry.bodies.size(); i++) {
                        if (m_registry.bodies[i].name == m_ghostEntity->name) {
                            m_selectedBodyIndex = i;
                            break;
                        }
                    }
                    m_reallocatingExisting = true;
                    auto* existingRc = m_ghostEntity->getComponent<RenderComponent>();
                    if (existingRc) m_preReallocColor = existingRc->getBaseColor();
                    m_placeState = PlaceState::Positioning;
                }
            }
            break;

        case ToolMode::Velocity:
            if (simulationRunning) {
                m_velDragging = false;
                m_velocityTarget = nullptr;
                break;
            }

            if (!m_velDragging) {
                if (leftJustPressed) {
                    int idx = TryPickPlanet();
                    if (idx >= 0) {
                        m_velocityTarget     = planetList[idx];
                        m_velDragWorldOrigin = m_velocityTarget->transform.position;
                        ImVec2 screenCenter;
                        if (WorldToScreen(m_velDragWorldOrigin, screenCenter))
                            m_velDragStart = Vec2(screenCenter.x, screenCenter.y);
                        else
                            m_velDragStart = m_input.getMousePos();
                        m_velDragging = true;
                    }
                }
            } else {
                if (rightJustPressed) {
                    m_velDragging   = false;
                    m_velocityTarget = nullptr;
                    break;
                }

                if (!leftPressed) {
                    Vec2 mouseNow = m_input.getMousePos();
                    Vec2 delta    = mouseNow - m_velDragStart;  

                    Vec3 lookDir  = (m_target->transform.position - m_camera->transform.position).normalize();
                    Vec3 worldUp(0.0f, 1.0f, 0.0f);
                    Vec3 rightDir = cross(lookDir, worldUp).normalize();
                    Vec3 camUpDir = cross(rightDir, lookDir).normalize();

                    Vec3 velocity = (rightDir * delta.x - camUpDir * delta.y) * kVelocityPixelScale;

                    auto* planetComp = m_velocityTarget->getComponent<PlanetComponent>();
                    if (planetComp) planetComp->getPlanetParams().velocity = velocity;

                    m_velDragging    = false;
                    m_velocityTarget = nullptr;
                }
            }
            break;
        }
    }

    // Overlay
    void DrawVelocityArrow() {
        if (m_toolMode != ToolMode::Velocity || !m_velDragging || !m_velocityTarget)
            return;

        ImVec2 screenOrigin;
        if (!WorldToScreen(m_velDragWorldOrigin, screenOrigin)) return;

        Vec2 mouseNow = m_input.getMousePos();
        ImVec2 tip(mouseNow.x, mouseNow.y);

        Vec2 delta(tip.x - screenOrigin.x, tip.y - screenOrigin.y);
        float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        dl->AddLine(screenOrigin, tip, IM_COL32(255, 220, 50, 220), 2.5f);

        if (length > 8.0f) {
            Vec2 dir (delta.x / length, delta.y / length);
            Vec2 perp(-dir.y, dir.x);
            float hs = 8.0f;
            ImVec2 p1(tip.x - dir.x * hs + perp.x * hs * 0.5f,
                      tip.y - dir.y * hs + perp.y * hs * 0.5f);
            ImVec2 p2(tip.x - dir.x * hs - perp.x * hs * 0.5f,
                      tip.y - dir.y * hs - perp.y * hs * 0.5f);
            dl->AddTriangleFilled(tip, p1, p2, IM_COL32(255, 220, 50, 220));
        }

        float speed = length * kVelocityPixelScale;
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f u/s", speed * 1000.0f);
        dl->AddText(ImVec2(tip.x + 8.0f, tip.y - 14.0f), IM_COL32(255, 255, 255, 200), buf);

        dl->AddCircleFilled(screenOrigin, 5.0f, IM_COL32(255, 220, 50, 220));
    }

    static constexpr float kVelocityArrowWorldScale = 5.0f;

   void DrawAllVelocityVectors() {
    if (m_toolMode != ToolMode::Velocity) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    auto* camComp = m_camera->getComponent<CameraComponent>();
    if (!camComp) return;

    Vec3 camPos  = m_camera->transform.position;
    Vec3 lookDir = (m_target->transform.position - camPos).normalize();
    Vec3 worldUp(0.0f, 1.0f, 0.0f);
    Vec3 camRight = cross(lookDir, worldUp).normalize();
    Vec3 camUp    = cross(camRight, lookDir).normalize();

    Mat4 proj;
    camComp->getCamera().getProjectionMatrix(proj);
    Vec2 winSize = m_game.GetEngine().getWindow().getSize();

    const float kArrowPixels = 80.0f;

    for (Entity* planet : planetList) {
        if (!planet) continue;
        if (m_velDragging && planet == m_velocityTarget) continue;

        auto* planetComp = planet->getComponent<PlanetComponent>();
        if (!planetComp) continue;

        Vec3 vel = planetComp->getPlanetParams().velocity;
        float speed = vel.length();
        if (speed < 0.0001f) continue;

        Vec3 velDir      = vel * (1.0f / speed);
        Vec3 worldOrigin = planet->transform.position;

        ImVec2 screenOrigin;
        if (!WorldToScreen(worldOrigin, screenOrigin)) continue;

        float depth = dot(worldOrigin - camPos, lookDir);
        if (depth <= 0.0f) continue;
        float pixelToWorld = (depth / proj[0][0]) * (2.0f / winSize.x);

        Vec3  worldTip = worldOrigin + velDir * kArrowPixels * pixelToWorld;
        ImVec2 tip;
        if (!WorldToScreen(worldTip, tip)) continue;

        Vec2 delta(tip.x - screenOrigin.x, tip.y - screenOrigin.y);
        float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        const ImU32 col = IM_COL32(120, 200, 255, 220);

        dl->AddLine(screenOrigin, tip, col, 2.5f);

        if (length > 8.0f) {
            Vec2 dir (delta.x / length, delta.y / length);
            Vec2 perp(-dir.y, dir.x);
            float hs = 8.0f;
            ImVec2 p1(tip.x - dir.x * hs + perp.x * hs * 0.5f,
                      tip.y - dir.y * hs + perp.y * hs * 0.5f);
            ImVec2 p2(tip.x - dir.x * hs - perp.x * hs * 0.5f,
                      tip.y - dir.y * hs - perp.y * hs * 0.5f);
            dl->AddTriangleFilled(tip, p1, p2, col);
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f u/s", speed * 1000.0f);
        dl->AddText(ImVec2(tip.x + 8.0f, tip.y - 14.0f),
                    IM_COL32(255, 255, 255, 200), buf);

        dl->AddCircleFilled(screenOrigin, 5.0f, col);
    }
}

    void DrawSelectionHighlight() {
        if (m_toolMode != ToolMode::Selection || !selectedEntity) return;

        ImVec2 screenPos;
        if (!WorldToScreen(selectedEntity->transform.position, screenPos)) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->AddCircleFilled(screenPos, 5.0f, IM_COL32(100, 200, 255, 230));
        dl->AddCircle(screenPos, 7.0f, IM_COL32(100, 200, 255, 120), 16, 1.0f);
    }

    void UpdateFocusTransition(float dt) {
        if (!m_focusTransitioning) return;

        m_focusProgress += dt / m_focusDuration;
        if (m_focusProgress >= 1.0f) {
            m_focusProgress      = 1.0f;
            m_focusTransitioning = false;
        }

        float t = m_focusProgress * m_focusProgress * (3.0f - 2.0f * m_focusProgress);
        m_target->transform.position = Vec3::lerp(m_focusStartPos, m_focusEndPos, t);
    }

    void DestroyPlanet(Entity* planet) {
        if (!planet) return;

        if (planet == m_centralBody) return;

        DestroyAnimation anim;
        anim.center       = planet->transform.position;
        anim.planetRadius = planet->transform.scale.x;
        anim.entity       = planet;
        m_destroyAnims.push_back(anim);

        if (selectedEntity == planet) { selectedEntity = nullptr; m_followingPlanet = false; }

        auto it = std::find(planetList.begin(), planetList.end(), planet);
        if (it != planetList.end()) planetList.erase(it);
    }

    void UpdateDestroyAnimations(float dt) {
        for (auto& anim : m_destroyAnims) {
            if (anim.done) continue;
            anim.elapsed += dt;
            float t = anim.elapsed / anim.duration;

            if (anim.entity) {
                float shrinkT = std::min(t * 2.0f, 1.0f);
                float scale   = anim.planetRadius * (1.0f - shrinkT);
                anim.entity->transform.scale = Vec3(scale, scale, scale);
                if (anim.entity->getComponent<PlanetComponent>()->hasAtmosphere()) {
                    float atmosScale = anim.entity->getComponent<PlanetComponent>()->getAtmosphere().thickness;
                    anim.entity->getComponent<PlanetComponent>()->getAtmosphere().thickness = atmosScale * (1.0f - shrinkT);
                }

                if (shrinkT >= 1.0f) {
                    if (simulationRunning) {
                        anim.entity->transform.scale = Vec3(0.0f, 0.0f, 0.0f);
                    } else {
                        m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(anim.entity);
                    }
                    anim.entity = nullptr;
                }
            }

            if (t >= 1.0f) anim.done = true;
        }
        m_destroyAnims.erase(
            std::remove_if(m_destroyAnims.begin(), m_destroyAnims.end(),
                           [](const DestroyAnimation& a){ return a.done; }),
            m_destroyAnims.end());
    }

    void DrawDestroyAnimations() {
        if (m_destroyAnims.empty()) return;
        auto& renderer = m_game.GetEngine().getRenderer();
        auto& cam      = m_camera->getComponent<CameraComponent>()->getCamera();

        constexpr int kSegments = 24;
        constexpr float kStep   = 2.0f * 3.14159265f / kSegments;

        for (const auto& anim : m_destroyAnims) {
            float t = anim.elapsed / anim.duration;

            float ringT = (t - 0.5f) * 2.0f;
            if (ringT <= 0.0f) continue;

            float alpha  = 1.0f - ringT;
            float radius = anim.planetRadius * (1.0f + ringT * 2.0f);
            Vec3  color  = Vec3(1.0f, 0.8f, 0.3f) * alpha;

            for (int i = 0; i < kSegments; ++i) {
                float a0 = i       * kStep;
                float a1 = (i + 1) * kStep;
                Vec3 p0 = anim.center + Vec3(std::cos(a0) * radius, 0.0f, std::sin(a0) * radius);
                Vec3 p1 = anim.center + Vec3(std::cos(a1) * radius, 0.0f, std::sin(a1) * radius);
                renderer.drawLine(p0, p1, cam, m_camera->transform, color, 1.0f, false);
            }
        }
    }

    void StartSimulation() {
        m_simSnapshot.clear();
        for (Entity* planet : planetList) {
            if (!planet) continue;
            auto* pc = planet->getComponent<PlanetComponent>();
            if (!pc) continue;
            PlanetSnapshot snap;
            snap.entity        = planet;
            snap.position      = planet->transform.position;
            snap.velocity      = pc->getPlanetParams().velocity;
            snap.scale         = planet->transform.scale;
            snap.hasAtmosphere = pc->hasAtmosphere();
            if (snap.hasAtmosphere)
                snap.atmosphereThickness = pc->getAtmosphere().thickness;
            m_simSnapshot.push_back(snap);
        }
        m_hasSnapshot     = true;
        simulationRunning = true;
        physicsPaused     = false;
        m_trails.clear();
    }

    void PauseSimulation() {
        if (!simulationRunning) return;
        physicsPaused = true;
    }

    void ResumeSimulation() {
        if (!simulationRunning) return;
        physicsPaused = false;
    }

    void ResetSimulation() {
        if (!m_hasSnapshot) return;
        simulationRunning  = false;
        physicsPaused      = false;
        m_followingPlanet  = false;
        m_destroyAnims.clear();

        for (auto& snap : m_simSnapshot) {
            if (!snap.entity) continue;

            snap.entity->transform.position = snap.position;
            snap.entity->transform.scale    = snap.scale;

            auto* pc = snap.entity->getComponent<PlanetComponent>();
            if (pc) {
                pc->getPlanetParams().velocity = snap.velocity;
                if (snap.hasAtmosphere)
                    pc->getAtmosphere().thickness = snap.atmosphereThickness;
            }

            bool alive = std::find(planetList.begin(), planetList.end(), snap.entity) != planetList.end();
            if (!alive) planetList.push_back(snap.entity);
        }
        m_trails.clear();
    }

    void StepSimulation(float dt) {
        float scaledDt = dt * m_game.timeScale;

        struct Body {
            Entity*          entity;
            PlanetComponent* pc;
            float            mass;
            float            collisionRadius; // matches transform.scale.x used elsewhere
            Vec3             pos;
            Vec3             vel;
            bool             locked;
        };

        std::vector<Body> bodies;
        bodies.reserve(planetList.size());
        for (Entity* planet : planetList) {
            if (!planet) continue;
            auto* pc = planet->getComponent<PlanetComponent>();
            if (!pc) continue;
            Body b;
            b.entity          = planet;
            b.pc              = pc;
            b.mass            = pc->getPlanetParams().mass * m_massScale;
            b.collisionRadius = planet->transform.scale.x;
            b.pos             = planet->transform.position;
            b.vel             = pc->getPlanetParams().velocity;
            b.locked          = pc->getPlanetParams().locked;
            bodies.push_back(b);
        }

        std::vector<Vec3> accels(bodies.size(), Vec3(0.0f, 0.0f, 0.0f));

        for (int i = 0; i < (int)bodies.size(); ++i) {
            if (bodies[i].locked) continue;
            for (int j = 0; j < (int)bodies.size(); ++j) {
                if (i == j) continue;
                Vec3  d         = bodies[j].pos - bodies[i].pos;
                float dist2     = d.x*d.x + d.y*d.y + d.z*d.z;
                float softDist2 = dist2 + kGravSoftening * kGravSoftening;
                float invDist   = 1.0f / std::sqrt(softDist2);
                // a = G * M * r_hat / (r² + ε²)  — softened to stay stable at close range
                accels[i] += d * (kGravConst * bodies[j].mass * invDist * invDist * invDist);
            }
        }

        for (int i = 0; i < (int)bodies.size(); ++i) {
            if (bodies[i].locked) continue;
            bodies[i].vel += accels[i] * scaledDt;
            bodies[i].pos += bodies[i].vel * scaledDt;
        }

        for (int i = 0; i < (int)bodies.size(); ++i) {
            bodies[i].pc->getPlanetParams().velocity = bodies[i].vel;
            bodies[i].entity->transform.position     = bodies[i].pos;

            if (drawTrails && !bodies[i].locked) {
                auto& trail = m_trails[bodies[i].entity];
                // Sample every ~30 world units so density stays consistent at any timeScale
                if (trail.empty() || (trail.back() - bodies[i].pos).length() > 30.0f) {
                    trail.push_back(bodies[i].pos);
                    if ((int)trail.size() > kMaxTrailPoints)
                        trail.erase(trail.begin());
                }
            }
        }

        std::vector<Entity*> toDestroy;

        for (int i = 0; i < (int)bodies.size(); ++i) {
            for (int j = i + 1; j < (int)bodies.size(); ++j) {
                float threshold = (bodies[i].collisionRadius + bodies[j].collisionRadius) * 0.6f;
                if ((bodies[j].pos - bodies[i].pos).length() >= threshold) continue;

                int survIdx = (bodies[i].mass >= bodies[j].mass) ? i : j;
                int killIdx = (survIdx == i) ? j : i;

                Entity* killEnt = bodies[killIdx].entity;
                Entity* survEnt = bodies[survIdx].entity;

                if (std::find(toDestroy.begin(), toDestroy.end(), killEnt) != toDestroy.end()) continue;
                if (std::find(toDestroy.begin(), toDestroy.end(), survEnt) != toDestroy.end()) continue;

                float totalMass = bodies[i].mass + bodies[j].mass;
                Vec3  newVel    = (bodies[i].vel * bodies[i].mass + bodies[j].vel * bodies[j].mass)
                                  * (1.0f / totalMass);
                bodies[survIdx].vel = newVel;
                bodies[survIdx].pc->getPlanetParams().velocity = newVel;

                toDestroy.push_back(killEnt);
            }
        }

        for (Entity* e : toDestroy) {
            m_trails.erase(e);
            DestroyPlanet(e);
        }
    }

    void DrawTrails() {
        if (!drawTrails || !simulationRunning || m_trails.empty()) return;

        auto& renderer = m_game.GetEngine().getRenderer();
        auto& cam      = m_camera->getComponent<CameraComponent>()->getCamera();

        for (auto& [entity, trail] : m_trails) {
            if ((int)trail.size() < 2) continue;
            int n = (int)trail.size();
            for (int i = 1; i < n; ++i) {
                float t      = (float)i / (float)(n - 1); // 0 = oldest, 1 = newest
                float bright = t * t;                      // quadratic fade: dim old, bright recent
                Vec3  color  = Vec3(0.25f, 0.55f, 1.0f) * bright;
                renderer.drawLine(trail[i-1], trail[i], cam, m_camera->transform, color, 10.0f, false);
            }
        }
    }

    void Update() override {
        float dt = m_game.GetEngine().getDeltaTime();

        if (drawGrid && movingEnabled) {
            DrawGrid(m_game.GetEngine().getRenderer(), m_camera);
        }

        DrawDestroyAnimations();
        DrawTrails();

        if (!physicsPaused) {
            Entity* sunEntity = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Sun").get();

            for (Entity* planet : planetList) {
                if (planet) {
                    auto* planetComp = planet->getComponent<PlanetComponent>();
                    if (planetComp) {
                        if (sunEntity && planet != sunEntity) {
                            Vec3 toSun = sunEntity->transform.position - planet->transform.position;
                            float dist = toSun.length();
                            if (dist > 0.0001f)
                                planetComp->getPlanetParams().sunDir = toSun * (1.0f / dist);
                        }

                        planetComp->update(dt * m_game.timeScale);
                    }
                }
            }

            if (simulationRunning)
                StepSimulation(dt);
        }

        if (isTransitioning) Transition(dt);

        if (movingEnabled) {
            if (m_followingPlanet && selectedEntity && simulationRunning) {
                if (m_focusTransitioning)
                    m_focusEndPos = selectedEntity->transform.position;
                else
                    m_target->transform.position = selectedEntity->transform.position;
            }
            UpdateFocusTransition(dt);
        }

        if (movingEnabled) {
            Vec3 camForward = m_camera->transform.forward();
            Vec3 forwardXZ = Vec3(camForward.x, 0.0f, camForward.z).normalize();

            if (forwardXZ.length() < 0.0001f) forwardXZ = Vec3(0, 0, 1);
            
            Vec3 rightXZ = Vec3(forwardXZ.z, 0.0f, -forwardXZ.x);

            Vec3 movement(0, 0, 0);
            if (m_input.isKeyDown(KEY_W)) { movement += forwardXZ; m_followingPlanet = false; }
            if (m_input.isKeyDown(KEY_S)) { movement -= forwardXZ; m_followingPlanet = false; }
            if (m_input.isKeyDown(KEY_A)) { movement += rightXZ;   m_followingPlanet = false; }
            if (m_input.isKeyDown(KEY_D)) { movement -= rightXZ;   m_followingPlanet = false; }

            m_target->transform.position += movement * dt * moveSpeed;

            moveSpeed = 100.0f * m_orbitCamera->GetRadius() * dt;

            bool isRightClicked = m_input.isMouseButtonPressed(MOUSE_RIGHT);

            if (m_placeState == PlaceState::Positioning || isRightClicked) {
                m_input.setCursorMode(true);
            } else {
                m_input.setCursorMode(false);
            }

            bool velToolBlocking = (m_toolMode == ToolMode::Velocity && m_velDragging);
            if (m_placeState == PlaceState::None && isRightClicked && !velToolBlocking) {
                m_orbitCamera->Update(&m_input, dt);
                m_orbitCamera->ApplyScroll(&m_input);
            } else {
                m_orbitCamera->ApplyPosition();
                m_orbitCamera->ApplyScroll(&m_input);
            }

            if (!simulationRunning) {
                HandlePlacementLogic();
            } else if (m_toolMode == ToolMode::Selection) {
                bool leftJustPressed = m_input.isMouseButtonPressed(MOUSE_LEFT) && !m_prevLeftPressed;
                if (!ImGui::GetIO().WantCaptureMouse && leftJustPressed) {
                    int idx = TryPickPlanet();
                    if (idx >= 0) SelectPlanet(planetList[idx]);
                    else { selectedEntity = nullptr; m_followingPlanet = false; }
                }
            } else if (m_toolMode == ToolMode::Velocity) {
                HandlePlacementLogic();
            }

            if (m_input.isKeyPressed(KEY_G)) drawGrid = !drawGrid;
            if (m_input.isKeyDown(KEY_LSHIFT)) m_target->transform.position.y += 10.0f;
            if (m_input.isKeyDown(KEY_LCTRL))  m_target->transform.position.y -= 10.0f;

            if (m_input.isKeyPressed(KEY_1)) m_toolMode = ToolMode::Selection;
            if (!simulationRunning) {
                if (m_input.isKeyPressed(KEY_2)) m_toolMode = ToolMode::Reallocation;
            }
            if (m_input.isKeyPressed(KEY_3)) m_toolMode = ToolMode::Velocity;

            if (m_input.isKeyPressed(KEY_P)) {
                if (!simulationRunning) StartSimulation();
                else                    ResumeSimulation();
            }
            if (m_input.isKeyPressed(KEY_SPACE)) PauseSimulation();
            if (m_input.isKeyPressed(KEY_R))     ResetSimulation();
        }

        m_prevLeftPressed = m_input.isMouseButtonPressed(MOUSE_LEFT);
        m_prevRightPressed = m_input.isMouseButtonPressed(MOUSE_RIGHT);

        UpdateDestroyAnimations(dt);

        if (m_centralBody) {
            auto* sunComp = m_centralBody->getComponent<PlanetComponent>();
            if (sunComp) {
                float distToSun = (m_camera->transform.position - m_centralBody->transform.position).length();
                sunComp->getAtmosphere().thickness = std::max(550.0f, distToSun * 0.1f);
            }
        }
    }

    void LateUpdate() override {
        if (!isTransitioning) {
            DrawSelectionHighlight();
            DrawAllVelocityVectors();
            DrawVelocityArrow();
        }
        DrawUI();
        DrawMainMenuBar();

        if (showPlanetsWindow) DrawPlanetsWindow();

        if (m_selector.isOpen())     m_selector.Draw();
        std::string selectedFile = m_selector.ConsumeSelection();
        if (!selectedFile.empty()) {
            int idx = m_registry.loadCustom(selectedFile);
            if (idx >= 0)
                StartPlacement(idx);
        }

        if (m_loadSelector.isOpen()) m_loadSelector.Draw();
        std::string sandboxFile = m_loadSelector.ConsumeSelection();
        if (!sandboxFile.empty()) {
            LoadSandbox(sandboxFile);
        }

        DrawSaveDialog();
    }

    // Save/Loading
    void SaveSandbox(const std::string& name) {
        std::string dir = Path::resolve("user/sandbox/");
        std::filesystem::create_directories(dir);

        nlohmann::json j;
        j["planets"] = nlohmann::json::array();

        for (Entity* planet : planetList) {
            if (!planet || planet == m_centralBody) continue;

            auto* pc = planet->getComponent<PlanetComponent>();
            if (!pc) continue;

            nlohmann::json entry;
            entry["name"]     = planet->name;
            entry["position"] = { planet->transform.position.x,
                                  planet->transform.position.y,
                                  planet->transform.position.z };
            entry["velocity"] = { pc->getPlanetParams().velocity.x,
                                  pc->getPlanetParams().velocity.y,
                                  pc->getPlanetParams().velocity.z };

            const GravityBody* def = m_registry.find(planet->name);
            if (def && def->isCustom) {
                entry["isCustom"]   = true;
                entry["customPath"] = def->customPath;
            } else if (def) {
                entry["isReal"]        = true;
                entry["registryName"]  = def->name;
            } else {
                continue;
            }

            j["planets"].push_back(entry);
        }

        std::string savePath = dir + name + ".sandbox";
        std::ofstream file(savePath);
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
            Logger::info("Saved sandbox to " + savePath);
        } else {
            Logger::error("Failed to save sandbox to " + savePath);
        }
    }

    void LoadSandbox(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::error("Failed to open sandbox file: " + filepath);
            return;
        }

        nlohmann::json j;
        try {
            file >> j;
        } catch (const nlohmann::json::parse_error& e) {
            Logger::error("Failed to parse sandbox file: " + std::string(e.what()));
            return;
        }

        ResetSandbox();

        if (!j.contains("planets")) return;

        for (const auto& entry : j["planets"]) {
            Vec3 position(0.0f, 0.0f, 0.0f);
            Vec3 velocity(0.0f, 0.0f, 0.0f);

            if (entry.contains("position")) {
                position = Vec3(entry["position"][0], entry["position"][1], entry["position"][2]);
            }
            if (entry.contains("velocity")) {
                velocity = Vec3(entry["velocity"][0], entry["velocity"][1], entry["velocity"][2]);
            }

            if (entry.value("isReal", false)) {
                std::string regName = entry.value("registryName", "");
                const GravityBody* def = m_registry.find(regName);
                if (!def) continue;

                if (regName == "Earth") {
                    for (Entity* planet : planetList) {
                        if (planet && planet->name == "Earth") {
                            planet->transform.position = position;
                            planet->getComponent<PlanetComponent>()->getPlanetParams().velocity = velocity;
                            break;
                        }
                    }
                    continue;
                }

                int idx = -1;
                for (int i = 0; i < (int)m_registry.bodies.size(); i++) {
                    if (m_registry.bodies[i].name == regName) { idx = i; break; }
                }
                if (idx < 0) continue;

                const GravityBody& body = m_registry.bodies[idx];
                Entity* e = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity(body.name);
                e->addComponent<RenderComponent>(body.modelPath);
                auto* pc = e->addComponent<PlanetComponent>(m_game);
                body.apply(*pc);
                pc->initialize();
                e->transform.position          = position;
                pc->getPlanetParams().velocity = velocity;
                planetList.push_back(e);

            } else if (entry.value("isCustom", false)) {
                std::string customPath = entry.value("customPath", "");
                if (customPath.empty()) continue;

                int idx = m_registry.loadCustom(customPath);
                if (idx < 0) continue;

                const GravityBody& body = m_registry.bodies[idx];
                Entity* e = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity(body.name);
                e->addComponent<RenderComponent>(
                    body.modelPath,
                    "assets/shaders/builtin/vert.glsl",
                    "assets/shaders/builtin/frag.glsl",
                    true);
                auto* pc = e->addComponent<PlanetComponent>(m_game);
                std::shared_ptr<Texture> dummyTexture;
                pc->loadFromFile(customPath, dummyTexture);
                e->transform.position          = position;
                pc->getPlanetParams().velocity = velocity;
                planetList.push_back(e);
            }
        }
    }

    void DrawSaveDialog() {
        if (!m_showSaveDialog) return;

        ImGui::SetNextWindowPos(
            ImVec2(m_game.GetEngine().getWindow().getSize().x * 0.5f - 160.0f,
                   m_game.GetEngine().getWindow().getSize().y * 0.5f - 75.0f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(320, 150), ImGuiCond_Always);
        ImGui::Begin("Save Sandbox", &m_showSaveDialog,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        ImGui::Text("Sandbox name:");
        ImGui::SetNextItemWidth(-1.0f);
        bool hitEnter = ImGui::InputText("##SaveName", m_saveNameBuf, sizeof(m_saveNameBuf),
                                         ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SetItemDefaultFocus();

        bool canSave = m_saveNameBuf[0] != '\0';
        if (!canSave) ImGui::BeginDisabled();
        if (ImGui::Button("Save", ImVec2(148, 0)) || (hitEnter && canSave)) {
            SaveSandbox(m_saveNameBuf);
            m_showSaveDialog = false;
            std::memset(m_saveNameBuf, 0, sizeof(m_saveNameBuf));
        }
        if (!canSave) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(148, 0))) {
            m_showSaveDialog = false;
            std::memset(m_saveNameBuf, 0, sizeof(m_saveNameBuf));
        }

        ImGui::End();
    }

    void ResetSandbox() {
        simulationRunning = false;
        physicsPaused     = false;
        m_hasSnapshot     = false;
        m_simSnapshot.clear();

        CancelPlacement();

        Entity* earth = nullptr;
        for (Entity* planet : planetList) {
            if (planet && planet != m_centralBody && planet->name == "Earth") {
                earth = planet;
                break;
            }
        }

        for (Entity* planet : planetList) {
            if (!planet || planet == m_centralBody || planet == earth) continue;
            m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(planet);
        }
        planetList.clear();
        if (m_centralBody)   planetList.push_back(m_centralBody);

        if (earth) {
            if (const GravityBody* def = m_registry.find("Earth")) {
                def->apply(*earth->getComponent<PlanetComponent>());
            }
            earth->getComponent<PlanetComponent>()->getPlanetParams().velocity = Vec3(0.2f, 0.0f, 0.3f);
            earth->transform.position = Vec3(0.0f, 0.0f, 0.0f);
            planetList.push_back(earth);
        }

        selectedEntity    = nullptr;
        m_followingPlanet = false;
        m_toolMode        = ToolMode::Selection;
        m_destroyAnims.clear();
        m_trails.clear();
        m_velDragging     = false;
        m_velocityTarget  = nullptr;
        m_target->transform.position = Vec3(0.0f, 0.0f, 0.0f);

        const float kPitch45 = 0.78539816f;
        const float kYaw45   = 0.78539816f;
        m_camera->transform.position = Vec3(
            startRadius * cosf(kPitch45) * cosf(kYaw45),
            startRadius * sinf(kPitch45),
            startRadius * cosf(kPitch45) * sinf(kYaw45)
        );
        m_camera->transform.rotation = Vec3(-45.0f, -135.0f, 0.0f);
        m_orbitCamera->SyncFromCurrentPosition();
    }

    void Transition(float dt) {
        if (!m_orbitCamera) return;

        m_transitionProgress += dt / m_transitionDuration;
        if (m_transitionProgress > 1.0f) m_transitionProgress = 1.0f;

        const float kPitch45 = 0.78539816f; // 45 degrees in radians
        const float kYaw45   = 0.78539816f;

        Vec3 targetPosition(
            startRadius * cosf(kPitch45) * cosf(kYaw45),
            startRadius * sinf(kPitch45),
            startRadius * cosf(kPitch45) * sinf(kYaw45)
        );

        Vec3 targetRotation(-45.0f, -135.0f, 0.0f);

        float t = m_transitionProgress * m_transitionProgress * (3.0f - 2.0f * m_transitionProgress);

        m_camera->transform.position = Vec3::lerp(m_startPosition, targetPosition, t);
        m_camera->transform.rotation = Vec3::lerp(m_startRotation, targetRotation, t);

        if (m_transitionProgress >= 1.0f) {
            movingEnabled = true;
            isTransitioning = false;
            m_orbitCamera->SyncFromCurrentPosition();
        }
    }

    void DrawGrid(Renderer& renderer, Entity* m_camera) {
        for (int i = -13; i <= 13; ++i) {
            Vec3 start = Vec3(i * 750.0f, m_target->transform.position.y, -9750.0f);
            Vec3 end   = Vec3(i * 750.0f, m_target->transform.position.y,  9750.0f);
            renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(),
                            m_camera->transform, Vec3(.005f, .005f, .005f), 10.0f, false);
            start = Vec3(-9750.0f, m_target->transform.position.y, i * 750.0f);
            end   = Vec3( 9750.0f, m_target->transform.position.y, i * 750.0f);
            renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(),
                            m_camera->transform, Vec3(.005f, .005f, .005f), 10.0f, false);
        }
    }

    // UI
    void DrawUI() {
        if (isTransitioning) return;

        auto windowSize = m_game.GetEngine().getWindow().getSize();

        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.5f, 5.0f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::Begin("##PlayPause", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::End();

        ImGuiID sandboxID  = ImGui::GetID("Sandbox");
        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.5f, (float)windowSize.y),
                                ImGuiCond_Always, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(500, 200.0), ImGuiCond_Always);

        int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | 
                    ImGuiWindowFlags_NoDocking;


        if (!m_showSandbox) return;
        ImGui::Begin("Sandbox", nullptr, flags);

        if (!ImGui::IsWindowCollapsed()) {
            if (ImGui::BeginTabBar("SandboxTabBar")) {

                if (ImGui::BeginTabItem("Tools")) {
                    float childH = ImGui::GetContentRegionAvail().y;
                    ImGui::BeginChild("ToolScrollList", ImVec2(0, childH));

                    float totalSpacing = ImGui::GetStyle().ItemSpacing.x * 2.0f;
                    float buttonW = (ImGui::GetContentRegionAvail().x - totalSpacing) / 3.0f;

                    auto toolButton = [&](const char* label, ToolMode mode) {
                        bool active = (m_toolMode == mode);
                        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        
                        if (ImGui::Button(label, ImVec2(buttonW, childH)))
                            m_toolMode = mode;
                            
                        if (active) ImGui::PopStyleColor();
                        ImGui::SameLine();
                    };

                    toolButton("Selection [1]",    ToolMode::Selection);

                    if (simulationRunning) ImGui::BeginDisabled();
                    toolButton("Reallocation [2]", ToolMode::Reallocation);
                    if (simulationRunning) ImGui::EndDisabled();
                    
                    toolButton("Velocity [3]",     ToolMode::Velocity);

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Planets")) {
                    float childH = ImGui::GetContentRegionAvail().y;
                    float totalW = ImGui::GetContentRegionAvail().x;
                    float rightSideW = 150.0f; 
                    float leftSideW = totalW - rightSideW - ImGui::GetStyle().ItemSpacing.x;

                    ImGui::BeginChild("EntityScrollList", ImVec2(leftSideW, childH),
                                    ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

                    if (simulationRunning) ImGui::BeginDisabled();

                    static int selectedType = 0; 

                    if (selectedType == 2) {
                        if (ImGui::Button("Load Custom Planet...", ImVec2(0, 30))) {
                            m_selector.toggleOpen();
                        }
                        ImGui::Separator();
                    }

                    for (int i = 0; i < (int)m_registry.bodies.size(); i++) {
                        bool show = false;
                        if (selectedType == 0 && m_registry.bodies[i].isReal && !m_registry.bodies[i].isCustom) show = true;
                        if (selectedType == 1 && !m_registry.bodies[i].isReal && !m_registry.bodies[i].isCustom) show = true;
                        if (selectedType == 2 && m_registry.bodies[i].isCustom) show = true;

                        if (show) {
                            if (ImGui::Button(m_registry.bodies[i].name.c_str(),
                                            ImVec2(100, ImGui::GetContentRegionAvail().y)))
                                StartPlacement(i);
                            ImGui::SameLine();
                        }
                    }

                    if (simulationRunning) ImGui::EndDisabled();

                    ImGui::EndChild();

                    ImGui::SameLine();

                    ImGui::BeginChild("PlanetTypeSelection", ImVec2(rightSideW, childH));
                    
                    ImGui::RadioButton("Solar System", &selectedType, 0);
                    ImGui::RadioButton("Campaign", &selectedType, 1);
                    ImGui::RadioButton("Custom", &selectedType, 2);

                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
                //if (ImGui::BeginTabItem("Central Body")) {
                //    ImGui::EndTabItem();
                //}
                if (ImGui::BeginTabItem("Properties")) {
                    if (selectedEntity && selectedEntity->getComponent<PlanetComponent>()) {
                        auto* pc = selectedEntity->getComponent<PlanetComponent>();

                        std::string name = selectedEntity->name;
                        float mass = pc->getPlanetParams().mass;
                        float radius = pc->getPlanetParams().radius;
                        float period = pc->getPlanetParams().period;

                        char nameBuf[64] = {};
                        strncpy(nameBuf, name.c_str(), sizeof(nameBuf) - 1);

                        float totalWidth = ImGui::GetContentRegionAvail().x;
                        float spacing    = ImGui::GetStyle().ItemSpacing.x;

                        ImGui::SetNextItemWidth((totalWidth - spacing) * 0.75f);
                        if (ImGui::InputText("##NameField", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                            selectedEntity->name = nameBuf;
                        }

                        ImGui::SameLine();  
                        bool locked = pc->getPlanetParams().locked;
                        if (ImGui::Checkbox("Locked", &locked)) {
                            pc->getPlanetParams().locked = locked;
                        }

                        float halfWidth = (totalWidth - spacing) * 0.5f;

                        ImGui::SetNextItemWidth(halfWidth);
                        if (ImGui::SliderFloat("##MassField", &mass, 1.0f, 1.0e10f, "Mass: %.3e", ImGuiSliderFlags_Logarithmic)) {
                            pc->getPlanetParams().mass = mass;
                        }
                        
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(halfWidth);
                        if (ImGui::SliderFloat("##RadiusField", &radius, 1.0f, 100000.0f, "Radius: %.1f", ImGuiSliderFlags_Logarithmic)) {
                            pc->getPlanetParams().radius = radius;
                            pc->applyScale();
                        }

                        ImGui::SetNextItemWidth(halfWidth);
                        if (ImGui::SliderFloat("##PeriodField", &period, 0.0f, 1000.0f, "Period: %.3f")) {
                            pc->getPlanetParams().period = period;
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(halfWidth);
                        Vec3 velocity = pc->getPlanetParams().velocity;
                        if (simulationRunning) ImGui::BeginDisabled();
                        velocity = 1000.0f * velocity;
                        if (ImGui::InputFloat3("##VelocityField", &velocity.x, "%.3f")) {
                            pc->getPlanetParams().velocity = velocity / 1000.0f;
                        }
                        if (simulationRunning) ImGui::EndDisabled();

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.65f, 0.10f, 0.10f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(0.85f, 0.18f, 0.18f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.95f, 0.30f, 0.30f, 1.00f));

                        if (selectedEntity != m_centralBody) {
                            if (ImGui::Button("DESTROY", ImVec2(totalWidth, 0))) {
                                Entity* toDestroy = selectedEntity;
                                DestroyPlanet(toDestroy);
                            }
                        }

                        ImGui::PopStyleColor(3);
                    } else {
                        float halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
                        m_ui.setFont(1);
                        float titleWidth = ImGui::CalcTextSize("No planet selected!").x;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() * 0.5f + 60.0f);
                        ImGui::SetCursorPosX(halfWidth - titleWidth * 0.5f);
                        ImGui::TextDisabled("No planet selected!");
                        m_ui.resetFont();
                        float subtitleWidth = ImGui::CalcTextSize("Use the Selection tool and with an object.").x;
                        ImGui::SetCursorPosX(halfWidth - subtitleWidth * 0.5f);
                        ImGui::TextDisabled("Use the Selection tool and with an object.");
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings")) {
                    if (ImGui::Button("Grid [G]")) drawGrid = !drawGrid;
                    ImGui::SameLine();
                    if (ImGui::Button("Orbit Trails [T]")) drawTrails = !drawTrails;
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Y")) m_target->transform.position.y = 0.0f;
                    float totalWidth = ImGui::GetContentRegionAvail().x;

                    ImGui::SetNextItemWidth(totalWidth);
                    ImGui::SliderFloat("##TimeScale", &m_game.timeScale,
                                       1.0f, 100000.0f, "Time Scale: %.2f",
                                       ImGuiSliderFlags_Logarithmic);
                    ImGui::SetNextItemWidth(totalWidth);
                    ImGui::SliderFloat("##MassScale", &m_massScale,
                                       0.01f, 100.0f, "Mass Scale: %.3fx",
                                       ImGuiSliderFlags_Logarithmic);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Simulation")) {
                    float spacing = ImGui::GetStyle().ItemSpacing.x;
                    float buttonW = (ImGui::GetContentRegionAvail().x - (spacing * 2.0f)) / 3.0f;

                    bool isPlaying = simulationRunning && !physicsPaused;

                    if (isPlaying) ImGui::BeginDisabled();
                    if (ImGui::Button("Play [P]", ImVec2(buttonW, -1))) {
                        if (!simulationRunning) StartSimulation();
                        else                    ResumeSimulation();
                    }
                    if (isPlaying) ImGui::EndDisabled();

                    ImGui::SameLine();

                    if (!simulationRunning) ImGui::BeginDisabled();
                    if (ImGui::Button("Pause [Space]", ImVec2(buttonW, -1))) {
                        PauseSimulation();
                    }
                    if (!simulationRunning) ImGui::EndDisabled();

                    ImGui::SameLine();

                    if (!m_hasSnapshot) ImGui::BeginDisabled();
                    if (ImGui::Button("Reset [R]", ImVec2(buttonW, -1))) {
                        ResetSimulation();
                    }
                    if (!m_hasSnapshot) ImGui::EndDisabled();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        ImGui::End();
    }

    void DrawMainMenuBar() {
        if (isTransitioning) return;

        ImGui::PushStyleColor(ImGuiCol_WindowBg,   ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg,  ImVec4(0, 0, 0, 0));
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Game")) {
                if (ImGui::MenuItem("Main Menu")) m_ui.loadMainMenu();
                if (ImGui::MenuItem("Settings"))  m_game.showSettings = !m_game.showSettings;
                if (ImGui::MenuItem("Quit"))      m_game.GetEngine().stop();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Sandbox")) {
                if (ImGui::MenuItem("New Sandbox")) { ResetSandbox(); }
                if (ImGui::MenuItem("Load Sandbox")) { m_loadSelector.toggleOpen(); }
                if (ImGui::MenuItem("Save Sandbox")) { m_showSaveDialog = true; }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Sandbox Panel")) m_showSandbox = !m_showSandbox;
                if (ImGui::MenuItem("Active Planets")) showPlanetsWindow = !showPlanetsWindow;
                if (ImGui::MenuItem("Grid", "G")) drawGrid = !drawGrid;
                if (ImGui::MenuItem("Orbit Trails", "T")) drawTrails = !drawTrails;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Selection",    "1")) m_toolMode = ToolMode::Selection;
                if (simulationRunning) ImGui::BeginDisabled();
                if (ImGui::MenuItem("Reallocation", "2")) m_toolMode = ToolMode::Reallocation;
                if (simulationRunning) ImGui::EndDisabled();
                if (ImGui::MenuItem("Velocity",     "3")) m_toolMode = ToolMode::Velocity;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Simulation")) {
                bool isPlaying = simulationRunning && !physicsPaused;

                if (ImGui::MenuItem("Play", "P", false, !isPlaying)) {
                    if (!simulationRunning) StartSimulation();
                    else                    ResumeSimulation();
                }
                if (ImGui::MenuItem("Pause", "Space", false, simulationRunning)) PauseSimulation();
                if (ImGui::MenuItem("Reset", "R", false, m_hasSnapshot)) ResetSimulation();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Planets")) {
                if (ImGui::MenuItem("Show in Window")) showPlanetsWindow = !showPlanetsWindow;
                ImGui::Separator();
                if (planetList.empty()) {
                    ImGui::TextDisabled("No planets placed.");
                } else {
                    for (Entity* planet : planetList) {
                        if (!planet) continue;
                        bool isSelected = (planet == selectedEntity);
                        if (ImGui::MenuItem(planet->name.c_str(), nullptr, isSelected)) {
                            SelectPlanet(planet);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            const char* statusText = simulationRunning ? (physicsPaused ? "Simulation Paused" : "Simulation Running") : "";
            float statusWidth = ImGui::CalcTextSize(statusText).x;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - statusWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.75f, 1.0f, 0.75f, 1.0f), "%s", statusText);

            const char* toolName;
            if (simulationRunning && m_toolMode == ToolMode::Reallocation) {
                toolName = physicsPaused ? "Simulation Paused" : "Simulation Running";
            } else {
                toolName = (m_toolMode == ToolMode::Selection)    ? "Selection"
                          : (m_toolMode == ToolMode::Reallocation) ? "Reallocation"
                                                                     : "Velocity";
            }
            float textWidth = ImGui::CalcTextSize(toolName).x + 8.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - textWidth);
            ImGui::TextDisabled("%s", toolName);
        }
        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor(2);
    }
    
    void DrawPlanetsWindow() {
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Planets & Stars", &showPlanetsWindow, ImGuiWindowFlags_NoCollapse)) {
            if (planetList.empty()) {
                ImGui::TextDisabled("No planets placed.");
            } else {
                for (Entity* planet : planetList) {
                    if (!planet) continue;
                    bool isSelected = (planet == selectedEntity);
                    if (ImGui::Selectable(planet->name.c_str(), isSelected)) {
                        SelectPlanet(planet);
                    }
                }
            }
        }
        ImGui::End();
    }
};