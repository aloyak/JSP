#include "game.h"
#include "engine/utils/logger.h"
#include "engine/utils/path.h"
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
    m_ui.setLang(settings.language);
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
        m_currentGameMode->OnExit();
        if ((!same || forceReload) && !m_currentGameMode->GetScenePath().empty() && (forceReload || m_engine->getSceneManager().getActiveScene() != nullptr)) {
            m_engine->getSceneManager().unload();
        }
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

void UI::drawSettingsWindow() {
    Engine& m_engine = m_game.GetEngine();
    AudioManager& m_audio = m_game.GetAudioManager();
    auto& settings = m_game.settingsManager.Get();
    bool settingsChanged = false;

    int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowPos(ImVec2(650, 350), ImGuiCond_Once);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

    ImGui::Begin(getText("sttngs"), &m_game.showSettings, flags);

    ImGui::SeparatorText(getText("sttngs.lang"));
    const char* languages[] = { "English", "Español", "SOON..." };
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##Language", &settings.language, languages, IM_ARRAYSIZE(languages))) {
        setLang(settings.language);
        settingsChanged = true;
    }

    ImGui::SeparatorText(getText("sttngs.graphics"));
        
    if (checkbox(getText("sttngs.fullscreen"), &settings.isFullscreen)) {
        m_engine.getWindow().setFullscreen(settings.isFullscreen);
        settingsChanged = true;
    }

    if (checkbox("VSync", &settings.vsyncEnabled)) {
        m_engine.getWindow().enableVSync(settings.vsyncEnabled);
        settingsChanged = true;
    }

    ImGui::SetNextItemWidth(-1.0f);
    std::string targetFpsFormat = (settings.targetFPS == 1000 || settings.targetFPS == -1) ? getText("sttngs.target.unlimited") : std::string(getText("sttngs.target")) + " %d";
    if (ImGui::SliderInt("##TargetFPS", &settings.targetFPS, 30, 1000, targetFpsFormat.c_str())) {
        if (settings.targetFPS >= 1000) {
            settings.targetFPS = -1;
        }
        m_engine.setTargetFps(settings.targetFPS);
        settingsChanged = true;
    }

    ImGui::SeparatorText(getText("sttngs.gameplay"));
    std::string mouseSensFormat = std::string(getText("sttngs.sens")) + " %.1f";
    if (ImGui::SliderFloat("##MouseSensitivity", &settings.mouseSens, 0.1f, 100.0f, mouseSensFormat.c_str())) {
        settingsChanged = true;
    }

    std::string headBobFormat = std::string(getText("sttngs.headBob")) + " %.2f";
    if (ImGui::SliderFloat("##HeadBobIntensity", &settings.headbobIntensity, 0.0f, 1.0f, headBobFormat.c_str())) {
        settingsChanged = true;
    }

    ImGui::SeparatorText(getText("sttngs.audio"));

    float masterVol = settings.masterVolume * 100.0f;
    std::string masterVolFormat = std::string(getText("sttngs.vol.master")) + " %.0f%%";
    if (ImGui::SliderFloat("##MasterVolume", &masterVol, 0.0f, 100.0f, masterVolFormat.c_str())) {
        settings.masterVolume = masterVol * 0.01f;
        m_audio.setVolume(settings.masterVolume, Master);
        settingsChanged = true;
    }

    float musicVol = settings.musicVolume * 100.0f;
    std::string musicVolFormat = std::string(getText("sttngs.vol.music")) + " %.0f%%";
    if (ImGui::SliderFloat("##MusicVolume", &musicVol, 0.0f, 100.0f, musicVolFormat.c_str())) {
        settings.musicVolume = musicVol * 0.01f;
        m_audio.setVolume(settings.musicVolume, Music);
        settingsChanged = true;
    }

    float sfxVol = settings.sfxVolume * 100.0f;
    std::string sfxVolFormat = std::string(getText("sttngs.vol.sfx")) + " %.0f%%";
    if (ImGui::SliderFloat("##SFXVolume", &sfxVol, 0.0f, 100.0f, sfxVolFormat.c_str())) {
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
    ImGui::Begin(getText("help"), &m_game.showHelp, flags);
    
    std::string gamemode = m_game.GetCurrentGameMode()->modeName;

    struct HelpSection {
        const char* title;
        const char* subtitle;
        std::vector<std::pair<const char*, const char*>> entries;
    };

    std::vector<HelpSection> sections;

    if (gamemode == "Sandbox") {
        sections.push_back({nullptr, nullptr, {
            {"WASD", "Move"}, 
            {"Shift", "Y+"}, {"Ctrl", "Y-"}, 
            {getText("help.key.scroll"), getText("help.action.zoom")}, 
            {getText("help.key.rclick"), getText("help.action.movecam")}, 
            {getText("help.key.lclick"), getText("help.action.applytool")}, 
            {"1-2-3", getText("help.action.tools")}
        }});
    } else if (gamemode == "PlanetBuilder") {
        sections.push_back({nullptr, nullptr, {
            {getText("help.key.lclick"), getText("help.action.applytool")},
            {getText("help.key.rclick"), getText("help.action.movecam")}, 
            {getText("help.key.scroll"), getText("help.action.zoom")}, 
            {"1-2-3", getText("help.action.tools")}
        }});
    } else if (gamemode == "SpacecraftBuilder") {
        sections.push_back({getText("help.movemode.oc"), nullptr, {
            {getText("help.key.rclick"), getText("help.action.movecam")}, 
            {getText("help.key.scroll"), getText("help.action.zoom")}, 
            {"Shift", "Y+"}, {"Ctrl", "Y-"}
        }});
        sections.push_back({getText("help.movemode.fpv"), nullptr, {
            {"WASD", getText("help.action.move")}, 
            {"Shift", getText("help.action.sprint")}, 
            {getText("help.key.lalt"), getText("help.action.cursor")},
        }});
        sections.push_back({getText("help.movemode.bp"), getText("help.hint.bp"), {
            {"W", getText("help.action.rot.up")}, 
            {"S", getText("help.action.rot.dn")}
        }});
    }

    if (sections.empty()) {
        ImGui::Text(getText("help.nohelp"));
        ImGui::End();
        return;
    }

    float maxActionWidth = 0.0f;
    float maxKeyWidth = 0.0f;
    
    for (const auto& section : sections) {
        for (const auto& entry : section.entries) {
            float actionW = ImGui::CalcTextSize(entry.second).x;
            float keyW = ImGui::CalcTextSize(entry.first).x;
            if (actionW > maxActionWidth) maxActionWidth = actionW;
            if (keyW > maxKeyWidth) maxKeyWidth = keyW;
        }
    }
    
    maxActionWidth += 40.0f; 
    maxKeyWidth += 20.0f;

    int tableFlags = ImGuiTableFlags_BordersInnerV;
    int tableId = 0;

    for (const auto& section : sections) {
        if (section.title) {
            ImGui::Spacing();
            ImGui::SeparatorText(section.title);
        }
        if (section.subtitle) {
            ImGui::TextDisabled("%s", section.subtitle);
            ImGui::Spacing();
        }

        ImGui::PushID(tableId++);
        if (ImGui::BeginTable("HelpTable", 2, tableFlags)) {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, maxKeyWidth);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, maxActionWidth);

            for (const auto& entry : section.entries) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(40, 45, 55, 255));
                ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.8f, 1.0f), " %s ", entry.first);
                
                ImGui::TableSetColumnIndex(1);
                
                float textWidth = ImGui::CalcTextSize(entry.second).x;
                float columnWidth = ImGui::GetColumnWidth();
                float cursorX = ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f;
                ImGui::SetCursorPosX(cursorX);
                
                ImGui::Text("%s", entry.second);
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    ImGui::End();
}

const char* UI::getText(const char* key) {
    auto it = m_translations.find(key);
    if (it == m_translations.end()) return key;

    const std::vector<std::string>& translations = it->second;
    size_t langIndex = static_cast<size_t>(m_language);

    if (langIndex < translations.size() && !translations[langIndex].empty())
        return translations[langIndex].c_str();

    if (!translations.empty() && !translations[0].empty())
        return translations[0].c_str();

    return key;
}