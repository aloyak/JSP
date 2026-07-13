// This should only be used for spaceship building!
// this also holds the effect for the blueprint mode
#pragma once

#include <algorithm>
#include <cmath>

#include "engine/render/postprocess/postProcessor.h"
#include "engine/input/input.h"
#include "engine/components/entity.h"
#include "registries/partsRegistry.h"
#include "game.h"

class Blueprint {
private:
    Game* m_game = nullptr;
    Input* m_input = nullptr;

    Spaceship& m_ship;

    Entity* m_camera = nullptr;
    Entity* m_target = nullptr;
    PostProcessor* m_blueprint;

    const Vec3* m_geometryCenter = nullptr;

    int m_presetIndex = 0;

    bool m_isTurning = false;
    float m_transitionElapsed = 0.0f;
    float m_transitionDuration = 0.3f;
    float m_fromAngleDeg = 0.0f;
    float m_toAngleDeg = 0.0f;
    float m_transitionDistance = 0.0f;

    Texture m_blueprintTexture = Texture("assets/textures/blueprint.png");

    void startTurn(int direction) {
        int previousIndex = m_presetIndex;
        m_presetIndex = ((m_presetIndex + direction) % 4 + 4) % 4;

        m_transitionDistance = computeDistance();
        m_fromAngleDeg = 90.0f - 90.0f * static_cast<float>(previousIndex);
        m_toAngleDeg = m_fromAngleDeg - 90.0f * static_cast<float>(direction);

        m_transitionElapsed = 0.0f;
        m_isTurning = true;
    }

    float computeDistance() const {
        float fov = m_camera->getComponent<CameraComponent>()->getFOV();
        float cameraY = m_target->transform.position.y; // camera is kept level with the target

        ImGuiIO& io = ImGui::GetIO();
        float aspectRatio = (io.DisplaySize.y > 0.0f) ? (io.DisplaySize.x / io.DisplaySize.y) : 1.0f;

        return ComputeCameraDistance(m_ship, cameraY, fov, aspectRatio, kTopClearance, kBottomClearance);
    }

    bool showStats = true;
    bool showInfo = true;
    bool showStages = true;

