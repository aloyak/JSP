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

private:
    Game& m_game;

    void setStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        
        //style.WindowPadding = ImVec2(30, 30);
        //style.ItemSpacing = ImVec2(10, 12);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
    }
};