#include "game.h"
#include "engine/utils/logger.h"
#include "gamemodes/mainmenu.h"

Game::Game(Engine& engine) : m_engine(&engine), m_ui(*this), m_currentGameMode(nullptr), settingsManager("user/settings.json") {
    m_engine->initUI(); 
    ImGuiIO& io = m_engine->getIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    m_engine->getRenderer().setupRenderTarget(600, 400);
    m_engine->getRenderer().setPixelArt(true, 16);

    m_audio.setAudioSystem(&m_engine->getAudioSystem());

    settingsManager.Load();
    auto& settings = settingsManager.Get();

    m_engine->getWindow().setFullscreen(settings.isFullscreen); // TODO: SET ON RELEASE
    m_engine->getWindow().enableVSync(settings.vsyncEnabled);
    // m_engine->getWindow().allowResize(false); // TODO
    m_engine->setTargetFps(settings.targetFPS);

    m_audio.setVolume(settings.masterVolume, Master);
    m_audio.setVolume(settings.musicVolume, Music);
    m_audio.setVolume(settings.sfxVolume, SFX);

    //Logger::setVerbose(1); // TODO: SET ON RELEASE

    m_ui.initialize(); 
}

void Game::Update() {
    m_engine->beginUI();

    m_audio.update(m_engine->getDeltaTime());

    if (m_transitionState == TransitionState::FadingOut) {
        m_transitionAlpha += m_engine->getDeltaTime() * m_transitionSpeed;
        if (m_transitionAlpha >= 1.0f) {
            m_transitionAlpha = 1.0f;
            m_transitionState = TransitionState::None;
            m_pendingModeChange = true;
            m_waitingForFadeIn = true;
        }
    } else if (m_transitionState == TransitionState::FadingIn) {
        m_transitionAlpha -= m_engine->getDeltaTime() * m_transitionSpeed;
        if (m_transitionAlpha <= 0.0f) {
            m_transitionAlpha = 0.0f;
            m_transitionState = TransitionState::None;
        }
    }

    if (m_pendingModeChange) {
        m_pendingModeChange = false;
        ApplyGameMode(std::move(m_nextGameMode), m_nextForceReload);
    } else if (m_waitingForFadeIn) {
        m_waitingForFadeIn = false;
        m_transitionState = TransitionState::FadingIn;
    }

    if (m_currentGameMode) m_currentGameMode->Update();
    if (showSettings) m_ui.drawSettingsWindow(); 
    if (showHelp) m_ui.drawHelpWindow();
}

void Game::LateUpdate() {
    if (m_currentGameMode) {
        m_currentGameMode->LateUpdate();
    }

    // DEBUG: style editor
    //ImGui::ShowStyleEditor();

    if (m_transitionState != TransitionState::None || m_transitionAlpha > 0.0f) {
        m_ui.drawTransitionScreen(m_transitionAlpha);
    }

    m_engine->endUI();
}

void Game::SetGameMode(std::unique_ptr<GameMode> newGameMode, bool forceReload, bool transition) {
    m_nextGameMode = std::move(newGameMode);
    m_nextForceReload = forceReload;
    
    if (transition) {
        m_transitionState = TransitionState::FadingOut;
        m_transitionAlpha = 0.0f;
    } else {
        m_pendingModeChange = true;
    }
}

void Game::ApplyGameMode(std::unique_ptr<GameMode> newGameMode, bool forceReload) {
    bool same = m_currentGameMode && 
        !newGameMode->GetScenePath().empty() && 
        m_currentGameMode->GetScenePath() == newGameMode->GetScenePath();

    if (m_currentGameMode) {
        if ((!same || forceReload) && !m_currentGameMode->GetScenePath().empty() && (forceReload || m_engine->getSceneManager().getActiveScene() != nullptr)) {
            m_engine->getSceneManager().unload();
        }
        m_currentGameMode->OnExit();
    }

    m_lastGameMode = std::move(m_currentGameMode);
    m_currentGameMode = std::move(newGameMode);

    if (m_currentGameMode) {
        if ((!same || forceReload) && !m_currentGameMode->GetScenePath().empty() && (forceReload || m_engine->getSceneManager().getActiveScene() == nullptr)) {
            m_engine->getSceneManager().load(m_currentGameMode->GetScenePath());
        }
        m_currentGameMode->OnEnter();
    }
}

