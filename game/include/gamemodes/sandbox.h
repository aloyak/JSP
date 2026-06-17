#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"

#include "grid.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/utils/logger.h"

class Game;

struct GravityBody {
    std::string modelPath = "assets/models/earth.fbx";
    float mass = 1.0f;
    float radius = 1.0f;
    float period = 24.0f;
    std::string name = "Unnamed";
};

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
    float m_selectedColor[3] = { 1.0f, 1.0f, 1.0f };   // color shown in Properties tab
    std::vector<Entity*> planetList;

    // simulation
    bool physicsPaused = false;
    bool simulationRunning = false;
    float m_massScale = 1.0f; // user-adjustable multiplier applied to all body masses in ApplyGravity

    struct PlanetSnapshot {
        Entity* entity = nullptr;
        Vec3 position;
        Vec3 velocity;
    };
    std::vector<PlanetSnapshot> m_simSnapshot;
    bool m_hasSnapshot = false;

    ToolMode m_toolMode = ToolMode::Reallocation;   // active tool

    PlaceState m_placeState = PlaceState::None;
    int m_selectedBodyIndex = -1;
    
    Vec3 m_placePosition;
    Vec3 m_placeVelocity;
    Vec2 m_startMousePos;

    Entity* m_ghostEntity = nullptr;
    float m_velocityScale = 0.5f;

    bool m_prevLeftPressed = false;
    bool m_prevRightPressed = false;

    // velocity-drag state
    Entity* m_velocityTarget = nullptr; 
    Vec2   m_velDragStart;
    Vec3   m_velDragWorldOrigin;
    bool   m_velDragging = false; 
    static constexpr float kVelocityPixelScale = 0.005f;

    bool  m_focusTransitioning = false;
    Vec3  m_focusStartPos;
    Vec3  m_focusEndPos;
    float m_focusProgress = 0.0f;
    float m_focusDuration = 0.6f;

    std::vector<GravityBody> gravityBodies = {
        { "assets/models/earth.fbx", 50, 637.1f, 24.0f, "Earth" },
        { "assets/models/moon.fbx", 7, 173.7f, 27.3f, "Moon" },
        { "assets/models/mars.fbx", 43, 338.9f, 24.6f, "Mars" },
        { "assets/models/venus.fbx", 23, 605.2f, 240.0f, "Venus" },
        { "assets/models/mercury.fbx", 5, 243.9f, 1408.0f, "Mercury" },
        { "assets/models/jupiter.fbx", 132, 6991.1f, 9.9f, "Jupiter" },
        { "assets/models/saturn.fbx", 56, 5823.2f, 10.7f, "Saturn" },
        { "assets/models/uranus.fbx", 75, 2536.2f, 17.2f, "Uranus" },
        { "assets/models/neptune.fbx", 17, 2462.2f, 16.1f, "Neptune" }
    };

    bool showPlanetsWindow = false;
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
        m_orbitCamera->SetMaxRadius(50000.0f);
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
        for (const auto& body : gravityBodies)
            if (body.name == name) return &body;
        return nullptr;
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

        m_focusStartPos      = m_target->transform.position;
        m_focusEndPos        = selectedEntity->transform.position;
        m_focusProgress      = 0.0f;
        m_focusTransitioning = true;
    }

    // -------------------------------------------------------------------
    // Simulation control
    static constexpr float kGravConstant = 6.674e-11f;

    void StartSimulation() {
        if (simulationRunning) return;

        // Snapshot current state so we can restore it on Reset.
        m_simSnapshot.clear();
        for (Entity* planet : planetList) {
            if (!planet) continue;
            PlanetSnapshot snap;
            snap.entity   = planet;
            snap.position = planet->transform.position;
            auto* pc = planet->getComponent<PlanetComponent>();
            snap.velocity = pc ? pc->getVelocity() : Vec3(0, 0, 0);
            m_simSnapshot.push_back(snap);
        }
        m_hasSnapshot = true;

        simulationRunning = true;
        physicsPaused     = false;

        // Simulation runs on its own, so deselect / cancel any in-progress tool action.
        CancelPlacement();
        m_velDragging    = false;
        m_velocityTarget = nullptr;
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

        for (const PlanetSnapshot& snap : m_simSnapshot) {
            if (!snap.entity) continue;
            snap.entity->transform.position = snap.position;
            auto* pc = snap.entity->getComponent<PlanetComponent>();
            if (pc) pc->setVelocity(snap.velocity);
        }

        simulationRunning = false;
        physicsPaused     = false;
    }

    // N-body gravitational attraction between all placed planets.
    // NOTE: positions/velocities are in engine units (1000 km = 100 units,
    // so 1 unit = 10 km = 1e4 m), but mass/G are SI. Convert distances to
    // meters for the force calculation, then convert resulting accel
    // (m/s^2) back to units/s^2.
    static constexpr float kUnitToMeters = 1.0e4f;

    void ApplyGravity(float dt) {
        int n = (int)planetList.size();
        if (n < 2) return;

        std::vector<Vec3> accel(n, Vec3(0, 0, 0));

        for (int i = 0; i < n; i++) {
            Entity* a = planetList[i];
            if (!a) continue;
            const GravityBody* bodyA = FindBodyDef(a->name);
            if (!bodyA) continue;

            for (int j = i + 1; j < n; j++) {
                Entity* b = planetList[j];
                if (!b) continue;
                const GravityBody* bodyB = FindBodyDef(b->name);
                if (!bodyB) continue;

                Vec3 diff = b->transform.position - a->transform.position;
                float distMeters = diff.length() * kUnitToMeters;
                float distSq = distMeters * distMeters;
                if (distSq < 1.0f) distSq = 1.0f; // avoid singularities

                Vec3 dir = diff.normalize();
                float scaledMassA = bodyA->mass * m_massScale;
                float scaledMassB = bodyB->mass * m_massScale;
                float forceMag = kGravConstant * scaledMassA * scaledMassB / distSq;

                // forceMag/mass is in m/s^2; convert to units/s^2
                accel[i] += dir * (forceMag / scaledMassA) / kUnitToMeters;
                accel[j] -= dir * (forceMag / scaledMassB) / kUnitToMeters;
            }
        }

        for (int i = 0; i < n; i++) {
            Entity* planet = planetList[i];
            if (!planet) continue;
            auto* pc = planet->getComponent<PlanetComponent>();
            if (!pc) continue;

            Vec3 vel = pc->getVelocity();
            vel += accel[i] * dt;
            pc->setVelocity(vel);
        }
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
        m_ghostEntity = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Ghost");
        m_ghostEntity->addComponent<RenderComponent>(gravityBodies[index].modelPath);

        float r = gravityBodies[index].radius;
        m_ghostEntity->transform.scale = Vec3(r, r, r);
    }

    void ConfirmPlacement() {
        GravityBody& body = gravityBodies[m_selectedBodyIndex];

        m_ghostEntity->name = body.name;
        m_ghostEntity->getComponent<RenderComponent>()->setBaseColor(Vec3(1.0f, 1.0f, 1.0f));

        auto* planet = m_ghostEntity->addComponent<PlanetComponent>(body.period, body.radius);
        planet->adjustScale();

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

        // ghost
        if (m_placeState == PlaceState::Positioning && m_ghostEntity) {
            Vec3 intersect = GetMouseIntersection();
            float ghostRadius = m_ghostEntity->transform.scale.x;
            bool valid = IsPositionValid(intersect, ghostRadius);

            m_ghostEntity->transform.position = intersect;
            float ghostIntensity = sin((m_game.GetEngine().getTime() * 5.0f)) * 0.25f + .5f;
            m_ghostEntity->getComponent<RenderComponent>()->setBaseColor(
                valid ? Vec3(0.5f, 1.0f, 0.5f) * ghostIntensity
                      : Vec3(1.0f, 0.4f, 0.4f) * ghostIntensity);

            if (!ImGui::GetIO().WantCaptureMouse) {
                if (leftJustPressed && valid) { ConfirmPlacement(); return; }
                if (rightJustPressed)         { CancelPlacement();  return; }
            }
            return;
        }

        if (ImGui::GetIO().WantCaptureMouse) return;

        // active tool
        switch (m_toolMode) {

        // SELECTION TOOL
        case ToolMode::Selection:
            if (leftJustPressed) {
                int idx = TryPickPlanet();
                if (idx >= 0) {
                    SelectPlanet(planetList[idx]);
                } else {
                    selectedEntity = nullptr;   // click empty space to deselect
                }
            }
            break;

        // REALLOCATION TOOL
        case ToolMode::Reallocation:
            if (m_placeState == PlaceState::None && leftJustPressed) {
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
            break;

        // VELOCITY TOOL
        case ToolMode::Velocity:
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
                    Vec2 delta    = mouseNow - m_velDragStart;  // screen-space pixels

                    Vec3 lookDir  = (m_target->transform.position - m_camera->transform.position).normalize();
                    Vec3 worldUp(0.0f, 1.0f, 0.0f);
                    Vec3 rightDir = cross(lookDir, worldUp).normalize();
                    Vec3 fwdDir   = cross(rightDir, worldUp).normalize(); // keep it planar

                    Vec3 velocity = (rightDir * delta.x - fwdDir * delta.y) * kVelocityPixelScale;

                    auto* planetComp = m_velocityTarget->getComponent<PlanetComponent>();
                    if (planetComp) planetComp->setVelocity(velocity);

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
        snprintf(buf, sizeof(buf), "%.1f u/s", speed);
        dl->AddText(ImVec2(tip.x + 8.0f, tip.y - 14.0f), IM_COL32(255, 255, 255, 200), buf);

        // Small dot at the planet center to make the anchor obvious
        dl->AddCircleFilled(screenOrigin, 5.0f, IM_COL32(255, 220, 50, 220));
    }

    // Shows the current (stored) velocity vector for every placed planet
    // while the Velocity tool is active, so the player can see where each
    // body is currently "aimed" before/while editing it.
    //
    // The arrow is built as a real 3D segment in world space (planet
    // position -> position + velocity * kVelocityArrowWorldScale) and both
    // endpoints are projected with WorldToScreen, so its on-screen direction
    // reflects the true world-space velocity direction no matter how the
    // camera is orbited.
    static constexpr float kVelocityArrowWorldScale = 5.0f;

    void DrawAllVelocityVectors() {
        if (m_toolMode != ToolMode::Velocity) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        for (Entity* planet : planetList) {
            if (!planet) continue;

            // Skip the planet currently being dragged; DrawVelocityArrow handles it.
            if (m_velDragging && planet == m_velocityTarget) continue;

            auto* planetComp = planet->getComponent<PlanetComponent>();
            if (!planetComp) continue;

            Vec3 vel = planetComp->getVelocity();
            float speed = vel.length();
            if (speed < 0.0001f) continue;

            Vec3 worldOrigin = planet->transform.position;
            Vec3 worldTip    = worldOrigin + vel * kVelocityArrowWorldScale;

            ImVec2 screenOrigin, tip;
            if (!WorldToScreen(worldOrigin, screenOrigin)) continue;
            if (!WorldToScreen(worldTip, tip)) continue;

            Vec2 delta(tip.x - screenOrigin.x, tip.y - screenOrigin.y);
            float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);

            const ImU32 col = IM_COL32(120, 200, 255, 200);

            dl->AddLine(screenOrigin, tip, col, 2.0f);

            if (length > 8.0f) {
                Vec2 dir (delta.x / length, delta.y / length);
                Vec2 perp(-dir.y, dir.x);
                float hs = 7.0f;
                ImVec2 p1(tip.x - dir.x * hs + perp.x * hs * 0.5f,
                          tip.y - dir.y * hs + perp.y * hs * 0.5f);
                ImVec2 p2(tip.x - dir.x * hs - perp.x * hs * 0.5f,
                          tip.y - dir.y * hs - perp.y * hs * 0.5f);
                dl->AddTriangleFilled(tip, p1, p2, col);
            }

            char buf[64];
            snprintf(buf, sizeof(buf), "%.1f u/s", speed);
            dl->AddText(ImVec2(tip.x + 6.0f, tip.y - 14.0f), IM_COL32(255, 255, 255, 180), buf);

            dl->AddCircleFilled(screenOrigin, 4.0f, col);
        }
    }

    // highlight selected entity
    void DrawSelectionHighlight() {
        if (m_toolMode != ToolMode::Selection || !selectedEntity) return;

        ImVec2 screenPos;
        if (!WorldToScreen(selectedEntity->transform.position, screenPos)) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        // Filled dot
        dl->AddCircleFilled(screenPos, 5.0f, IM_COL32(100, 200, 255, 230));
        // Thin outer ring so it reads on any background
        dl->AddCircle(screenPos, 7.0f, IM_COL32(100, 200, 255, 120), 16, 1.0f);
    }

    void UpdateFocusTransition(float dt) {
        if (!m_focusTransitioning) return;

        m_focusProgress += dt / m_focusDuration;
        if (m_focusProgress >= 1.0f) {
            m_focusProgress      = 1.0f;
            m_focusTransitioning = false;
        }

        // Smooth-step easing
        float t = m_focusProgress * m_focusProgress * (3.0f - 2.0f * m_focusProgress);
        m_target->transform.position = Vec3::lerp(m_focusStartPos, m_focusEndPos, t);
    }

    // -----------------------------------------------------------------------
    void Update() override {
        float dt = m_game.GetEngine().getDeltaTime();

        if (drawGrid && movingEnabled) {
            DrawGrid(m_game.GetEngine().getRenderer(), m_camera);
        }

        if (!physicsPaused) {
            if (simulationRunning) {
                ApplyGravity(dt * m_game.timeScale);
            }

            for (Entity* planet : planetList) {
                if (planet) {
                    auto* planetComp = planet->getComponent<PlanetComponent>();
                    if (planetComp) {
                        planetComp->update(dt * m_game.timeScale);

                        // Velocity is only an initial condition until the
                        // simulation is actually running; setting it via
                        // the Velocity tool shouldn't move the planet.
                        if (simulationRunning) {
                            planet->transform.position +=
                                planetComp->getVelocity() * (dt * m_game.timeScale);
                        }
                    }
                }
            }
        }

        if (isTransitioning) Transition(dt);
        if (movingEnabled)   UpdateFocusTransition(dt);

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

            moveSpeed = 1000.0f * m_orbitCamera->GetRadius() * dt;

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

            if (!simulationRunning) HandlePlacementLogic();

            // shortcuts
            if (m_input.isKeyPressed(KEY_G)) drawGrid = !drawGrid;
            if (m_input.isKeyDown(KEY_LSHIFT)) m_target->transform.position.y += 1.0f;
            if (m_input.isKeyDown(KEY_LCTRL))  m_target->transform.position.y -= 1.0f;

            // Tool shortcuts
            if (!simulationRunning) {
                if (m_input.isKeyPressed(KEY_1)) m_toolMode = ToolMode::Selection;
                if (m_input.isKeyPressed(KEY_2)) m_toolMode = ToolMode::Reallocation;
                if (m_input.isKeyPressed(KEY_3)) m_toolMode = ToolMode::Velocity;
            }

            if (m_input.isKeyPressed(KEY_P)) {
                if (!simulationRunning) StartSimulation();
                else                    ResumeSimulation();
            }
            if (m_input.isKeyPressed(KEY_SPACE)) PauseSimulation();
            if (m_input.isKeyPressed(KEY_R))     ResetSimulation();
        }

        m_prevLeftPressed = m_input.isMouseButtonPressed(MOUSE_LEFT);
        m_prevRightPressed = m_input.isMouseButtonPressed(MOUSE_RIGHT);

        DrawMainMenuBar();
    }

    void LateUpdate() override {
        if (!isTransitioning) {
            DrawSelectionHighlight();
            DrawAllVelocityVectors();
            DrawVelocityArrow();
        }
        DrawUI();

        if (showPlanetsWindow) DrawPlanetsWindow();
    }

    // Transition
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

        // Bottom panel
        ImGuiID sandboxID  = ImGui::GetID("Sandbox");
        bool isCollapsed = ImGui::GetStateStorage()->GetBool(sandboxID, false);
        float panelHeight = isCollapsed ? ImGui::GetFrameHeight() : 160.0f;

        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.5f, (float)windowSize.y),
                                ImGuiCond_Always, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(500, panelHeight), ImGuiCond_Always);

        int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoResize;

        ImGui::Begin("Sandbox", nullptr, flags);

        if (!ImGui::IsWindowCollapsed()) {
            if (ImGui::BeginTabBar("SandboxTabBar")) {

                if (ImGui::BeginTabItem("Tools")) {
                    float childH = ImGui::GetContentRegionAvail().y;
                    ImGui::BeginChild("ToolScrollList", ImVec2(0, childH),
                                      ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

                    if (simulationRunning) ImGui::BeginDisabled();

                    auto toolButton = [&](const char* label, ToolMode mode) {
                        bool active = (m_toolMode == mode);
                        if (active) ImGui::PushStyleColor(ImGuiCol_Button,
                                        ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        if (ImGui::Button(label, ImVec2(0, ImGui::GetContentRegionAvail().y)))
                            m_toolMode = mode;
                        if (active) ImGui::PopStyleColor();
                        ImGui::SameLine();
                    };

                    toolButton("Selection [1]",    ToolMode::Selection);
                    toolButton("Reallocation [2]", ToolMode::Reallocation);
                    toolButton("Velocity [3]",     ToolMode::Velocity);

                    if (simulationRunning) ImGui::EndDisabled();

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Planets")) {
                    float childH = ImGui::GetContentRegionAvail().y;
                    ImGui::BeginChild("EntityScrollList", ImVec2(0, childH),
                                      ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

                    if (simulationRunning) ImGui::BeginDisabled();

                    for (int i = 0; i < (int)gravityBodies.size(); i++) {
                        if (ImGui::Button(gravityBodies[i].name.c_str(),
                                          ImVec2(0, ImGui::GetContentRegionAvail().y)))
                            StartPlacement(i);
                        ImGui::SameLine();
                    }

                    if (simulationRunning) ImGui::EndDisabled();

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Properties")) {
                    if (selectedEntity) {
                        const GravityBody* bd = FindBodyDef(selectedEntity->name);

                        char nameBuf[64] = {};
                        strncpy(nameBuf, selectedEntity->name.c_str(), sizeof(nameBuf) - 1);

                        float totalWidth = ImGui::GetContentRegionAvail().x;
                        float spacing    = ImGui::GetStyle().ItemSpacing.x;

                        ImGui::SetNextItemWidth((totalWidth - spacing) * 0.75f);
                        ImGui::InputText("##NameField", nameBuf, IM_ARRAYSIZE(nameBuf),
                                         ImGuiInputTextFlags_ReadOnly);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth((totalWidth - spacing) * 0.25f);
                        if (ImGui::ColorEdit3("##ColorField", m_selectedColor,
                                              ImGuiColorEditFlags_NoInputs)) {
                            auto* rc = selectedEntity->getComponent<RenderComponent>();
                            if (rc) rc->setBaseColor(
                                Vec3(m_selectedColor[0], m_selectedColor[1], m_selectedColor[2]));
                        }

                        float halfWidth = (totalWidth - spacing) * 0.5f;
                        float mass   = bd ? bd->mass   : 0.0f;
                        float radius = bd ? bd->radius : 0.0f;

                        ImGui::SetNextItemWidth(halfWidth);
                        ImGui::InputFloat("##MassField", &mass, 0.0f, 0.0f,
                                          "Mass: %.3e", ImGuiInputTextFlags_ReadOnly);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(halfWidth);
                        ImGui::InputFloat("##RadiusField", &radius, 0.0f, 0.0f,
                                          "Radius: %.1f", ImGuiInputTextFlags_ReadOnly);

                        auto* pc = selectedEntity->getComponent<PlanetComponent>();
                        if (pc) {
                            Vec3 vel = pc->getVelocity();
                            float speed = vel.length();
                            ImGui::Text("Velocity: (%.1f, %.1f, %.1f)  |  %.1f u/s",
                                        vel.x, vel.y, vel.z, speed);
                        }

                        Vec3 pos = selectedEntity->transform.position;
                        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                    } else {
                        ImGui::TextDisabled("No planet selected.");
                        ImGui::TextDisabled("Use the Selection tool and click a planet.");
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings")) {
                    if (ImGui::Button("Grid [G]")) drawGrid = !drawGrid;
                    ImGui::SliderFloat("##TimeScale", &m_game.timeScale,
                                       1.0f, 10000.0f, "Time Scale: %.2f",
                                       ImGuiSliderFlags_Logarithmic);
                    ImGui::SliderFloat("##MassScale", &m_massScale,
                                       0.01f, 100.0f, "Mass Scale: %.3fx",
                                       ImGuiSliderFlags_Logarithmic);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Simulation")) {
                    bool isPlaying = simulationRunning && !physicsPaused;

                    if (isPlaying) ImGui::BeginDisabled();
                    if (ImGui::Button("Play [P]")) {
                        if (!simulationRunning) StartSimulation();
                        else                    ResumeSimulation();
                    }
                    if (isPlaying) ImGui::EndDisabled();

                    ImGui::SameLine();

                    if (!simulationRunning) ImGui::BeginDisabled();
                    if (ImGui::Button("Pause [Space]")) PauseSimulation();
                    if (!simulationRunning) ImGui::EndDisabled();

                    ImGui::SameLine();

                    if (!m_hasSnapshot) ImGui::BeginDisabled();
                    if (ImGui::Button("Reset [R]")) ResetSimulation();
                    if (!m_hasSnapshot) ImGui::EndDisabled();

                    ImGui::Spacing();
                    if (simulationRunning) {
                        ImGui::TextDisabled(physicsPaused ? "Status: Paused" : "Status: Running");
                    } else {
                        ImGui::TextDisabled("Status: Stopped");
                    }
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
                if (ImGui::MenuItem("Quit"))      m_game.GetEngine().stop();
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Sandbox"))  {}
                if (ImGui::MenuItem("Load Sandbox")) {}
                if (ImGui::MenuItem("Save Sandbox")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("Grid", "G")) drawGrid = !drawGrid;
                ImGui::EndMenu();
            }
            if (simulationRunning) ImGui::BeginDisabled();
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Selection",    "1")) m_toolMode = ToolMode::Selection;
                if (ImGui::MenuItem("Reallocation", "2")) m_toolMode = ToolMode::Reallocation;
                if (ImGui::MenuItem("Velocity",     "3")) m_toolMode = ToolMode::Velocity;
                ImGui::EndMenu();
            }
            if (simulationRunning) ImGui::EndDisabled();
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

            const char* toolName;
            if (simulationRunning) {
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
        if (ImGui::Begin("Planets", &showPlanetsWindow)) {
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