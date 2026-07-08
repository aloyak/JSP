#pragma once

#include "game.h"
#include "gamemode.h"
#include "ui/selector.h"

#include <array>
#include <memory>
#include <vector>
#include <string>

#include "moveset/cameraorbit.h"
#include "components/planetComponent.h"
#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/render/render.h"
#include "engine/render/texture.h"

#include <cmath>
#include <cstring>
#include <nlohmann/json.hpp>
#include <fstream>

enum class PlanetToolMode {
    None,
    Terrain,
    Paint,
    ColorHistory
};

class PlanetBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input;
    UI& m_ui;
    AudioManager& m_audio;

    Selector m_selector;

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;

    Entity* planet = nullptr;

    bool showProperties = true;
    bool showTools = true;

    PlanetToolMode currentTool = PlanetToolMode::None;
    bool syncToolTab = false;

    // Paint tool
    float brushSize     = 10.0f;
    float brushColor[3] = { 1.0f, 0.0f, 0.0f };
    float brushOpacity  = 1.0f;

    // Terrain tool
    float terrainBrushSize      = 150.0f; 
    float terrainBrushStrength  = 150.0f;  
    float terrainBrushFalloff   = 1.5f; 

    std::vector<std::array<float, 3>> colorHistory;

    std::shared_ptr<Texture> m_paintTexture;

    static constexpr int   k_textureSize  = 512;
    static constexpr float k_brushUVScale = 1.0f / 1000.0f;

    static constexpr float k_uOffset = 0.75f;
    static constexpr bool  k_swapXZ  = true;

    bool m_isHoveringPlanet = false;
    Vec3 m_hoverWorldPos    = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 m_hoverLocalPos    = Vec3(0.0f, 0.0f, 0.0f);

    bool m_leftButtonWasDown = false;
    bool m_leftButtonClicked = false;

    bool m_rightButtonWasDown   = false;
    bool m_rightClickIsOrbiting = false;

    AudioManager::SoundHandle m_brushSoundHandle = 0;
    bool m_isActivelyEditing = false;
    bool m_brushSoundEnabled = false;

    float toLinear(float c) const;
    
    void PaintUVGradient();
    void UpdateMouseIntersection();
    void GetHoverUV(float& outU, float& outV) const;
    void StartBrushSound();
    void StopBrushSound();
    void TryPaint();
    void DrawBrushIndicator();

    void TryTerrain();
    void ResetTerrain();
    
    void Save();
    void Load(const std::string& filepath);
    
    void DrawPropertiesWindow();
    void DrawToolsWindow();

public:
    PlanetBuilderMode(Game& game);

    void OnEnter() override;
    void OnExit() override;
    void Update() override;
    void LateUpdate() override;
};