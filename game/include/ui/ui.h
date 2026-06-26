#pragma once

#include <imgui.h>

#include "engine/utils/path.h"

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

    // defined in game.cpp
    void loadMainMenu();
    void drawSplashScreen(float bgAlpha, float logoAlpha, unsigned int textureId);
    void drawTransitionScreen(float alpha);

private:
    Game& m_game;

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