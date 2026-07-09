#pragma once

#include "gamemode.h"
#include "moveset/cameraorbit.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/components/skyboxComponent.h"
#include "engine/render/postprocess/postProcessor.h"
#include "engine/utils/logger.h"

#include "components/planetComponent.h"
#include "registries/planetRegistry.h"
#include "ui/selector.h"
#include "engine/utils/path.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <string>

class Game;

enum class PlaceState {
    None,
    Positioning,
    VelocityDrag,
};

enum class ToolMode {
    Selection,
    Reallocation,
    Velocity,
};

class SandboxMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Selector m_selector      = Selector(m_game, "user/planets", ".planet");
    Selector m_loadSelector  = Selector(m_game, "user/sandbox", ".sandbox");

    bool m_showSaveDialog = false;
    char m_saveNameBuf[128] = {};
    bool m_pendingLoad = false;

    OrbitCamera* m_orbitCamera = nullptr;
    Entity* m_target = nullptr;
    Entity* m_camera = nullptr;

    bool m_inputEnabled = true;
    bool m_topBarActive = false;
    bool m_sandboxWindowActive = false;
    bool m_saveDialogActive = false;
    bool movingEnabled = false;

    float moveSpeed = 500.0f;
    float startRadius = 2350.0f;
    
    bool drawGrid = false;
    bool drawTrails = true;
    bool m_showSandbox = true;

    bool isTransitioning = true;

    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 2.0f; 

    Vec3 m_startPosition;
    Vec3 m_startRotation;

    Entity* selectedEntity = nullptr;
    float m_selectedColor[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<Entity*> planetList;

    bool physicsPaused = false;
    bool simulationRunning = false;
    float m_massScale = 1.0f;

    struct PlanetSnapshot {
        Entity* entity = nullptr;
        Vec3 position;
        Vec3 velocity;
        Vec3 scale;
        float atmosphereThickness = 0.0f;
        bool  hasAtmosphere = false;
    };
    std::vector<PlanetSnapshot> m_simSnapshot;
    bool m_hasSnapshot = false;

    ToolMode m_toolMode = ToolMode::Selection;   // active tool

    PlaceState m_placeState = PlaceState::None;
    int m_selectedBodyIndex = -1;
    
    Vec3 m_placePosition;
    Vec3 m_placeVelocity;
    Vec2 m_startMousePos;

    Entity* m_ghostEntity = nullptr;
    float m_velocityScale = 0.5f;
    bool m_reallocatingExisting = false;
    Vec3 m_preReallocColor = Vec3(1.0f, 1.0f, 1.0f);

    bool m_prevLeftPressed = false;
    bool m_prevRightPressed = false;

    struct DestroyAnimation {
        Vec3   center;
        float  planetRadius;
        float  elapsed   = 0.0f;
        float  duration  = 0.6f;
        bool   done      = false;
        Entity* entity   = nullptr;
    };

    std::vector<DestroyAnimation> m_destroyAnims;

    std::unordered_map<Entity*, std::vector<Vec3>> m_trails;

    Entity* m_velocityTarget = nullptr; 
    Vec2   m_velDragStart;
    Vec3   m_velDragWorldOrigin;
    bool   m_velDragging = false; 
    static constexpr float kVelocityPixelScale = 0.0014f;
    static constexpr float kGravConst          = 0.02f; 
    static constexpr float kGravSoftening      = 100.0f;  // prevents singularity at close range
    static constexpr int   kMaxTrailPoints     = 300;

    bool  m_focusTransitioning = false;
    Vec3  m_focusStartPos;
    Vec3  m_focusEndPos;
    float m_focusProgress = 0.0f;
    float m_focusDuration = 0.6f;

    bool  m_followingPlanet = false;   // camera tracks selectedEntity during simulation

    PlanetRegistry m_registry;
    Entity* m_centralBody = nullptr;
    // TODO: not hardcoding central bodies
    PostProcessor* m_blackHole = nullptr;
    bool m_blackHoleActive = false;

    bool showPlanetsWindow = false;
public:
    SandboxMode(Game& game);

    void OnEnter() override;

    void OnExit() override;

    // Helpers
    Vec3 GetMouseIntersection();
    bool IsPositionValid(const Vec3& pos, float ghostRadius);
    const GravityBody* FindBodyDef(const std::string& name) const;
    void SelectPlanet(Entity* planet);

    // TODO: move this to engine or a utility class, since it's useful in general
    bool WorldToScreen(const Vec3& worldPos, ImVec2& outScreen) const;

    // Placement
    void StartPlacement(int index);
    void ConfirmPlacement();
    void CancelPlacement();

    // Planet picking
    int TryPickPlanet();

    // interaction logic per tool
    void HandlePlacementLogic();

    // Overlay
    static constexpr float kVelocityArrowWorldScale = 5.0f;
    void DrawVelocityArrow();
    void DrawAllVelocityVectors();
    void DrawSelectionHighlight();

    void UpdateFocusTransition(float dt);

    void DestroyPlanet(Entity* planet);
    void UpdateDestroyAnimations(float dt);
    void DrawDestroyAnimations();

    void StartSimulation();
    void PauseSimulation();
    void ResumeSimulation();
    void ResetSimulation();
    void StepSimulation(float dt);
    
    void DrawTrails();
    void DrawGrid(Renderer& renderer, Entity* m_camera);
    
    void Update() override;
    void LateUpdate() override;
    
    // Save/Loading
    void SaveSandbox(const std::string& name);
    void LoadSandbox(const std::string& filepath);
    void DrawSaveDialog();
    void ResetSandbox();

    void SetCentralBodyType(bool useBlackHole);
    
    void Transition(float dt);

    // UI
    void DrawUI();
    void DrawMainMenuBar();
    void DrawPlanetsWindow();
};