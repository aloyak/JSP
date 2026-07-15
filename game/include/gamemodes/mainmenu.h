#pragma once

// Not the best way to control version but whatever
#define VERSION "0.11.1"

#ifdef _WIN32
#include <windows.h>
#endif

#include "gamemode.h"
#include "components/planetComponent.h"
#include "engine/components/entity.h"
#include "game.h"

#include "gamemodes/sandbox.h"
#include "gamemodes/planetbuilder.h"
#include "gamemodes/spacecraftbuilder.h"
#include "gamemodes/credits.h"

#include "engine/render/texture.h"

#include <chrono>

class MainMenuMode : public GameMode {
private:
    Game& m_game;
    Engine& m_engine;
    AudioManager& m_audio;
    UI& m_ui;

    Entity* m_earthEntity = nullptr;
    PlanetComponent* m_menuPlanet = nullptr;

    bool showExtras = false;
    bool showMenuSettings = true;
    bool showSandbox = false;

    double m_simulatedTime = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    Texture logo = Texture("assets/logo_pixelized.png");
    Texture splashLogo = Texture("assets/originlogo.png");
    float m_splashTime = 0.0f;
    float m_splashDuration = 5.0f;
    bool m_showSplash;

    bool m_sceneSetupDone = false;

public:
    MainMenuMode(Game& game, bool showSplash = false)
        : GameMode("MainMenu", "assets/scenes/menu.scene")
        , m_game(game)
        , m_engine(game.GetEngine())
        , m_audio(game.GetAudioManager())
        , m_ui(game.GetUI())
        , m_showSplash(showSplash) {}

    void OnEnter() override {
        m_engine.getSceneManager().getActiveScene()->setAmbientStrength(0.05f);
        m_earthEntity = m_engine.getSceneManager().getActiveScene()->getEntityByName("Earth").get();
        m_game.timeScale = 25.0f;
        m_sceneSetupDone = false;
        m_menuPlanet = nullptr;

        if (m_showSplash) m_audio.playSound("assets/audio/splash.ogg", SFX);
        else m_audio.playMusic("assets/audio/space_atmosphere.ogg", true, 1.5f);
    }

    void OnExit() override {
        m_audio.stopMusic(2.0f);
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

            if (m_splashTime >= m_splashDuration &&
                !m_game.settingsManager.Get().firstRun)
            {
                m_showSplash = false;
                m_audio.playMusic("assets/audio/space_atmosphere.ogg", true, 2.0f);
            }
        }
        if (m_game.settingsManager.Get().firstRun) return;