void UI::drawSplashScreen(float bgAlpha, float logoAlpha, unsigned int textureId) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    Vec2 windowSize = m_game.GetEngine().getWindow().getSize();
    ImGui::SetNextWindowSize(ImVec2(windowSize.x, windowSize.y));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, bgAlpha));
    ImGui::Begin("Splash Screen", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    float originalWidth = 1999.0f;
    float originalHeight = 676.0f;
    float aspectRatio = originalWidth / originalHeight;

    float maxWidth = windowSize.x * 0.5f;
    float maxHeight = windowSize.y * 0.5f;

    float logoWidth = maxWidth;
    float logoHeight = logoWidth / aspectRatio;

    if (logoHeight > maxHeight) {
        logoHeight = maxHeight;
        logoWidth = logoHeight * aspectRatio;
    }

    ImGui::SetCursorPos(ImVec2((windowSize.x - logoWidth) * 0.5f, (windowSize.y - logoHeight) * 0.5f));
    ImGui::Image((void*)(intptr_t)textureId, ImVec2(logoWidth, logoHeight), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, logoAlpha), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::End();
    ImGui::PopStyleColor();
}

void UI::drawTransitionScreen(float alpha) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    Vec2 windowSize = m_game.GetEngine().getWindow().getSize();
    ImU32 color = IM_COL32(0, 0, 0, static_cast<int>(alpha * 255));
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(windowSize.x, windowSize.y), color);

    if (alpha > 0.3f) {
        float iconAlpha = (alpha - 0.3f) / 0.7f;
        if (iconAlpha > 1.0f) iconAlpha = 1.0f;

        float time = static_cast<float>(ImGui::GetTime());
        float angle = time * 3.0f;

        float radius = 12.0f;
        float thickness = 3.0f;
        float padding = 28.0f;
        ImVec2 center = ImVec2(windowSize.x - padding, windowSize.y - padding);

        ImU32 trackColor = IM_COL32(255, 255, 255, static_cast<int>(40 * iconAlpha));
        drawList->AddCircle(center, radius, trackColor, 32, thickness);

        float arcSpan = M_PI * 1.25f;
        ImU32 arcColor = IM_COL32(255, 255, 255, static_cast<int>(200 * iconAlpha));
        drawList->PathArcTo(center, radius, angle, angle + arcSpan, 32);
        drawList->PathStroke(arcColor, 0, thickness);
    }
}

void UI::loadMainMenu() {
    m_game.SetGameMode(std::make_unique<MainMenuMode>(m_game, false), true, true);
}

void UI::updateButtonSfx(ImGuiID id) {
    const bool hovered = ImGui::IsItemHovered();

    if (ImGui::IsItemClicked()) {
        m_game.GetAudioManager().playSound(m_pressSoundPath, UserInterface);
    }

    const bool wasHovered = m_hoveredLastFrame.count(id) != 0;
    if (hovered && !wasHovered) {
        m_game.GetAudioManager().playSound(m_hoverSoundPath, UserInterface);
    }

    if (hovered) {
        m_hoveredLastFrame.insert(id);
    } else {
        m_hoveredLastFrame.erase(id);
    }
}

