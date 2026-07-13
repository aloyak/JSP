#pragma once

#include "game.h"
#include "gamemode.h"

#include <algorithm>
#include <cmath>

#include "moveset/cameraorbit.h"
#include "moveset/firstperson.h"
#include "moveset/blueprint.h"

#include "registries/partsRegistry.h"

#include "ui/selector.h"

#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"
#include "engine/components/skyboxComponent.h"

enum class MoveMode {
    CameraOrbit,
    CameraFirstPerson,
    Blueprint
};

enum class OperationMode {
    Assembly,
    Blueprint
};

class SpacecraftBuilderMode : public GameMode {
private:
    Game& m_game;
    Input& m_input = m_game.GetEngine().getInput();
    UI& m_ui = m_game.GetUI();

    Entity* m_camera = nullptr;
    OrbitCamera* m_orbitCamera = nullptr;
    FirstPersonCamera* m_firstPersonCamera = nullptr;
    Blueprint* m_blueprintMode = nullptr;

    MoveMode m_moveMode = MoveMode::CameraOrbit;

    Entity* m_target = nullptr;
    Entity* m_skybox = nullptr;

    bool m_isTransitioning = false;
    MoveMode m_transitionTarget = MoveMode::CameraOrbit;
    float m_transitionElapsed = 0.0f;
    float m_transitionDuration = 0.5f;
    Vec3 m_transitionFromPos{0.0f};
    Vec3 m_transitionToPos{0.0f};
    Vec3 m_transitionFromRot{0.0f};
    Vec3 m_transitionToRot{0.0f};

    Vec3 m_firstPersonStartPos{0.0f, 3.0f, 0.0f};

    bool m_hasSavedFirstPersonPos = false;
    Vec3 m_savedFirstPersonPos{0.0f};
    Vec3 m_savedFirstPersonRot{0.0f};

    Vec3 m_targetPosBeforeBlueprint{0.0f};

    float m_orbitFov = 55.0f;
    float m_transitionFromFov = m_orbitFov;
    float m_transitionToFov = m_orbitFov;

    bool m_isSprinting = false;
    float m_sprintMultiplier = 1.5f;
    float m_baseMoveSpeed = 3.5f;

    float m_musicDelay = 2.5f;

    bool m_assemblyPanelVisible = true;

    Category m_selectedCategory = Category::FuelTank;
    Spaceship m_spaceship;

    std::vector<Part> m_parts = CreateDefaultParts();

    static constexpr Category m_categorys[] = {
        Category::Information,
        Category::Customization,
        Category::Engine,
        Category::FuelTank,
        Category::Cockpit,
        Category::Structural,
        Category::Electrical,
        Category::Utility,
    };

    // Modular placement / snapping
    struct PlacedPart {
        Entity* entity = nullptr;
        Part* partDef = nullptr;
        std::vector<bool> usedAttachments; // true = already mated to another part

        int id = -1;
        int parentId = -1;
        int parentAttachIdx = -1;
        std::vector<int> childIds;
    };
    std::vector<PlacedPart> m_placedParts;
    int m_nextPlacedPartId = 0;
    std::vector<int> m_hoveredMenuPartIds;

    Entity* m_ghostEntity = nullptr;
    Part* m_ghostPartDef = nullptr;
    bool m_ghostSnapped = false;
    Vec3 m_ghostSnapPosition{0.0f};
    Vec3 m_ghostSnapRotation{0.0f};

    bool m_prevLeftPressed = false;
    bool m_prevRightPressed = false;

    static constexpr float k_snapPixelThreshold = 28.0f;
    static constexpr float k_ghostFloatDistance = 20.0f;
    static constexpr float k_gizmoArmLength = 0.35f;
    static constexpr float k_gizmoLineWidth = 2.0f;

    float m_bottomClearance = 6.0f;

    PostProcessor* m_barrier = nullptr;
    bool m_barrierEnabled = true;

    PostProcessor* m_fog = nullptr;

    bool m_showCenterOfMass = true;
    bool m_showGeometryCenter = true;
public:
    SpacecraftBuilderMode(Game& game);

    void OnEnter() override;
    void OnExit() override;

    Vec3 ComputeDefaultFirstPersonPos() const;

    void StartTransitionTo(MoveMode mode);
    
    void drawBarrier();
    
    Vec3 RotateEuler(const Vec3& v, const Vec3& eulerDegrees) const;
    Vec3 RotationToAlignNormals(const Vec3& from, const Vec3& to, float fallbackYawDegrees) const;
    
    void StartGhostPlacement(Part& part);
    void PlacePartInstant(Part& part, Vec3 position, Vec3 rotation);
    
    // TODO: move this to engine or a utility class, since it's useful in general
    bool WorldToScreen(const Vec3& worldPos, ImVec2& outScreen) const;
    
    void DrawAttachmentPointGizmos();
    
    bool FindSnapPoint(const Vec2& mousePos, Vec3& outPos, Vec3& outRot,
    PlacedPart*& outTargetPart, int& outTargetIdx, int& outGhostIdx);
        
    void ConfirmGhostPlacement(PlacedPart* targetPart, int targetAttachIdx, int ghostAttachIdx);
    void CancelGhostPlacement();
    void HandlePartPlacement();

    void CollectDescendants(int partId, std::vector<int>& outIds) const;
    void DeletePart(int partId);
    void DeleteAllParts();
    void ApplyPartHighlight();
    
    void ShowPartTooltip(const Part& part) const;
        
    void LateUpdate() override;
        
    void updateCenterOfMass();
    void updateGeometryCenter();
        
    void Update() override;
    void showAssemblyWindow();
    
    void drawMenuBar();
    void drawInformation();
    void drawCustomization();
};