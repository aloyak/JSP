#pragma once

// Not the best way to control version but whatever
#define VERSION "0.5.1"

#include "gamemode.h"
#include "components/PlanetComponent.h"
#include "engine/components/entity.h"
#include "game.h"

#include "gamemodes/sandbox.h"
#include "gamemodes/planetbuilder.h"
#include "gamemodes/spacecraftbuilder.h"

#include "engine/render/texture.h"

#include <chrono>

class MainMenuMode : public GameMode {
private:
    Game& m_game;
    Engine& m_engine;
    UI& m_ui;

    Entity* m_earthEntity = nullptr;
    PlanetComponent* m_menuPlanet = nullptr;

    bool showSettings = false;
    bool showExtras = false;
    bool showMenuSettings = true;
    bool showSandbox = false;

    double m_simulatedTime = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    Texture logo = Texture("assets/logo.png");
    Texture splashLogo = Texture("assets/originlogo.png");
    float m_splashTime = 0.0f;
    float m_splashDuration = 4.0f;
    bool m_showSplash;

    bool m_sceneSetupDone = false;

public:
    MainMenuMode(Game& game, bool showSplash = false)
        : GameMode("assets/scenes/menu.scene") 
        , m_game(game)
        , m_engine(game.GetEngine())
        , m_ui(game.GetUI())
        , m_showSplash(showSplash) {}

    void OnEnter() override {
        m_engine.getSceneManager().getActiveScene()->setAmbientStrength(0.05f);
        m_earthEntity = m_engine.getSceneManager().getActiveScene()->getEntityByName("Earth").get();
        m_game.timeScale = 25.0f;
        m_sceneSetupDone = false;
        m_menuPlanet = nullptr;
    }

    void OnExit() override {
        m_game.timeScale = 1.0f;
    }

    void Update() override {
        if (!m_sceneSetupDone) {
            if (m_earthEntity) {
                PlanetComponent* jsonPlanet = m_earthEntity->getComponent<PlanetComponent>();
                if (jsonPlanet) {
                    jsonPlanet->isEnabled = false;
                }

                m_menuPlanet = m_earthEntity->addComponent<PlanetComponent>(m_game);
                m_menuPlanet->setHasAtmosphere(true);
                m_menuPlanet->getAtmosphere().thickness = 150.0f;
                m_menuPlanet->getAtmosphere().rayleighCoeff = Vec3(0.006f, 0.014f, 0.033f);
                m_menuPlanet->getPlanetParams().sunIntensity = 8.5f;
                m_menuPlanet->getAtmosphere().edgeFalloff = 300.0f;

                m_menuPlanet->initialize();
            }

            Entity* cameraEntity = m_engine.getSceneManager().getActiveScene()->getEntityByName("Camera").get();
            if (cameraEntity) {
                m_engine.setActiveCamera(cameraEntity);
            }

            m_sceneSetupDone = true;
        }

        if (m_showSplash) {
            m_splashTime += m_engine.getDeltaTime();
            if (m_splashTime >= m_splashDuration) {
                m_showSplash = false;
            }
        }

        if (m_menuPlanet) {
            m_menuPlanet->update(m_engine.getDeltaTime() * m_game.timeScale);
        }
    }

    void LateUpdate() override {
        RenderMainMenu();

        if (showSettings) ShowSettings();
        if (showExtras) ShowExtras();

        if (showMenuSettings) ShowMenuSettings();

        if (m_showSplash) {
            float bgAlpha = 1.0f;
            float logoAlpha = 0.0f;
            float fadeOutStart = m_splashDuration - 1.0f;

            if (fadeOutStart < 2.0f) {
                fadeOutStart = 2.0f;
            }

            if (m_splashTime <= 0.5f) {
                bgAlpha = 1.0f;
                logoAlpha = 0.0f;
            } else if (m_splashTime <= 2.0f) {
                bgAlpha = 1.0f;
                logoAlpha = (m_splashTime - 0.5f) / 1.5f;
            } else if (m_splashTime <= fadeOutStart) {
                bgAlpha = 1.0f;
                logoAlpha = 1.0f;
            } else {
                float fadeOutDuration = m_splashDuration - fadeOutStart;
                float fadeOutFactor = (m_splashDuration - m_splashTime) / fadeOutDuration;
                bgAlpha = fadeOutFactor;
                logoAlpha = fadeOutFactor;
            }

            if (bgAlpha < 0.0f) bgAlpha = 0.0f;
            if (bgAlpha > 1.0f) bgAlpha = 1.0f;
            if (logoAlpha < 0.0f) logoAlpha = 0.0f;
            if (logoAlpha > 1.0f) logoAlpha = 1.0f;

            m_ui.drawSplashScreen(bgAlpha, logoAlpha, splashLogo.getID());
        }
    }