        if (m_menuPlanet) {
            m_menuPlanet->update(m_engine.getDeltaTime() * m_game.timeScale);
        }
    }

    void LateUpdate() override {
        RenderMainMenu();
        if (showExtras) ShowExtras();
        if (showMenuSettings) ShowMenuSettings();

        bool firstRun = m_game.settingsManager.Get().firstRun;

        if (m_showSplash) {
            float bgAlpha = 1.0f;
            float logoAlpha = 0.0f;

            float fadeOutStart = m_splashDuration - 1.0f;
            if (fadeOutStart < 2.0f) fadeOutStart = 2.0f;

            if (m_splashTime <= 0.5f) logoAlpha = 0.0f;
            else if (m_splashTime <= 2.0f) logoAlpha = (m_splashTime - 0.5f) / 1.5f;
            else if (m_splashTime <= fadeOutStart) logoAlpha = 1.0f;
            else {
                float fadeOutDuration = m_splashDuration - fadeOutStart;
                float factor = (m_splashDuration - m_splashTime) / fadeOutDuration;

                if (firstRun) {
                    bgAlpha = 1.0f;
                    logoAlpha = std::max(0.0f, factor);
                }
                else {
                    bgAlpha = factor;
                    logoAlpha = factor;
                }
            }

            bgAlpha = std::clamp(bgAlpha, 0.0f, 1.0f);
            logoAlpha = std::clamp(logoAlpha, 0.0f, 1.0f);

            m_ui.drawSplashScreen(bgAlpha, logoAlpha, splashLogo.getID());
        }

        if (firstRun) {
            if (!m_showSplash || m_splashTime >= m_splashDuration) {
                RenderFirstRunScreen();
            }
            return;
        }
    }
    
    void RenderFirstRunScreen() {
        Vec2 windowSize = m_engine.getWindow().getSize();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, windowSize.y));
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Begin("FirstRunBg", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::End();
        ImGui::PopStyleColor();

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | 
                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));
        
        ImVec2 center = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin("FirstRunInfo", nullptr, flags);

        m_ui.setFont(1);
        ImGui::Text("Welcome to J.S.P!");
        m_ui.resetFont();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Before you begin, please note this is an early build.");
        ImGui::Text("Take a moment to check the availiable options in each game mode.");
        ImGui::Text("You can also adjust the settings in the main menu.");
        ImGui::Text("Many features or assets are still placeholders or incomplete.");
        ImGui::Text("For any feedback, which is wanted, please reach out!");

        ImGui::Spacing();
        ImGui::Text("Tutorial & instructions:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "Game > Help");
        
        ImGui::Spacing();

        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("- 4loyak!").x * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.6f), "- 4loyak!");

        ImGui::Spacing();

        if (m_ui.button("Acknowledge & Continue")) {
            m_game.settingsManager.Get().firstRun = false;
            m_game.settingsManager.Save();
            m_showSplash = false;

            m_audio.playMusic("assets/audio/space_atmosphere.ogg", true, 2.0f);
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
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
        if (m_ui.button("mm.campaign")) {}
        if (m_ui.button("mm.explore")) {} //m_game.SetGameMode(std::make_unique<ExploreMode>(m_game));
        ImGui::EndDisabled();

        
        if (m_ui.button("mm.sandbox")) { showSandbox = !showSandbox; }
        if (showSandbox) {
            ImGui::Indent();
            if (m_ui.button("mm.planeteditor")) m_game.SetGameMode(std::make_unique<PlanetBuilderMode>(m_game));
            if (m_ui.button("mm.sceditor")) m_game.SetGameMode(std::make_unique<SpacecraftBuilderMode>(m_game));
            if (m_ui.button("mm.gravitysandbox")) m_game.SetGameMode(std::make_unique<SandboxMode>(m_game), false, false);
            ImGui::Unindent();
        }
        
        if (m_ui.button("mm.settings")) { m_game.showSettings = !m_game.showSettings; }
        if (m_ui.button("mm.extras")) { showExtras = !showExtras; }
        
        if (m_ui.button("mm.quit")) m_engine.stop();
        ImGui::PopStyleColor(2);
        m_ui.resetFont();
        ImGui::End();

        ImGui::PopStyleColor(2);

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
        
        ImGui::Begin(m_ui.getText("mm.extras"), &showExtras, flags);

        m_ui.setFont(1);
        CenterText(m_ui.getText("mm.credits"));
        m_ui.resetFont();
        
        CenterText(m_ui.getText("crdt.author"));
        CenterText(m_ui.getText("crdt.src"));
        CenterText(m_ui.getText("crdt.eng"));
                
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
        const char* label = m_ui.getText("crdt.full");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x - ImGui::GetStyle().FramePadding.x * 2.0f) * 0.5f);
        if (m_ui.button(label)) { 
            m_game.showSettings = false;
            m_game.showHelp = false;
            m_game.SetGameMode(std::make_unique<CreditsMode>(m_game), false, false);
        }
        ImGui::Spacing();

        m_ui.setFont(1);
        CenterText(m_ui.getText("crdt.thank"));
        m_ui.resetFont();
                
        CenterText(m_ui.getText("crdt.1"));
        CenterText(m_ui.getText("crdt.2"));

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