#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"
#include "components/PlanetComponent.h"

#include "ui/selector.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/render/render.h"
#include "engine/render/texture.h"

#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <fstream>

class Game;

enum class PlanetToolMode {
    None,
    Terrain,
    Paint,
    ColorHistory
};

class PlanetBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();
    AudioManager& m_audio = m_game.GetAudioManager();

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
    float terrainBrushStrength  = 150.0f;  
    float terrainBrushFalloff   = 1.5f; 

    std::vector<std::array<float, 3>> colorHistory;

    std::shared_ptr<Texture> m_paintTexture;

    static constexpr int   k_textureSize  = 512;
    static constexpr float k_brushUVScale = 1.0f / 1000.0f;

    static constexpr float k_uOffset = 0.75f;
    static constexpr bool  k_swapXZ  = true;

    bool m_isHoveringPlanet = false;
    Vec3 m_hoverWorldPos    = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 m_hoverLocalPos    = Vec3(0.0f, 0.0f, 0.0f);

    bool m_leftButtonWasDown = false;
    bool m_leftButtonClicked = false;

    bool m_rightButtonWasDown   = false;
    bool m_rightClickIsOrbiting = false;

    AudioManager::SoundHandle m_brushSoundHandle = 0;
    bool m_isActivelyEditing = false;
    bool m_brushSoundEnabled = false;
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
        StopBrushSound();
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

        const bool leftButtonDown = m_input.isMouseButtonPressed(MOUSE_LEFT);
        m_leftButtonClicked = leftButtonDown && !m_leftButtonWasDown;
        m_leftButtonWasDown = leftButtonDown;

        if (rightButtonDown && m_rightClickIsOrbiting) {
            m_input.setCursorMode(true);
            m_orbitCamera->Update(&m_input, m_game.GetEngine().getDeltaTime());
        } else {
            m_input.setCursorMode(false);
        }
        m_orbitCamera->ApplyScroll(&m_input);

        if (m_input.isKeyPressed(KEY_1)) { currentTool = PlanetToolMode::Paint;   syncToolTab = true; }
        if (m_input.isKeyPressed(KEY_2)) { currentTool = PlanetToolMode::ColorHistory; syncToolTab = true; }
        if (m_input.isKeyPressed(KEY_3)) { currentTool = PlanetToolMode::Terrain; syncToolTab = true; }

        UpdateMouseIntersection();
        m_isActivelyEditing = false;
        TryPaint();
        TryTerrain();
        if (!m_isActivelyEditing) StopBrushSound();
    }

    void LateUpdate() override {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if (ImGui::BeginMainMenuBar()) {
            if (m_ui.beginMenu("Game")) {
                if (m_ui.menuItem("Main Menu")) { m_ui.loadMainMenu(); }
                if (m_ui.menuItem("Settings"))  { m_game.showSettings = !m_game.showSettings; }
                if (m_ui.menuItem("Quit"))      { m_game.GetEngine().stop(); }
                ImGui::EndMenu();
            }
            if (m_ui.beginMenu("Planet")) {
                if (m_ui.menuItem("New Planet")) { 
                    m_game.SetGameMode(std::make_unique<PlanetBuilderMode>(m_game), true);
                }
                if (m_ui.menuItem("Save Planet")) { Save(); }
                if (m_ui.menuItem("Load Planet")) { m_selector.toggleOpen(); }
                ImGui::EndMenu();
            }
            if (m_ui.beginMenu("View")) {
                if (m_ui.menuItem("Properties")) { showProperties = !showProperties; }
                if (m_ui.menuItem("Tools"))      { showTools      = !showTools; }
                ImGui::EndMenu();
            }
            if (m_ui.beginMenu("Tools")) {
                if (m_ui.menuItem("Paint",   "1")) { currentTool = PlanetToolMode::Paint;   syncToolTab = true; }
                if (m_ui.menuItem("Color History", "2")) { currentTool = PlanetToolMode::ColorHistory; syncToolTab = true; }
                if (m_ui.menuItem("Terrain", "3")) { currentTool = PlanetToolMode::Terrain; syncToolTab = true; }
                ImGui::EndMenu();
            }

            const char* toolName = (currentTool == PlanetToolMode::Terrain) ? "Terrain"
                                 : (currentTool == PlanetToolMode::Paint)   ? "Paint"
                                 : (currentTool == PlanetToolMode::ColorHistory) ? "Color History"
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
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

        ImGui::PushItemWidth(225.0f);

        if (planet) {
            ImGui::SeparatorText("General");
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

                ImGui::SeparatorText("Sun");
                float sunIntensity = planetComponent->getPlanetParams().sunIntensity;
                if (ImGui::SliderFloat("Sun Intensity", &sunIntensity, 0.0f, 50.0f)) {
                    planetComponent->getPlanetParams().sunIntensity = sunIntensity;
                }

                Vec3 sunDir = planetComponent->getPlanetParams().sunDir;
                if (ImGui::SliderFloat3("Sun Direction", &sunDir.x, -1.0f, 1.0f)) {
                    planetComponent->getPlanetParams().sunDir = sunDir.normalize();
                }
            }

            ImGui::SeparatorText("Atmosphere");
            bool hasAtmosphere = planetComponent->hasAtmosphere();
            if (m_ui.checkbox("Has Atmosphere", &hasAtmosphere)) {
                planetComponent->setHasAtmosphere(hasAtmosphere);
            }
            if (hasAtmosphere) {
                AtmosphereParams& atmo = planetComponent->getAtmosphere();
                if (ImGui::SliderFloat("Thickness", &atmo.thickness, 1.0f, 1000.0f)) {}

                Vec3& rc = atmo.rayleighCoeff;
                static constexpr float k_rayleighMax = 0.08f; // displayable ceiling
                float skyColor[3] = {
                    std::fmin(rc.x / k_rayleighMax, 1.0f),
                    std::fmin(rc.y / k_rayleighMax, 1.0f),
                    std::fmin(rc.z / k_rayleighMax, 1.0f)
                };
                if (ImGui::ColorEdit3("Hue##rayleigh", skyColor,
                        ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueWheel)) {
                    float maxC = std::fmax(skyColor[0], std::fmax(skyColor[1], skyColor[2]));
                    if (maxC < 1e-5f) maxC = 1.0f;
                    rc.x = (skyColor[0] / maxC) * k_rayleighMax;
                    rc.y = (skyColor[1] / maxC) * k_rayleighMax;
                    rc.z = (skyColor[2] / maxC) * k_rayleighMax;
                }

                float maxCoeff = std::fmax(rc.x, std::fmax(rc.y, rc.z));
                float scatterStrength = maxCoeff / k_rayleighMax; // [0,1]
                if (ImGui::SliderFloat("Scatter Density", &scatterStrength, 0.01f, 1.0f)) {
                    float prevMax = std::fmax(rc.x, std::fmax(rc.y, rc.z));
                    if (prevMax > 1e-6f) {
                        float scale = (scatterStrength * k_rayleighMax) / prevMax;
                        rc.x *= scale;
                        rc.y *= scale;
                        rc.z *= scale;
                    }
                }

                if (ImGui::SliderFloat("Horizon Sharpness", &atmo.edgeFalloff, 0.0f, 1200.0f,
                        "%.2f", ImGuiSliderFlags_Logarithmic)) {}
            }

            ImGui::SeparatorText("Water");
            bool hasWater = planetComponent->hasWater();
            if (m_ui.checkbox("Has Water", &hasWater)) {
                planetComponent->setHasWater(hasWater);
            }
            if (hasWater) {
                float waterLevel = planetComponent->getWater().level;
                if (ImGui::SliderFloat("Water Level", &waterLevel, -100.0f, 1000.0f)) {
                    planetComponent->getWater().level = waterLevel;
                }

                Vec3 waterColorA = planetComponent->getWater().colorA;
                if (ImGui::ColorEdit3("Water Color A", &waterColorA.x)) {
                    planetComponent->getWater().colorA = waterColorA;
                }

                Vec3 waterColorB = planetComponent->getWater().colorB;
                if (ImGui::ColorEdit3("Water Color B", &waterColorB.x)) {
                    planetComponent->getWater().colorB = waterColorB;
                }

                float depthMultiplier = planetComponent->getWater().depthMultiplier;
                if (ImGui::SliderFloat("Depth Multiplier", &depthMultiplier, 0.0f, 0.5f)) {
                    planetComponent->getWater().depthMultiplier = depthMultiplier;
                }

                float alphaMultiplier = planetComponent->getWater().alphaMultiplier;
                if (ImGui::SliderFloat("Alpha Multiplier", &alphaMultiplier, 0.0f, 0.5f)) {
                    planetComponent->getWater().alphaMultiplier = alphaMultiplier;
                }
            }
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

    void DrawToolsWindow() {
        ImGui::SetNextWindowSize(ImVec2(375, 0), ImGuiCond_Once);
        ImGui::SetNextWindowPos(
            ImVec2(m_game.GetEngine().getWindow().getSize().x - 500, 80), ImGuiCond_Once);
        ImGui::Begin("Tool Properties", &showTools, 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

        ImGui::PushItemWidth(250.0f);

        bool consumeSync = syncToolTab;
        syncToolTab = false;

        if (ImGui::BeginTabBar("Tools")) {
            ImGuiTabItemFlags paintFlags = (consumeSync && currentTool == PlanetToolMode::Paint)
                                         ? ImGuiTabItemFlags_SetSelected : 0;
            if (m_ui.beginTabItem("Paint", nullptr, paintFlags)) {
                if (!consumeSync) currentTool = PlanetToolMode::Paint;

                ImGui::SliderFloat("Brush Size",    &brushSize,    1.0f,  150.0f);
                ImGui::ColorEdit3 ("Brush Color",    brushColor);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    colorHistory.push_back({ brushColor[0], brushColor[1], brushColor[2] });
                }
                ImGui::SliderFloat("Brush Opacity", &brushOpacity, 0.0f,  1.0f);

                ImGui::Spacing();
                if (m_ui.button("Clear Texture")) {
                    if (m_paintTexture) {
                        m_paintTexture->fill(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                }

                ImGui::SameLine();
                if (m_ui.checkbox("Brush Sound", &m_brushSoundEnabled)) {}

                ImGui::EndTabItem();
            }

            ImGuiTabItemFlags colorHistoryFlags = (consumeSync && currentTool == PlanetToolMode::ColorHistory)
                                            ? ImGuiTabItemFlags_SetSelected : 0;
            if (m_ui.beginTabItem("Color History", nullptr, colorHistoryFlags)) {
                if (!consumeSync) currentTool = PlanetToolMode::ColorHistory;

                ImGui::ColorEdit3("Brush Color", brushColor);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    colorHistory.push_back({ brushColor[0], brushColor[1], brushColor[2] });
                }
                
                ImGui::SeparatorText("History");
                for (size_t i = 0; i < colorHistory.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::ColorButton("##ColorHistory",
                            ImVec4(colorHistory[i][0], colorHistory[i][1], colorHistory[i][2], 1.0f),
                            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                            ImVec2(30, 30))) {
                        std::memcpy(brushColor, colorHistory[i].data(), sizeof(float) * 3);
                        currentTool = PlanetToolMode::Paint;
                        syncToolTab = true;
                    }
                    ImGui::PopID();
                    if ((i + 1) % 8 != 0) {
                        ImGui::SameLine();
                    }
                }
                ImGui::EndTabItem();
            }

            ImGuiTabItemFlags terrainFlags = (consumeSync && currentTool == PlanetToolMode::Terrain)
                                            ? ImGuiTabItemFlags_SetSelected : 0;
            if (m_ui.beginTabItem("Terrain", nullptr, terrainFlags)) {
                if (!consumeSync) currentTool = PlanetToolMode::Terrain;

                ImGui::SliderFloat("Brush Size",     &terrainBrushSize,     10.0f, 800.0f);
                ImGui::SliderFloat("Strength",       &terrainBrushStrength,  0.1f,  400.0f);
                ImGui::SliderFloat("Falloff",        &terrainBrushFalloff,   0.5f,   4.0f);

                ImGui::Spacing();
                if (m_ui.button("Reset Mesh")) {
                    ResetTerrain();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

private:
    float toLinear(float c) const {
        auto& r = m_game.GetEngine().getRenderer();
        if (r.isGammaCorrectionEnabled())
            c = std::pow(std::max(0.0f, c), r.getGammaCorrection());
        const float exposure = r.getExposure();
        if (exposure > 0.0f) c /= exposure;
        return std::max(0.0f, c);
    }

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

    void GetHoverUV(float& outU, float& outV) const {
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

        outU = std::fmax(0.0f, std::fmin(1.0f, u));
        outV = std::fmax(0.0f, std::fmin(1.0f, v));
    }

    void StartBrushSound() {
        m_isActivelyEditing = true;
        if (m_brushSoundHandle != 0 && m_audio.isSoundPlaying(m_brushSoundHandle)) return;
        if (m_brushSoundEnabled) m_brushSoundHandle = m_audio.playSound("assets/audio/paint.wav", SFX, true, -0.35f);
    }

    void StopBrushSound() {
        if (m_brushSoundHandle == 0) return;
        m_audio.stopSound(m_brushSoundHandle, 0.1f);
        m_brushSoundHandle = 0;
    }

    void TryPaint() {
        if (currentTool != PlanetToolMode::Paint) return;
        if (!m_input.isMouseButtonPressed(MOUSE_LEFT)) return;
        if (!m_isHoveringPlanet) return;
        if (!m_paintTexture) return;

        StartBrushSound();

        float u, v;
        GetHoverUV(u, v);

        const float uvRadius = brushSize * k_brushUVScale;
        m_paintTexture->paint(
            u, v, uvRadius,
            Vec4(toLinear(brushColor[0]), toLinear(brushColor[1]), toLinear(brushColor[2]), brushOpacity)
        );
    }

    void DrawBrushIndicator() {
        const bool isPaint   = currentTool == PlanetToolMode::Paint;
        const bool isTerrain = currentTool == PlanetToolMode::Terrain;
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (!isPaint && !isTerrain) return;
        if ((isPaint || isTerrain) && !m_isHoveringPlanet) return;

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
        j["hasWater"] = planet->getComponent<PlanetComponent>()->hasWater();
        j["waterLevel"] = planet->getComponent<PlanetComponent>()->getWater().level;
        j["waterColorA"] = { planet->getComponent<PlanetComponent>()->getWater().colorA.x,
                            planet->getComponent<PlanetComponent>()->getWater().colorA.y,
                            planet->getComponent<PlanetComponent>()->getWater().colorA.z };
        j["waterColorB"] = { planet->getComponent<PlanetComponent>()->getWater().colorB.x,
                            planet->getComponent<PlanetComponent>()->getWater().colorB.y,
                            planet->getComponent<PlanetComponent>()->getWater().colorB.z };
        j["depthMultiplier"] = planet->getComponent<PlanetComponent>()->getWater().depthMultiplier;
        j["alphaMultiplier"] = planet->getComponent<PlanetComponent>()->getWater().alphaMultiplier;

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