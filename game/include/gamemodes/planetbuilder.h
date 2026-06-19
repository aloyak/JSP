#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"
#include "components/PlanetComponent.h"

#include "selector.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/render/render.h"
#include "engine/render/texture.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <fstream>

class Game;

enum class PlanetToolMode {
    None,
    Terrain,
    Paint
};

class PlanetBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Selector m_selector = Selector(m_game, "user/planets", ".planet");

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;

    Entity* planet = nullptr;

    bool showProperties = true;
    bool showTools = true;

    PlanetToolMode currentTool = PlanetToolMode::None;
    bool syncToolTab = false;

    // Paint tool
    float brushSize     = 10.0f;
    float brushColor[3] = { 1.0f, 0.0f, 0.0f };
    float brushOpacity  = 1.0f;

    // Terrain tool
    float terrainBrushSize      = 150.0f; 
    float terrainBrushStrength  = 15.0f;  
    float terrainBrushFalloff   = 1.5f; 

    std::shared_ptr<Texture> m_paintTexture;

    static constexpr int   k_textureSize  = 512;
    static constexpr float k_brushUVScale = 1.0f / 1000.0f;

    static constexpr float k_uOffset = 0.75f;
    static constexpr bool  k_swapXZ  = true;

    bool m_isHoveringPlanet = false;
    Vec3 m_hoverWorldPos    = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 m_hoverLocalPos    = Vec3(0.0f, 0.0f, 0.0f);

    bool m_rightButtonWasDown   = false;
    bool m_rightClickIsOrbiting = false;

