#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "engine/input/input.h"
#include "engine/core/math.h"
#include "gamemode.h"

class Game;

class CreditsMode : public GameMode {
private:
    struct CreditsLine {
        std::string text;
        bool isTitle; // true = title font, false = name font
    };
    using CreditsPanel = std::vector<CreditsLine>;

    Game& m_game;
    UI& m_ui = m_game.GetUI();

    Entity* m_camera = nullptr;

    float m_animDuration = 1.0f;
    float m_fadeSpeed = 1.0f;     // alpha change per second (1.0 = fade over 1s)
    float m_holdDuration = 2.5f;  // seconds a fully-visible panel stays on screen

    // Look-up animation state
    float m_lookUpElapsed = 0.0f;
    bool m_lookUpDone = false;
    Vec3 m_lookUpStartRot;
    Vec3 m_lookUpTargetRot;

    // Panel fade state
    enum class PanelState { FadingIn, Holding, FadingOut };
    PanelState m_panelState = PanelState::FadingIn;
    float m_panelAlpha = 0.0f;
    float m_holdElapsed = 0.0f;
    size_t m_currentPanel = 0;

    std::vector<CreditsPanel> m_creditPanels = {
        {
            { m_ui.getText("crdt.dev"), true },
            { "4loyak!", false },
            { "", false },
            { m_ui.getText("crdt.builtwith"), true },
            { "Origin Engine", false },
            { "Origin Sandbox", false }
        },
        {
            { m_ui.getText("crdt.music"), true },
            { "Space Atmosphere · Alexandr Zhelanov", false },
            { "Terminal Pt. 2 · İlker Yalçıner", false },
        },
        {
            { m_ui.getText("crdt.licenses"), true },
            { "Skybox 102 · FreeStylized.com", false },
            { "Milky Way Skybox · ESA", false },
            { "Jersey10 Font · Google Fonts", false },
            { "Planet 3D Models · NASA", false },

        },
        {
            { m_ui.getText("crdt.tech"), true },
            { "OpenGL", false },
            { "Bullet3 Physics", false },
            { "SDL3", false },
            { "Dear ImGui", false },
            { "SoLoud", false },
        },
        {
            { m_ui.getText("crdt.specialthanks"), true },
            { "Hack Club", false },
        },
    };

public:
    CreditsMode(Game& game)
        : GameMode("Credits", "assets/scenes/menu.scene")
        , m_game(game) {}

    void OnEnter() override {
        m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Camera").get();

        m_lookUpElapsed = 0.0f;
        m_lookUpDone = false;

        m_panelState = PanelState::FadingIn;
        m_panelAlpha = 0.0f;
        m_holdElapsed = 0.0f;
        m_currentPanel = 0;

        if (m_camera) {
            m_lookUpStartRot = m_camera->transform.rotation;

            m_lookUpTargetRot = m_lookUpStartRot;
            m_lookUpTargetRot.x = 85.0f;
        }
    }

    void OnExit() override {}

    void Update() override {
        Input& input = m_game.GetEngine().getInput();
        if (input.isKeyPressed(KEY_ESCAPE) || input.isKeyPressed(KEY_SPACE) || input.isKeyPressed(KEY_ENTER)) {
            m_ui.loadMainMenu();
        }

        if (m_camera && !m_lookUpDone) {
            float dt = m_game.GetEngine().getDeltaTime();
            m_lookUpElapsed += dt;

            float t = m_lookUpElapsed / m_animDuration;
            if (t > 1.0f) t = 1.0f;
            if (t < 0.0f) t = 0.0f;

            float eased = t * t * (3.0f - 2.0f * t);

            m_camera->transform.rotation = Vec3::lerp(m_lookUpStartRot, m_lookUpTargetRot, eased);

            if (t >= 1.0f) m_lookUpDone = true;
            return;
        }

        DrawFadingCredits();
    }

private:
    void DrawFadingCredits() {
        if (m_currentPanel >= m_creditPanels.size()) {
            return;
        }

        float dt = m_game.GetEngine().getDeltaTime();

        switch (m_panelState) {
            case PanelState::FadingIn:
                m_panelAlpha += m_fadeSpeed * dt;
                if (m_panelAlpha >= 1.0f) {
                    m_panelAlpha = 1.0f;
                    m_panelState = PanelState::Holding;
                    m_holdElapsed = 0.0f;
                }
                break;

            case PanelState::Holding:
                m_holdElapsed += dt;
                if (m_holdElapsed >= m_holdDuration) {
                    m_panelState = PanelState::FadingOut;
                }
                break;

            case PanelState::FadingOut:
                m_panelAlpha -= m_fadeSpeed * dt;
                if (m_panelAlpha <= 0.0f) {
                    m_panelAlpha = 0.0f;
                    m_currentPanel++;

                    if (m_currentPanel >= m_creditPanels.size()) {
                        m_ui.loadMainMenu();
                        return;
                    }

                    m_panelState = PanelState::FadingIn;
                }
                break;
        }

        ImGuiIO& io = ImGui::GetIO();
        const ImVec2 displaySize = io.DisplaySize;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(displaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoDecoration;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_panelAlpha);
        ImGui::Begin("##CreditsOverlay", nullptr, flags);

        const CreditsPanel& panel = m_creditPanels[m_currentPanel];

        // First pass: total height, using each line's own font to get its line height.
        float totalHeight = 0.0f;
        for (const CreditsLine& line : panel) {
            m_ui.setFont(line.isTitle ? 2 : 1);
            totalHeight += ImGui::GetTextLineHeightWithSpacing();
            m_ui.resetFont();
        }

        // Second pass: draw each line centered, stacked top to bottom.
        float y = (displaySize.y - totalHeight) * 0.5f;
        for (const CreditsLine& line : panel) {
            m_ui.setFont(line.isTitle ? 2 : 1);
            const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
            const ImVec2 textSize = ImGui::CalcTextSize(line.text.c_str());
            ImGui::SetCursorPos(ImVec2((displaySize.x - textSize.x) * 0.5f, y));
            if (!line.isTitle) {
                ImGui::TextUnformatted(line.text.c_str());
            } else {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.6f), line.text.c_str());
            }
            m_ui.resetFont();
            y += lineHeight;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
};