// TODO: user settings (change on engine, not game)
void UI::drawSettingsWindow() {
    Engine& m_engine = m_game.GetEngine();
    AudioManager& m_audio = m_game.GetAudioManager();
    auto& settings = m_game.settingsManager.Get();
    bool settingsChanged = false;

    

    int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowPos(ImVec2(650, 350), ImGuiCond_Once);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

    ImGui::Begin("Settings", &m_game.showSettings, flags);

    this->setFont(1);
    centerText("Settings");
    this->resetFont();
        
    ImGui::SeparatorText("Language");
    const char* languages[] = { "English", "SOON" };
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##Language", &settings.language, languages, IM_ARRAYSIZE(languages))) {
        settingsChanged = true;
    }

    ImGui::SeparatorText("Graphics");
        
    if (checkbox("Fullscreen", &settings.isFullscreen)) {
        m_engine.getWindow().setFullscreen(settings.isFullscreen);
        settingsChanged = true;
    }

    ImGui::SameLine(150.0f);

    if (checkbox("VSync", &settings.vsyncEnabled)) {
        m_engine.getWindow().enableVSync(settings.vsyncEnabled);
        settingsChanged = true;
    }

    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderInt("##TargetFPS", &settings.targetFPS, 30, 1000, (settings.targetFPS == 1000) || (settings.targetFPS == -1) ? "Target FPS: Unlimited" : "Target FPS: %d")) {
        if (settings.targetFPS >= 1000) {
            settings.targetFPS = -1;
        }
        m_engine.setTargetFps(settings.targetFPS);
        settingsChanged = true;
    }

    ImGui::SeparatorText("Gameplay");
    if (ImGui::SliderFloat("##MouseSensitivity", &settings.mouseSens, 0.1f, 100.0f, "Mouse Sensitivity: %.2f")) {
        settingsChanged = true;
    }

    if (ImGui::SliderFloat("##HeadBobIntensity", &settings.headbobIntensity, 0.0f, 1.0f, "Head Bob Intensity: %.2f")) {
        settingsChanged = true;
    }

    ImGui::SeparatorText("Audio");

    float masterVol = settings.masterVolume * 100.0f;
    if (ImGui::SliderFloat("##MasterVolume", &masterVol, 0.0f, 100.0f, "Master Volume: %.2f%%")) {
        settings.masterVolume = masterVol * 0.01f;
        m_audio.setVolume(settings.masterVolume, Master);
        settingsChanged = true;
    }

    float musicVol = settings.musicVolume * 100.0f;
    if (ImGui::SliderFloat("##MusicVolume", &musicVol, 0.0f, 100.0f, "Music Volume: %.2f%%")) {
        settings.musicVolume = musicVol * 0.01f;
        m_audio.setVolume(settings.musicVolume, Music);
        settingsChanged = true;
    }

    float sfxVol = settings.sfxVolume * 100.0f;
    if (ImGui::SliderFloat("##SFXVolume", &sfxVol, 0.0f, 100.0f, "SFX Volume: %.2f%%")) {
        settings.sfxVolume = sfxVol * 0.01f;
        m_audio.setVolume(settings.sfxVolume, SFX);
        settingsChanged = true;
    }

    if (settingsChanged) {
        m_game.settingsManager.Save();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void UI::drawHelpWindow() {
    int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowPos(ImVec2(650, 350), ImGuiCond_Once);
    ImGui::Begin("Help", &m_game.showHelp, flags);

    this->setFont(1);
    centerText("Help");
    this->resetFont();
    
    std::string gamemode = m_game.GetCurrentGameMode()->modeName;

    ImGui::SeparatorText(gamemode.c_str());

    if (gamemode == "Sandbox") {
        ImGui::Text("WASD - Move");
        ImGui::Text("Shift - Y+");
        ImGui::Text("Ctrl - Y-");
        ImGui::Text("Scroll - Zoom In/Out");
        ImGui::Text("Left Click - Move Camera");
        ImGui::Text("Right Click - Apply Tool");
        ImGui::Text("1,2,3 - Tools");
    } else if (gamemode == "PlanetBuilder") {
        ImGui::Text("Right Click - Apply Tool");
        ImGui::Text("Left Click - Move Camera");
        ImGui::Text("Scroll - Zoom In/Out");
        ImGui::Text("1,2,3 - Tools");
    } else if (gamemode == "SpacecraftBuilder") {
        ImGui::SeparatorText("Orbit Camera");
        ImGui::Text("Left Click - Move Camera");
        ImGui::Text("Scroll - Zoom In/Out");
        ImGui::Text("Shift - Y+");
        ImGui::Text("Ctrl - Y-");
        ImGui::SeparatorText("First Person Camera");
        ImGui::Text("WASD - Move");
        ImGui::Text("Left Alt - Show Cursor");
        ImGui::SeparatorText("Blueprint Controls");
        ImGui::TextDisabled("Operation Mode > Blueprint");
        ImGui::Text("A - Rotate Left");
        ImGui::Text("D - Rotate Right");
    } else {
        ImGui::Text("No help available for this game mode.");
    }

    ImGui::End();
};