public:
    PlanetBuilderMode(Game& game)
        : GameMode("assets/scenes/space.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_game.timeScale = 1.0f;

        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
        m_camera->addComponent<CameraComponent>(45.0f, 1600.0f / 900.0f, 0.1f, 30000.0f);
        m_game.GetEngine().setActiveCamera(m_camera);

        float distance = 2400.0f;
        float offset = distance / std::sqrt(3.0f);

        m_camera->transform.position = Vec3(offset, offset, offset);
        m_camera->transform.rotation = Vec3(-35.264f, 45.0f, 0.0f);

        planet = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Planet");

        auto* renderComponent = planet->addComponent<RenderComponent>(
            "assets/models/planetbase.fbx",
            "assets/shaders/builtin/vert.glsl",
            "assets/shaders/builtin/frag.glsl",
            true /*dynamic texture*/);
        auto* planetComponent = planet->addComponent<PlanetComponent>(m_game, 1.0f, 637.1);
        planetComponent->initialize();

        m_paintTexture.reset(Texture::createPaintable("assets/textures/checked.png"));

        renderComponent->setPaintableTexture(m_paintTexture);

        m_orbitCamera = new OrbitCamera(m_camera);
        m_orbitCamera->SetTarget(planet, distance);
        m_orbitCamera->SetMaxRadius(3700.0f);
        m_orbitCamera->SyncFromCurrentPosition();
        m_orbitCamera->ApplyPosition();
    }

    void OnExit() override {
        m_paintTexture.reset();
        delete m_orbitCamera;
    }

    void Update() override {
        const bool rightButtonDown = m_input.isMouseButtonPressed(MOUSE_RIGHT);

        if (rightButtonDown && !m_rightButtonWasDown) {
            const bool clickStartedOnTerrainTarget =
                currentTool == PlanetToolMode::Terrain && m_isHoveringPlanet;
            m_rightClickIsOrbiting = !clickStartedOnTerrainTarget;
        }
        m_rightButtonWasDown = rightButtonDown;

        if (rightButtonDown && m_rightClickIsOrbiting) {
            m_input.setCursorMode(true);
            m_orbitCamera->Update(&m_input, m_game.GetEngine().getDeltaTime());
        } else {
            m_input.setCursorMode(false);
        }
        m_orbitCamera->ApplyScroll(&m_input);

        // shortcuts
        if (m_input.isKeyPressed(KEY_1)) { currentTool = PlanetToolMode::Paint;   syncToolTab = true; }
        if (m_input.isKeyPressed(KEY_2)) { currentTool = PlanetToolMode::Terrain; syncToolTab = true; }

        UpdateMouseIntersection();
        TryPaint();
        TryTerrain();
    }

    void LateUpdate() override {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Game")) {
                if (ImGui::MenuItem("Main Menu")) { m_ui.loadMainMenu(); }
                if (ImGui::MenuItem("Quit"))      { m_game.GetEngine().stop(); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Planet")) {
                if (ImGui::MenuItem("New Planet")) { 
                    m_game.SetGameMode(std::make_unique<PlanetBuilderMode>(m_game), true);
                }
                if (ImGui::MenuItem("Save Planet")) { Save(); }
                if (ImGui::MenuItem("Load Planet")) { m_selector.toggleOpen(); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Properties")) { showProperties = !showProperties; }
                if (ImGui::MenuItem("Tools"))      { showTools      = !showTools; }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Paint",   "1")) { currentTool = PlanetToolMode::Paint;   syncToolTab = true; }
                if (ImGui::MenuItem("Terrain", "2")) { currentTool = PlanetToolMode::Terrain; syncToolTab = true; }
                ImGui::EndMenu();
            }

            const char* toolName = (currentTool == PlanetToolMode::Terrain) ? "Terrain"
                                 : (currentTool == PlanetToolMode::Paint)   ? "Paint"
                                                                             : "None";
            float textWidth = ImGui::CalcTextSize(toolName).x + 8.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - textWidth);
            ImGui::TextDisabled("%s", toolName);
        }
        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor(2);

        if (showProperties) DrawPropertiesWindow();
        if (showTools)      DrawToolsWindow();
        if (m_selector.isOpen()) {
            m_selector.Draw();
        }

        std::string selectedFile = m_selector.ConsumeSelection();
        if (!selectedFile.empty()) {
            Load(selectedFile);
        }

        DrawBrushIndicator();
    }

    void DrawPropertiesWindow() {
        ImGui::SetNextWindowPos(ImVec2(60, 80), ImGuiCond_Once);
        ImGui::Begin("Planet Properties", &showProperties,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::PushItemWidth(225.0f);

        if (planet) {
            char nameBuffer[128];
            std::strncpy(nameBuffer, planet->name.c_str(), sizeof(nameBuffer));
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                planet->name = std::string(nameBuffer);
            }

            auto* planetComponent = planet->getComponent<PlanetComponent>();
            if (planetComponent) {
                float radius = planetComponent->getPlanetParams().radius;
                if (ImGui::SliderFloat("Radius", &radius, 50.0f, 1500.0f)) {
                    planetComponent->getPlanetParams().radius = radius;
                    planetComponent->initialize();
                }

                float mass = planetComponent->getPlanetParams().mass;
                if (ImGui::SliderFloat("Mass", &mass, 1.0f, 10000.0f)) {
                    planetComponent->getPlanetParams().mass = mass;
                }

                float periodHours = planetComponent->getPlanetParams().period;
                float periodMinutes = periodHours * 60.0f;
                
                if (ImGui::SliderFloat("Rotation Period (min)", &periodMinutes, 0.08333f, 40.0f)) {
                    planetComponent->getPlanetParams().period = periodMinutes / 60.0f;
                }

                ImGui::Separator();
                float sunIntensity = planetComponent->getPlanetParams().sunIntensity;
                if (ImGui::SliderFloat("Sun Intensity", &sunIntensity, 0.0f, 50.0f)) {
                    planetComponent->getPlanetParams().sunIntensity = sunIntensity;
                }

                Vec3 sunDir = planetComponent->getPlanetParams().sunDir;
                if (ImGui::SliderFloat3("Sun Direction", &sunDir.x, -1.0f, 1.0f)) {
                    planetComponent->getPlanetParams().sunDir = sunDir.normalize();
                }
            }

            ImGui::Separator();
            bool hasAtmosphere = planetComponent->hasAtmosphere();
            if (ImGui::Checkbox("Has Atmosphere", &hasAtmosphere)) {
                planetComponent->setHasAtmosphere(hasAtmosphere);                
            }
            if (hasAtmosphere) {
                float atmosphereThickness = planetComponent->getAtmosphere().thickness;
                if (ImGui::SliderFloat("Atmosphere Thickness", &atmosphereThickness, 1.0f, 1000.0f)) {
                    planetComponent->getAtmosphere().thickness = atmosphereThickness;
                }

                Vec3 rayleighCoeff = planetComponent->getAtmosphere().rayleighCoeff;
                if (ImGui::ColorEdit3("Rayleigh Coeff", &rayleighCoeff.x)) {
                    planetComponent->getAtmosphere().rayleighCoeff = rayleighCoeff;
                }

                float edgeFalloff = planetComponent->getAtmosphere().edgeFalloff;
                if (ImGui::SliderFloat("Edge Falloff", &edgeFalloff, 0.0f, 1200.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
                    planetComponent->getAtmosphere().edgeFalloff = edgeFalloff;
                }
            }

            ImGui::Separator();
            bool hasRings = planetComponent->hasRings();
            if (ImGui::Checkbox("Has Rings", &hasRings)) {
                planetComponent->setHasRings(hasRings);
            }
            if (hasRings) {
                float innerRadius = 10; //placeholder
                if (ImGui::SliderFloat("Ring Inner Radius", &innerRadius, 1.0f, 5000.0f)) {
                }

                float outerRadius = 100; // placeholder
                if (ImGui::SliderFloat("Ring Outer Radius", &outerRadius, 1.0f, 5000.0f)) {
                }

                float ringInclination = 0; // placeholder
                if (ImGui::SliderFloat("Ring Inclination", &ringInclination, -90.0f, 90.0f)) {
                }
            }

            ImGui::Separator();
            bool hasWater = planetComponent->hasWater();
            if (ImGui::Checkbox("Has Water", &hasWater)) {
                planetComponent->setHasWater(hasWater);
            }
            if (hasWater) {
                float waterLevel = planetComponent->getWater().level;
                if (ImGui::SliderFloat("Water Level", &waterLevel, 0.0f, 1000.0f)) {
                    planetComponent->getWater().level = waterLevel;
                }

                // more shader params
            }
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

    void DrawToolsWindow() {
        ImGui::SetNextWindowPos(
            ImVec2(m_game.GetEngine().getWindow().getSize().x - 500, 80), ImGuiCond_Once);
        ImGui::Begin("Tool Properties", &showTools,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::PushItemWidth(250.0f);

        if (ImGui::BeginTabBar("Tools")) {
            ImGuiTabItemFlags paintFlags = (syncToolTab && currentTool == PlanetToolMode::Paint)
                                         ? ImGuiTabItemFlags_SetSelected : 0;
            if (ImGui::BeginTabItem("Paint", nullptr, paintFlags)) {
                if (!syncToolTab) currentTool = PlanetToolMode::Paint;

                ImGui::SliderFloat("Brush Size",    &brushSize,    1.0f,  150.0f);
                ImGui::ColorEdit3 ("Brush Color",    brushColor);
                ImGui::SliderFloat("Brush Opacity", &brushOpacity, 0.0f,  1.0f);

                ImGui::Spacing();
                if (ImGui::Button("Clear Texture")) {
                    if (m_paintTexture) {
                        m_paintTexture->fill(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                }

                ImGui::EndTabItem();
            }

            ImGuiTabItemFlags terrainFlags = (syncToolTab && currentTool == PlanetToolMode::Terrain)
                                            ? ImGuiTabItemFlags_SetSelected : 0;
            if (ImGui::BeginTabItem("Terrain", nullptr, terrainFlags)) {
                if (!syncToolTab) currentTool = PlanetToolMode::Terrain;

                ImGui::SliderFloat("Brush Size",     &terrainBrushSize,     10.0f, 800.0f);
                ImGui::SliderFloat("Strength",       &terrainBrushStrength,  0.1f,  400.0f);
                ImGui::SliderFloat("Falloff",        &terrainBrushFalloff,   0.5f,   4.0f);

                ImGui::Spacing();

                ImGui::Spacing();
                if (ImGui::Button("Reset Mesh")) {
                    ResetTerrain();
                }

                ImGui::EndTabItem();
            }

            syncToolTab = false;
            ImGui::EndTabBar();
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

private:
    void PaintUVGradient() {
        if (!m_paintTexture) return;
        for (int py = 0; py < k_textureSize; ++py) {
            for (int px = 0; px < k_textureSize; ++px) {
                float u = static_cast<float>(px) / static_cast<float>(k_textureSize - 1);
                float v = static_cast<float>(py) / static_cast<float>(k_textureSize - 1);
                m_paintTexture->setPixel(px, py,
                    Vec4(u, v, 0.0f, 1.0f));
            }
        }
        m_paintTexture->uploadToGPU();
    }

    void UpdateMouseIntersection() {
        m_isHoveringPlanet = false;
        if (!planet || !m_camera || ImGui::GetIO().WantCaptureMouse) return;

        auto* camComp = m_camera->getComponent<CameraComponent>();
        if (!camComp) return;

        Vec2 mousePos = m_input.getMousePos();
        Ray ray = m_game.GetEngine().getRenderer().pickRay(
            mousePos.x, mousePos.y, camComp->getCamera(), m_camera->transform);

        const float sphereRadius = planet->transform.scale.x;
        const Vec3  center = planet->transform.position;
        const Vec3  ro = ray.origin - center;
        const Vec3& rd = ray.direction;

        const float b = 2.0f * dot(ro, rd);
        const float c = dot(ro, ro) - (sphereRadius * sphereRadius);
        const float discriminant = b * b - 4.0f * c;
        if (discriminant < 0.0f) return;

        const float sqrtD = std::sqrt(discriminant);
        const float t0 = (-b - sqrtD) * 0.5f;
        const float t1 = (-b + sqrtD) * 0.5f;
        const float t = (t0 > 0.0f) ? t0 : t1;
        if (t <= 0.0f) return;

        m_hoverWorldPos = ray.origin + rd * t;
        m_hoverLocalPos = m_hoverWorldPos - center;
        m_isHoveringPlanet = true;
    }

    void TryPaint() {
        if (currentTool != PlanetToolMode::Paint) return;
        if (!m_input.isMouseButtonPressed(MOUSE_LEFT)) return;
        if (!m_isHoveringPlanet) return;
        if (!m_paintTexture) return;

        const float pi = 3.1415926535f;
        const float radY = planet->transform.rotation.y * (pi / 180.0f);
        const float cosY = std::cos(radY);
        const float sinY = std::sin(radY);
        const float rx = m_hoverLocalPos.x * cosY - m_hoverLocalPos.z * sinY;
        const float rz = m_hoverLocalPos.x * sinY + m_hoverLocalPos.z * cosY;
        const Vec3  n = Vec3(rx, m_hoverLocalPos.y, rz).normalize();

        const float ax = k_swapXZ ? n.z : n.x;
        const float az = k_swapXZ ? n.x : n.z;
        const float signX = -1.0f;

        float u = std::atan2(signX * ax, az) / (2.0f * pi) + 0.5f;
        float v = std::asin(std::fmax(-1.0f, std::fmin(1.0f, n.y))) / pi + 0.5f;

        u += k_uOffset;
        u -= std::floor(u);

        u = std::fmax(0.0f, std::fmin(1.0f, u));
        v = std::fmax(0.0f, std::fmin(1.0f, v));

        const float uvRadius = brushSize * k_brushUVScale;
        m_paintTexture->paint(
            u, v, uvRadius,
            Vec4(brushColor[0], brushColor[1], brushColor[2], brushOpacity)
        );
    }

    void DrawBrushIndicator() {
        const bool isPaint   = currentTool == PlanetToolMode::Paint;
        const bool isTerrain = currentTool == PlanetToolMode::Terrain;
        if (!m_isHoveringPlanet || ImGui::GetIO().WantCaptureMouse) return;
        if (!isPaint && !isTerrain) return;

        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        Vec3 diff = m_hoverWorldPos - m_camera->transform.position;
        float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

        const float pi = 3.1415926535f;
        const float fovRad     = 45.0f * (pi / 180.0f);
        const float screenHeight = ImGui::GetIO().DisplaySize.y;
        const float projScale  = screenHeight / (2.0f * std::tan(fovRad * 0.5f));

        float worldBrushRadius = 0.0f;
        ImU32 color;

        if (isPaint) {
            float sphereRadius = planet->transform.scale.x;
            worldBrushRadius = sphereRadius * (brushSize * k_brushUVScale) * pi;
            color = IM_COL32(static_cast<int>(brushColor[0] * 255.0f),
                             static_cast<int>(brushColor[1] * 255.0f),
                             static_cast<int>(brushColor[2] * 255.0f),
                             200);
        } else {
            worldBrushRadius = terrainBrushSize;
            const bool lowering = m_input.isMouseButtonPressed(MOUSE_RIGHT) && !m_rightClickIsOrbiting;
            color = lowering ? IM_COL32(255, 80, 80, 200) : IM_COL32(200, 200, 255, 200);
        }

        const float screenRadius = (worldBrushRadius / distance) * projScale;
        ImVec2 mousePos = ImGui::GetMousePos();
        ImGui::GetForegroundDrawList()->AddCircle(mousePos, screenRadius, color, 48, 2.0f);
    }

    void TryTerrain() {
        if (currentTool != PlanetToolMode::Terrain) return;
        if (!m_isHoveringPlanet) return;

        const bool raise = m_input.isMouseButtonPressed(MOUSE_LEFT);
        const bool lower = m_input.isMouseButtonPressed(MOUSE_RIGHT) && !m_rightClickIsOrbiting;
        if (!raise && !lower) return;

        auto* renderComp = planet->getComponent<RenderComponent>();
        if (!renderComp) return;
        std::shared_ptr<Model> model = renderComp->getModel();
        if (!model) return;

        const float dt        = m_game.GetEngine().getDeltaTime();
        const float sign      = raise ? 1.0f : -1.0f;

        const Vec3 brushCentre = planet->transform.worldToLocal(m_hoverWorldPos);

        const float planetScale        = planet->transform.scale.x;
        const float localBrushSize     = terrainBrushSize / std::fmax(planetScale, 1e-8f);
        const float localBrushStrength = terrainBrushStrength / std::fmax(planetScale, 1e-8f);
        const float maxDelta = sign * localBrushStrength * dt;
        const float rSq      = localBrushSize * localBrushSize;

        bool anyModified = false;

        for (Mesh& mesh : model->getMeshes()) {
            if (!mesh.isDynamic()) continue;

            for (Vertex& v : mesh.vertices) {
                const float dx = v.Position.x - brushCentre.x;
                const float dy = v.Position.y - brushCentre.y;
                const float dz = v.Position.z - brushCentre.z;
                const float distSq = dx*dx + dy*dy + dz*dz;
                if (distSq >= rSq) continue;

                const float t       = std::sqrt(distSq) / localBrushSize;
                const float tPow    = std::pow(t, terrainBrushFalloff);
                const float falloff = 1.0f - tPow * tPow * (3.0f - 2.0f * tPow);

                const float lenSq = v.Position.x*v.Position.x
                                  + v.Position.y*v.Position.y
                                  + v.Position.z*v.Position.z;
                if (lenSq < 1e-8f) continue;
                const float invLen = 1.0f / std::sqrt(lenSq);
                const Vec3 outward(v.Position.x * invLen,
                                   v.Position.y * invLen,
                                   v.Position.z * invLen);

                const float delta = maxDelta * falloff;
                v.Position.x += outward.x * delta;
                v.Position.y += outward.y * delta;
                v.Position.z += outward.z * delta;
                anyModified = true;
            }

            if (anyModified) {
                mesh.recalculateNormals();
                mesh.updateVertexBuffer();
            }
        }
    }

    void ResetTerrain() {
        auto* renderComp = planet->getComponent<RenderComponent>();
        if (!renderComp) return;
        renderComp->reloadModel();
    }

    void Save() {
        std::string baseDir = Path::resolve("user/planets/");

        if (m_paintTexture) {
            m_paintTexture->save(baseDir + planet->name + ".png");
        }

        nlohmann::json j;
        j["name"] = planet->name;
        j["mass"] = planet->getComponent<PlanetComponent>()->getPlanetParams().mass;
        j["radius"] = planet->getComponent<PlanetComponent>()->getPlanetParams().radius;
        j["period"] = planet->getComponent<PlanetComponent>()->getPlanetParams().period;
        j["texture"] = "/user/planets/" + planet->name + ".png";
        j["hasAtmosphere"] = planet->getComponent<PlanetComponent>()->hasAtmosphere();
        j["atmosphereThickness"] = planet->getComponent<PlanetComponent>()->getAtmosphere().thickness;
        j["sunIntensity"] = planet->getComponent<PlanetComponent>()->getPlanetParams().sunIntensity;
        j["sunDir"] = { planet->getComponent<PlanetComponent>()->getPlanetParams().sunDir.x,
                        planet->getComponent<PlanetComponent>()->getPlanetParams().sunDir.y,
                        planet->getComponent<PlanetComponent>()->getPlanetParams().sunDir.z };
        j["rayleighCoeff"] = { planet->getComponent<PlanetComponent>()->getAtmosphere().rayleighCoeff.x,
                            planet->getComponent<PlanetComponent>()->getAtmosphere().rayleighCoeff.y,
                            planet->getComponent<PlanetComponent>()->getAtmosphere().rayleighCoeff.z };
        j["edgeFalloff"] = planet->getComponent<PlanetComponent>()->getAtmosphere().edgeFalloff;
        j["hasRings"] = planet->getComponent<PlanetComponent>()->hasRings();
        j["hasWater"] = planet->getComponent<PlanetComponent>()->hasWater();

        std::string geomPath = baseDir + planet->name + ".geom";
        std::ofstream geomFile(geomPath, std::ios::binary);
        if (geomFile.is_open()) {
            auto* renderComp = planet->getComponent<RenderComponent>();
            if (renderComp && renderComp->getModel()) {
                for (auto& mesh : renderComp->getModel()->getMeshes()) {
                    for (const auto& v : mesh.vertices) {
                        geomFile.write(reinterpret_cast<const char*>(&v.Position), sizeof(v.Position));
                    }
                }
            }
            geomFile.close();
            j["geometry"] = "/user/planets/" + planet->name + ".geom";
        }

        std::string savePath = baseDir + planet->name + ".planet";
        std::ofstream file(savePath);
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
            Logger::info("Saved planet to " + savePath);
        } else {
            Logger::error("Failed to save planet to " + savePath);
        }
    }

    void Load(const std::string& filepath) {
        auto* planetComponent = planet->getComponent<PlanetComponent>();
        if (planetComponent) {
            planetComponent->loadFromFile(filepath, m_paintTexture);
        }
    }
};