    void drawInfo() {
        if (!showInfo) return;
        ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));

        ImGui::Begin("Blueprint Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        m_game->GetUI().setFont(0);
        ImGui::Text(&m_ship.name.c_str()[0]);
        m_game->GetUI().resetFont();    
        ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), &m_ship.version.c_str()[0]);
        ImGui::TextWrapped(&m_ship.description.c_str()[0]);

        ImGui::End();
        ImGui::PopStyleColor();
    }

    static constexpr float kMaxMass = 5000.0f;
    static constexpr float kMaxThrust = 20000.0f;
    static constexpr float kMaxFuel = 1000.0f;

    static constexpr float kStatLabelColumnWidth = 130.0f;
    static constexpr float kStatBarWidth = 160.0f;
    static constexpr float kStatBarHeight = 26.0f;

    void drawStatBar(const char* label, float value, float maxValue, const char* unit) {
        float fraction = std::clamp(maxValue > 0.0f ? value / maxValue : 0.0f, 0.0f, 1.0f);

        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "%s", label);
        ImGui::SameLine(kStatLabelColumnWidth);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1, 1, 1, 0.9f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "%.2f %s", value, unit);

        ImGui::ProgressBar(fraction, ImVec2(kStatBarWidth, kStatBarHeight), overlay);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    void drawStats() {
        if (!showStats) return;
        Vec2 windowSize = m_game->GetEngine().getWindow().getSize();
        ImGui::SetNextWindowPos(ImVec2(windowSize.x - 360, 80), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));

        ImGui::Begin("Blueprint Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        drawStatBar(m_game->GetUI().getText("mass"), m_ship.mass, kMaxMass, "kg");
        drawStatBar(m_game->GetUI().getText("thrust"), m_ship.thrust, kMaxThrust, "N");
        drawStatBar(m_game->GetUI().getText("fuel"), m_ship.fuelCapacity, kMaxFuel, "L");

        ImGui::End();
        ImGui::PopStyleColor();
    }

    void drawSettings() {
        Vec2 windowSize = m_game->GetEngine().getWindow().getSize();
        ImGui::SetNextWindowPos(ImVec2(40, windowSize.y - 50), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
        ImGui::Begin("Blueprint Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        m_game->GetUI().checkbox(m_game->GetUI().getText("scb.bp.info"), &showInfo);
        ImGui::SameLine();
        m_game->GetUI().checkbox(m_game->GetUI().getText("scb.bp.stats"), &showStats);
        ImGui::SameLine();
        m_game->GetUI().checkbox(m_game->GetUI().getText("scb.bp.stages"), &showStages);

        ImGui::End();
        ImGui::PopStyleColor();
    }

    void drawStages() {
        if (!showStages) return;
        Vec2 windowSize = m_game->GetEngine().getWindow().getSize();
        // center bottom of the screen
        ImGui::SetNextWindowSize(ImVec2(600, 300));
        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.5f - 300, windowSize.y * 0.5f + 300), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
        ImGui::Begin("Stages", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "Spaceship's stages:");

        ImGui::End();
        ImGui::PopStyleColor();
    }
public:
    static constexpr float kTopClearance = 8.0f;
    static constexpr float kBottomClearance = 6.0f;

    static constexpr float kMinDistance = 8.0f;

    static constexpr float kSideRollDegrees = 90.0f;

    static float ComputeCameraDistance(const Spaceship& ship, float cameraY, float verticalFovDegrees,
                                        float aspectRatio, float topClearance, float bottomClearance) {
        float halfVFovRad = (verticalFovDegrees * 0.5f) * (3.1415926535f / 180.0f);
        float halfHFovRad = std::atan(std::tan(halfVFovRad) * aspectRatio);
        float tanHalfFov = std::tan(halfHFovRad);
        if (tanHalfFov <= 0.0001f) return kMinDistance;

        float topExtent = std::max(0.0f, (ship.highestPartY - cameraY) + topClearance);
        float bottomExtent = std::max(0.0f, (cameraY - ship.lowestPartY) + bottomClearance);

        float requiredDistance = std::max(topExtent, bottomExtent) / tanHalfFov;
        return std::max(requiredDistance, kMinDistance);
    }

    static Vec3 presetPosition(const Vec3& center, int index, float distance) {
        float angleDegrees = 90.0f - 90.0f * static_cast<float>(index);
        float angle = angleDegrees * (3.1415926535f / 180.0f);
        return Vec3(center.x + distance * std::cos(angle),
                    center.y,
                    center.z + distance * std::sin(angle));
    }

    static int nearestPresetIndex(const Vec3& center, const Vec3& cameraPos) {
        Vec3 toCamera = cameraPos - center;
        float angleDegrees = std::atan2(toCamera.z, toCamera.x) * (180.0f / 3.1415926535f);
        int index = static_cast<int>(std::round((90.0f - angleDegrees) / 90.0f));
        return ((index % 4) + 4) % 4;
    }

    Blueprint(Entity* camera, Entity* target, Game &game, Spaceship& ship)
        : m_game(&game),
          m_target(target) ,
          m_camera(camera),
          m_ship(ship) {

        m_geometryCenter = &ship.geometryCenter;
        m_blueprint = &m_game->GetEngine().getRenderer().addPostProcessor(
              "assets/shaders/default_vert.glsl", "assets/shaders/blueprint_frag.glsl"
        );

        m_input = &m_game->GetEngine().getInput();

        // Keep the target locked to the ship's actual center of mass (not
        // just its height) so it's always centered on screen in blueprint mode.
        if (m_geometryCenter) {
            m_target->transform.position = *m_geometryCenter;
        }

        m_presetIndex = nearestPresetIndex(m_target->transform.position, m_camera->transform.position);
        
        m_game->GetEngine().getSceneManager().getActiveScene()->getEntityByName("Platform")->getComponent<RenderComponent>()->isEnabled = false; 
    }

    ~Blueprint() {
        m_game->GetEngine().getSceneManager().getActiveScene()->getEntityByName("Platform")->getComponent<RenderComponent>()->isEnabled = true; 
        m_game->GetEngine().getRenderer().removePostProcessor(m_blueprint);
    }

    void update(float deltaTime) {
        if (m_geometryCenter) {
            m_target->transform.position = *m_geometryCenter;
        }

        if (!m_isTurning) {
            if (m_input->isKeyPressed(KEY_W)) startTurn(1);
            else if (m_input->isKeyPressed(KEY_S)) startTurn(-1);
        }

        if (m_isTurning) {
            m_transitionElapsed += deltaTime;
            float t = std::clamp(m_transitionElapsed / m_transitionDuration, 0.0f, 1.0f);
            float s = t * t * (3.0f - 2.0f * t); // smoothstep

            float angleDeg = m_fromAngleDeg + (m_toAngleDeg - m_fromAngleDeg) * s;
            float angleRad = angleDeg * (3.1415926535f / 180.0f);
            const Vec3& center = m_target->transform.position;

            m_camera->transform.position = Vec3(center.x + m_transitionDistance * std::cos(angleRad),
                                                 center.y,
                                                 center.z + m_transitionDistance * std::sin(angleRad));

            if (t >= 1.0f) m_isTurning = false;
        }

        m_camera->getComponent<CameraComponent>()->lookAt(*m_target);
        m_camera->transform.rotation.z = kSideRollDegrees; // lay the ship on its side to use the wide screen axis

        drawBlueprintShader();

        drawSettings();
        drawInfo();
        drawStats();
        drawStages();
    }

    void drawBlueprintShader() {
        if (m_blueprint) {
            m_blueprint->setVec3("u_cameraPos", m_camera->transform.position);
            m_blueprint->setMat4("u_invView", m_camera->getComponent<CameraComponent>()->getCamera().getInvViewMatrix(m_camera->transform));
            m_blueprint->setMat4("u_invProj", m_camera->getComponent<CameraComponent>()->getCamera().getInvProjMatrix());

            m_blueprint->setFloat("u_near", m_camera->getComponent<CameraComponent>()->getNear());
            m_blueprint->setFloat("u_far", m_camera->getComponent<CameraComponent>()->getFar());

            m_blueprint->setTexture("blueprintImageTexture", m_blueprintTexture, 3);
        }
    }
};