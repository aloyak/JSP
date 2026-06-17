#pragma once

#include "gamemode.h"
#include "grid.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/render/render.h"
#include "engine/render/texture.h"

#include <cmath>
#include <cstring>
#include <memory>

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

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;

    Entity* planet = nullptr;
    float planetMass = 100.0f;
    bool hasAtmosphere = false;
    bool hasRings = false;

    bool showProperties = true;
    bool showTools = true;

    PlanetToolMode currentTool = PlanetToolMode::None;
    bool syncToolTab = false;

    float brushSize     = 10.0f;
    float brushColor[3] = { 1.0f, 0.0f, 0.0f };
    float brushOpacity  = 1.0f;

    std::shared_ptr<Texture> m_paintTexture;

    static constexpr int   k_textureSize  = 512;
    static constexpr float k_brushUVScale = 1.0f / 1000.0f;

    static constexpr float k_uOffset = 0.75f;
    static constexpr bool  k_swapXZ  = true;
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

        auto* renderComponent = planet->addComponent<RenderComponent>("assets/models/planetbase.fbx");
        auto* planetComponent = planet->addComponent<PlanetComponent>(1.0f, 637.1);
        planetComponent->adjustScale();

        m_paintTexture.reset(Texture::createPaintable(Vec2(k_textureSize, k_textureSize),
                                                      Vec4(1.0f, 1.0f, 1.0f, 1.0f)));

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
        if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
            m_input.setCursorMode(true);
            m_orbitCamera->Update(&m_input, m_game.GetEngine().getDeltaTime());
        } else {
            m_input.setCursorMode(false);
        }
        m_orbitCamera->ApplyScroll(&m_input);

        TryPaint();
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
            ImGui::Separator();
            if (ImGui::BeginMenu("Planet")) {
                if (ImGui::MenuItem("New Planet")) {}
                if (ImGui::MenuItem("Save Planet")) { SaveTexture(); } // good for now
                if (ImGui::MenuItem("Load Planet")) {}
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
    }

    void DrawPropertiesWindow() {
        ImGui::SetNextWindowPos(ImVec2(60, 80), ImGuiCond_Once);
        ImGui::Begin("Planet Properties", &showProperties,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

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

            ImGui::Separator();
            ImGui::Checkbox("Has Atmosphere", &hasAtmosphere);

            ImGui::Separator();
            ImGui::Checkbox("Has Rings", &hasRings);
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

                ImGui::SliderFloat("Brush Size",    &brushSize,    1.0f,  100.0f);
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

    void TryPaint() {
        if (currentTool != PlanetToolMode::Paint)      return;
        if (!m_input.isMouseButtonPressed(MOUSE_LEFT)) return;
        if (ImGui::GetIO().WantCaptureMouse)           return;
        if (!planet || !m_camera || !m_paintTexture)   return;

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

        const Vec3  worldHit = ray.origin + rd * t;
        const Vec3  localHit = worldHit - center;

        const float pi = 3.1415926535f;
        const float radY = planet->transform.rotation.y * (pi / 180.0f);
        const float cosY = std::cos(-radY);
        const float sinY = std::sin(-radY);
        const float rx = localHit.x * cosY - localHit.z * sinY;
        const float rz = localHit.x * sinY + localHit.z * cosY;
        const Vec3  n = Vec3(rx, localHit.y, rz).normalize();

        const float ax= k_swapXZ ? n.z : n.x;
        const float az= k_swapXZ ? n.x : n.z;
        const float signX = -1.0f; // CHECK

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

    void SaveTexture() {
        if (m_paintTexture) {
            m_paintTexture->save("assets/textures/planet_painted.png");
        }
    }
};