    void RenderMainMenu() {
        int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 30));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        ImGui::SetNextWindowPos(ImVec2(80, 80));
        ImGui::Begin("Main Menu", nullptr, flags);

        // add alpha to the logo
        float scaleFactor = m_engine.getWindow().getSize().y / 1080.0f;
        ImVec2 logoSize = ImVec2(250 * scaleFactor, 250 * scaleFactor);
        ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, 0.75f);
        ImVec4 borderColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImGui::Image(logo.getID(), logoSize, ImVec2(0, 0), ImVec2(1, 1), tintColor, borderColor);

        m_ui.setFont(2);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.35f), "Janitor Space Program - %s", VERSION);

        ImGui::SeparatorText("Select:");

        m_ui.resetFont();
        m_ui.setFont(1);

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.5f, 0.9f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.4f, 1.0f, 0.75f));
        ImGui::BeginDisabled();
        if (ImGui::Button("Campaign Mode")) {}
        if (ImGui::Button("Explore the Solar System")) {} //m_game.SetGameMode(std::make_unique<ExploreMode>(m_game));
        ImGui::EndDisabled();

        
        if (ImGui::Button("Sandbox Mode")) { showSandbox = !showSandbox; }
        if (showSandbox) {
            ImGui::Indent();
            if (ImGui::Button("Planet Editor")) m_game.SetGameMode(std::make_unique<PlanetBuilderMode>(m_game));
            if (ImGui::Button("Spacecraft Editor")) m_game.SetGameMode(std::make_unique<SpacecraftBuilderMode>(m_game));
            if (ImGui::Button("Gravity Sandbox")) m_game.SetGameMode(std::make_unique<SandboxMode>(m_game), false, false);
            ImGui::Unindent();
        }
        
        if (ImGui::Button("Settings")) { showSettings = !showSettings; }
        if (ImGui::Button("Extras")) { showExtras = !showExtras; }
        
        if (ImGui::Button("Exit")) m_engine.stop();
        ImGui::PopStyleColor(2);
        m_ui.resetFont();
        ImGui::End();

        ImGui::PopStyleColor(2);

        ImGui::PopStyleVar(2);
    }

    // TODO: user settings (change on engine, not game)
    void ShowSettings() {
        auto CenterText = [](const char* text) {
            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize(text).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextUnformatted(text);
        };

        int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowPos(ImVec2(650, 350), ImGuiCond_Once);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        ImGui::Begin("Settings", &showSettings, flags);

        m_ui.setFont(1);
        CenterText("Settings");
        m_ui.resetFont();
        
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Warning: Settings are not currently saved.");
        
        ImGui::SeparatorText("Language");
        static int langIndex = 0;
        const char* languages[] = { "English", "SOON" };
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##Language", &langIndex, languages, IM_ARRAYSIZE(languages));

        ImGui::SeparatorText("Graphics");
        
        bool isFullscreen = m_engine.getWindow().isFullscreen();
        if (ImGui::Checkbox("Fullscreen", &isFullscreen)) {
            m_engine.getWindow().setFullscreen(isFullscreen);
        }

        ImGui::SameLine(150.0f);

        bool vsyncEnabled = m_engine.getWindow().isVSyncEnabled();
        if (ImGui::Checkbox("VSync", &vsyncEnabled)) {
            m_engine.getWindow().enableVSync(vsyncEnabled);
        }

        int targetFPS = m_engine.getTargetFps();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderInt("##TargetFPS", &targetFPS, 30, 1000, (targetFPS == 1000) || (targetFPS == -1) ? "Target FPS: Unlimited" : "Target FPS: %d")) {
            if (targetFPS >= 1000) {
                targetFPS = -1; // fps < 0 => unlimited
            }
            m_engine.setTargetFps(targetFPS);
        }

        ImGui::SeparatorText("Audio");

        static float masterVolume = 1.0f;
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##MasterVolume", &masterVolume, 0.0f, 1.0f, "Volume: %.0f%%", ImGuiSliderFlags_Logarithmic);

        static float musicVolume = 0.8f;
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##MusicVolume", &musicVolume, 0.0f, 1.0f, "Music: %.0f%%", ImGuiSliderFlags_Logarithmic);

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void ShowExtras() {
        auto CenterText = [](const char* text, ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text)) {
            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize(text).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(textColor, text);
        };

        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
        
        ImGui::SetNextWindowPos(ImVec2(650, 200), ImGuiCond_Once);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        
        ImGui::Begin("Extras", &showExtras, flags);

        m_ui.setFont(1);
        CenterText("Credits");
        m_ui.resetFont();
        
        CenterText("Developer: 4loyak! (@aloyak)");
        CenterText("This project is open source!");
        CenterText("Built with Origin Engine");
                
        auto createLink = [&CenterText](const char* url) {
            CenterText(url, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (ImGui::IsMouseClicked(0)) {
    #ifdef _WIN32
                    ShellExecute(0, 0, url, 0, 0, SW_SHOW);
    #elif __APPLE__
                    system(("open " + std::string(url)).c_str());
    #else
                    system(("xdg-open " + std::string(url)).c_str());
    #endif
                }
            }
        };

        createLink("https://github.com/aloyak/JSP");
        createLink("https://github.com/aloyak/origin");

        ImGui::Spacing();

        m_ui.setFont(1);
        CenterText("Special thanks to:");
        m_ui.resetFont();
                
        CenterText("NASA & ESA for free quality assets");
        CenterText("Hack Club for being awesome!");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        CenterText("Extras coming SOON^(tm)");

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void ShowMenuSettings() {
        int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | 
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        //ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImVec2 windowSize = ImVec2(200, 80);
        ImVec2 padding = ImVec2(20, 20);

        ImVec2 pos = ImVec2(
            displaySize.x - windowSize.x - padding.x,
            displaySize.y - windowSize.y - padding.y
        );

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

        ImGui::Begin("Background Settings", nullptr, flags);

        m_simulatedTime += (m_engine.getDeltaTime() * m_game.timeScale);
        std::time_t displayTime = static_cast<std::time_t>(m_simulatedTime);
        std::tm* localTime = std::localtime(&displayTime);
        char timeBuffer[64];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%B %d %Y, %H:%M:%S", localTime);

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.6f), "%s", timeBuffer);
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("##TimeScale", &m_game.timeScale, 1.0f, 10000.0f, "Time Scale: %.1fx", ImGuiSliderFlags_Logarithmic);

        ImGui::End();
        //ImGui::PopStyleColor();
        //ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

};