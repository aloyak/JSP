#pragma once

#include <imgui.h>

#include "engine/utils/path.h"
#include "audio/audioManager.h"
#include "engine/render/texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Game;
class MainMenuMode;

class UI {
public:
    std::vector<ImFont*> fonts;

    UI(Game& game) : m_game(game) {}

    void initialize() {
        setStyle();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(Path::resolve("assets/fonts/Jersey10-Regular.ttf").c_str(), 24.0f);
        io.Fonts->AddFontFromFileTTF(Path::resolve("assets/fonts/Jersey10-Regular.ttf").c_str(), 32.0f);
        io.Fonts->AddFontFromFileTTF(Path::resolve("assets/fonts/Jersey10-Regular.ttf").c_str(), 128.0f);
        fonts.push_back(io.Fonts->Fonts.back());
        fonts.push_back(io.Fonts->Fonts[io.Fonts->Fonts.size() - 2]);
        fonts.push_back(io.Fonts->Fonts[io.Fonts->Fonts.size() - 3]);
    }

    void setFont(float index) {
        if (index < 0 || index >= fonts.size()) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        } else {
            ImGui::PushFont(fonts[index]);
        }
    }

    void resetFont() {
        ImGui::PopFont();
    }

    bool button(const char* label, const ImVec2& size = ImVec2(0, 0)) {
        const bool clicked = ImGui::Button(label, size);
        updateButtonSfx(ImGui::GetID(label));
        return clicked;
    }

    bool buttonWithId(const char* label, ImGuiID soundId, const ImVec2& size = ImVec2(0, 0)) {
        const bool clicked = ImGui::Button(label, size);
        updateButtonSfx(soundId);
        return clicked;
    }

    bool imageButton(const char* iconName, const ImVec2& size = ImVec2(0, 0), int padding = 50) {
        const std::string iconPath = "assets/textures/icons/" + std::string(iconName) + ".png";

        auto it = m_iconTextures.find(iconName);
        if (it == m_iconTextures.end()) {
            auto texture = std::make_unique<Texture>(iconPath);
            it = m_iconTextures.emplace(iconName, std::move(texture)).first;
        }

        Texture* texture = it->second.get();
        if (!texture || texture->getID() == 0) {
            return ImGui::Button(iconName, size);
        }

        const float imagePadding = padding > 0 ? static_cast<float>(padding) : 0.0f;
        ImVec2 buttonSize = size;
        if (buttonSize.x == 0.0f) {
            buttonSize.x = static_cast<float>(texture->getWidth()) + (imagePadding * 2.0f);
        } else if (buttonSize.x < 0.0f) {
            buttonSize.x = ImGui::GetContentRegionAvail().x + buttonSize.x;
            if (buttonSize.x < 0.0f) {
                buttonSize.x = 0.0f;
            }
        }

        if (buttonSize.y == 0.0f) {
            buttonSize.y = static_cast<float>(texture->getHeight()) + (imagePadding * 2.0f);
        } else if (buttonSize.y < 0.0f) {
            buttonSize.y = ImGui::GetContentRegionAvail().y + buttonSize.y;
            if (buttonSize.y < 0.0f) {
                buttonSize.y = 0.0f;
            }
        }

        const ImVec2 imageSize(
            buttonSize.x > (imagePadding * 2.0f) ? buttonSize.x - (imagePadding * 2.0f) : 0.0f,
            buttonSize.y > (imagePadding * 2.0f) ? buttonSize.y - (imagePadding * 2.0f) : 0.0f
        );

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(imagePadding, imagePadding));
        const bool clicked = ImGui::ImageButton(iconName, (ImTextureID)(intptr_t)texture->getID(), imageSize);
        updateButtonSfx(ImGui::GetID(iconName));
        ImGui::PopStyleVar();
        return clicked;
    }

    bool checkbox(const char* label, bool* v) {
        const bool clicked = ImGui::Checkbox(label, v);
        updateButtonSfx(ImGui::GetID(label));
        return clicked;
    }

    bool beginTabItem(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0) {
        const bool opened = ImGui::BeginTabItem(label, p_open, flags);
        updateButtonSfx(ImGui::GetID(label));
        return opened;
    }

    bool beginMenu(const char* label, bool enabled = true) {
        const bool opened = ImGui::BeginMenu(label, enabled);
        updateButtonSfx(ImGui::GetID(label));
        return opened;
    }

    bool menuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true) {
        const bool clicked = ImGui::MenuItem(label, shortcut, selected, enabled);
        updateButtonSfx(ImGui::GetID(label));
        return clicked;
    }

    void setButtonHoverSound(const std::string& path) { m_hoverSoundPath = path; }
    void setButtonPressSound(const std::string& path) { m_pressSoundPath = path; }

    // defined in game.cpp
    void loadMainMenu();
    void drawSplashScreen(float bgAlpha, float logoAlpha, unsigned int textureId);
    void drawTransitionScreen(float alpha);
    void drawSettingsWindow();
private:
    Game& m_game;

    std::string m_hoverSoundPath = "assets/audio/UI/hover.wav";
    std::string m_pressSoundPath = "assets/audio/UI/press.wav";

    std::unordered_map<std::string, std::unique_ptr<Texture>> m_iconTextures;
    std::unordered_set<ImGuiID> m_hoveredLastFrame;

    void updateButtonSfx(ImGuiID id); // defined in game.cpp, needs Game::GetAudioManager

    void setStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.PopupBorderSize = 0.0f;
        
        //style.WindowPadding = ImVec2(30, 30);
        //style.ItemSpacing = ImVec2(10, 12);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_FrameBg]                = ImVec4(0.24f, 0.24f, 0.24f, 0.63f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.23f, 0.23f, 0.23f, 0.45f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.33f, 0.33f, 0.33f, 0.78f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.24f, 0.24f, 0.24f, 0.63f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.17f, 0.17f, 0.17f, 0.65f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.16f, 0.16f, 0.16f, 0.80f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.24f, 0.24f, 0.24f, 0.63f);
        colors[ImGuiCol_TabSelected]            = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
    }
};