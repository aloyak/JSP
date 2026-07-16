#include "gamemodes/spacecraftbuilder.h"

#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>

#include "engine/utils/logger.h"

SpacecraftBuilderMode::SpacecraftBuilderMode(Game& game)
    : GameMode("SpacecraftBuilder", "assets/scenes/assembly.scene")
    , m_game(game) {}

void SpacecraftBuilderMode::OnEnter() {
    m_game.timeScale = 1.0f;

    m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
    m_camera->addComponent<CameraComponent>(m_orbitFov, 1600.0f / 900.0f, 0.1f, 1000.0f);
    m_game.GetEngine().setActiveCamera(m_camera);

    m_game.GetEngine().getRenderer().setMinimumAmbientLight(0.2f);
    
    float distance = 20.0f;
    float offset = distance / std::sqrt(3.0f);
    
    m_camera->transform.position = Vec3(offset, offset, offset);
    
    Vec3 orientation = Vec3(-35.264f, 45.0f, 0.0f);

    m_camera->transform.rotation = orientation;

    m_target = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Target");
    m_target->transform.position = Vec3(0.0f, 5.0f, 0.0f);

    m_skybox = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Skybox").get();
    
    m_orbitCamera = new OrbitCamera(m_camera);
    m_orbitCamera->SetTarget(m_target, distance);
    m_orbitCamera->SyncFromCurrentPosition();
    m_orbitCamera->SetMaxRadius(25.0f);
    m_orbitCamera->SetHeightLimits(2.0f, 300.0f);
    m_orbitCamera->ApplyPosition();

    m_firstPersonCamera = new FirstPersonCamera(m_camera);
    m_baseMoveSpeed = m_firstPersonCamera->getMoveSpeed();
    
    m_fog = &m_game.GetEngine().getRenderer().addPostProcessor(
        "assets/shaders/default_vert.glsl", "assets/shaders/fog_frag.glsl"
    );

    m_barrier = &m_game.GetEngine().getRenderer().addPostProcessor(
        "assets/shaders/default_vert.glsl", "assets/shaders/barrier_frag.glsl"
    );
}

void SpacecraftBuilderMode::OnExit() {
    m_game.GetEngine().getRenderer().removePostProcessor(m_barrier);
    m_game.GetEngine().getRenderer().removePostProcessor(m_fog);
    m_game.GetAudioManager().stopMusic(1.5f);
    CancelGhostPlacement();
    delete m_orbitCamera;
    delete m_firstPersonCamera;
    delete m_blueprintMode;
}

Vec3 SpacecraftBuilderMode::ComputeDefaultFirstPersonPos() const {
    Vec3 center = m_target->transform.position;
    return Vec3(center.x, m_firstPersonStartPos.y, center.z + 8.0f);
}

void SpacecraftBuilderMode::StartTransitionTo(MoveMode mode) {
    if (m_moveMode == mode || m_transitionTarget == mode) return;
    if (mode == MoveMode::CameraFirstPerson || mode == MoveMode::Blueprint) CancelGhostPlacement();

    m_transitionFromPos = m_camera->transform.position;
    m_transitionFromRot = m_camera->transform.rotation;
    m_transitionFromFov = m_camera->getComponent<CameraComponent>()->getFOV();

    if (m_moveMode == MoveMode::CameraFirstPerson) {
        m_savedFirstPersonPos = m_transitionFromPos;
        m_savedFirstPersonRot = m_transitionFromRot;
        m_hasSavedFirstPersonPos = true;
    }

    if (m_moveMode == MoveMode::Blueprint && mode != MoveMode::Blueprint) {
        delete m_blueprintMode;
        m_blueprintMode = nullptr;
        m_target->transform.position = m_targetPosBeforeBlueprint;
    }

    if (mode == MoveMode::CameraFirstPerson) {
        if (m_hasSavedFirstPersonPos) {
            m_transitionToPos = m_savedFirstPersonPos;
            m_transitionToRot = m_savedFirstPersonRot;
        } else {
            m_transitionToPos = ComputeDefaultFirstPersonPos();

            Vec3 previousPos = m_camera->transform.position;
            m_camera->transform.position = m_transitionToPos;
            m_camera->getComponent<CameraComponent>()->lookAt(*m_target);
            m_transitionToRot = m_camera->transform.rotation;
            m_camera->transform.position = previousPos; // restore; the transition will animate to it
        }
        m_transitionToFov = m_firstPersonCamera->getFov();
    } else if (mode == MoveMode::Blueprint) {
        m_targetPosBeforeBlueprint = m_target->transform.position;
        m_target->transform.position = m_spaceship.geometryCenter;

        ImGuiIO& blueprintIO = ImGui::GetIO();
        float blueprintAspect = (blueprintIO.DisplaySize.y > 0.0f)
            ? (blueprintIO.DisplaySize.x / blueprintIO.DisplaySize.y) : 1.0f;

        float blueprintDistance = Blueprint::ComputeCameraDistance(
            m_spaceship, m_target->transform.position.y, m_orbitFov, blueprintAspect,
            Blueprint::kTopClearance, Blueprint::kBottomClearance);

        int presetIndex = Blueprint::nearestPresetIndex(m_target->transform.position, m_transitionFromPos);
        m_transitionToPos = Blueprint::presetPosition(m_target->transform.position, presetIndex, blueprintDistance);
        m_transitionToRot = m_camera->transform.rotation; // blueprint drives via lookAt, not rotation
        m_transitionToFov = m_orbitFov;
        m_barrierEnabled = false;
    } else {
        m_transitionToPos = m_orbitCamera->GetComputedPosition();
        m_transitionToRot = m_camera->transform.rotation; // orbit drives via lookAt, not rotation
        m_transitionToFov = m_orbitFov;
        m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("Platform")->getComponent<RenderComponent>()->isEnabled = true; 
        m_barrierEnabled = true;
    }

    m_transitionTarget = mode;
    m_transitionElapsed = 0.0f;
    m_isTransitioning = true;
}

void SpacecraftBuilderMode::Update() {
    if (m_musicDelay > 0.0f) {
        m_musicDelay -= m_game.GetEngine().getDeltaTime();
        if (m_musicDelay <= 0.0f) {
            m_game.GetAudioManager().playMusic("assets/audio/terminal_pt2.ogg", 10.0f, true, -0.5f);
        }
    }

    float deltaTime = m_game.GetEngine().getDeltaTime();

    if (m_isTransitioning) {
        m_transitionElapsed += deltaTime;
        float t = std::clamp(m_transitionElapsed / m_transitionDuration, 0.0f, 1.0f);
        float s = t * t * (3.0f - 2.0f * t);

        m_camera->transform.position = m_transitionFromPos + (m_transitionToPos - m_transitionFromPos) * s;
        m_camera->getComponent<CameraComponent>()->setFOV(m_transitionFromFov + (m_transitionToFov - m_transitionFromFov) * s);

        if (m_transitionTarget == MoveMode::CameraFirstPerson) {
            m_camera->transform.rotation = m_transitionFromRot + (m_transitionToRot - m_transitionFromRot) * s;
        } else {
            m_camera->getComponent<CameraComponent>()->lookAt(*m_target);

            float fromRoll = (m_moveMode == MoveMode::Blueprint) ? Blueprint::kSideRollDegrees : 0.0f;
            float toRoll = (m_transitionTarget == MoveMode::Blueprint) ? Blueprint::kSideRollDegrees : 0.0f;
            m_camera->transform.rotation.z = fromRoll + (toRoll - fromRoll) * s;
        }

        if (t >= 1.0f) {
            m_isTransitioning = false;
            m_moveMode = m_transitionTarget;
            m_camera->getComponent<CameraComponent>()->setFOV(m_transitionToFov);

            if (m_moveMode == MoveMode::CameraFirstPerson) {
                m_firstPersonCamera->SetPlayerPosition(m_transitionToPos);
            } else if (m_moveMode == MoveMode::Blueprint) {
                m_blueprintMode = new Blueprint(m_camera, m_target, m_game, m_spaceship);
            } else {
                m_orbitCamera->ApplyPosition();
            }
        }
    } else if (m_moveMode == MoveMode::CameraOrbit) {
        if (m_input.isMouseButtonPressed(MOUSE_RIGHT)) {
            m_input.setCursorMode(true);
            m_orbitCamera->Update(&m_input, deltaTime);
            m_orbitCamera->ApplyScroll(&m_input);
        } else {
            m_input.setCursorMode(false);
        }
    } else if (m_moveMode == MoveMode::CameraFirstPerson) {
        if (!m_input.isKeyDown(KEY_LALT)) m_input.setCursorMode(true);
        else m_input.setCursorMode(false);
    
        auto& settings = m_game.settingsManager.Get();
        m_firstPersonCamera->setSensitivity(settings.mouseSens);
        m_firstPersonCamera->setHeadBobAmplitude(settings.headbobIntensity);

        if (m_input.isKeyDown(KEY_LSHIFT)) {
            m_isSprinting = true;
            m_firstPersonCamera->setMoveSpeed(m_baseMoveSpeed * m_sprintMultiplier);
        } else {
            m_isSprinting = false;
            m_firstPersonCamera->setMoveSpeed(m_baseMoveSpeed);
        }

        m_firstPersonCamera->Update(&m_input, deltaTime, m_game.GetEngine().getTime());
    } else if (m_moveMode == MoveMode::Blueprint) {
        m_input.setCursorMode(false);
        if (m_blueprintMode) m_blueprintMode->update(deltaTime);
    }

    bool targetHeightChanged = false;
    if (m_input.isKeyDown(KEY_LSHIFT)) {
        m_target->transform.position.y += 10.0f * deltaTime;
        targetHeightChanged = true;
    }
    if (m_input.isKeyDown(KEY_LCTRL)) {
        m_target->transform.position.y -= 10.0f * deltaTime;
        targetHeightChanged = true;
    }

    m_target->transform.position.y = std::clamp(m_target->transform.position.y, 0.0f, 60.0f);
    m_camera->transform.position.y = std::clamp(m_camera->transform.position.y, 2.0f, 230.0f);

    if (targetHeightChanged && !m_isTransitioning && m_moveMode == MoveMode::CameraOrbit) {
        m_orbitCamera->ApplyPosition();
    }

    if (m_moveMode == MoveMode::CameraFirstPerson) {
        Vec3& pos = m_camera->transform.position;
        pos.x = std::clamp(pos.x, -20.0f, 20.0f);
        pos.z = std::clamp(pos.z, -20.0f, 20.0f);
    }

    if (!m_isTransitioning && m_moveMode == MoveMode::CameraOrbit) HandlePartPlacement();

    if (!m_placedParts.empty()) {
        float lowestY = 1e9f;
        float highestY = -1e9f;
        for (const auto& placed : m_placedParts) {
            if (!placed.entity) continue;
            float y = placed.entity->transform.position.y;
            lowestY = std::min(lowestY, y);
            highestY = std::max(highestY, y);
        }
        if (lowestY < m_bottomClearance) {
            float diff = m_bottomClearance - lowestY;
            for (auto& placed : m_placedParts) {
                if (placed.entity) {
                    placed.entity->transform.position.y += diff;
                }
            }
            lowestY += diff;
            highestY += diff;
            if (!m_isTransitioning && m_moveMode == MoveMode::CameraOrbit) {
                m_orbitCamera->ApplyPosition();
            }
        }

        m_spaceship.lowestPartY = lowestY;
        m_spaceship.highestPartY = highestY;
    }

    showAssemblyWindow();
    drawBarrier();

    if (m_fog) {
        m_fog->setVec3("u_cameraPos", m_camera->transform.position);
        m_fog->setMat4("u_invView", m_camera->getComponent<CameraComponent>()->getCamera().getInvViewMatrix(m_camera->transform));
        m_fog->setMat4("u_invProj", m_camera->getComponent<CameraComponent>()->getCamera().getInvProjMatrix());

        m_fog->setFloat("u_distance", 100.0f);
        m_fog->setFloat("u_density", 0.04f);
        m_fog->setFloat("u_maxFog", 1.0f);
        m_fog->setFloat("u_skyFalloff", 1.0f);
        m_fog->setVec3("u_color", Vec3(0.5f, 0.7f, 0.8f));
    }
}

void SpacecraftBuilderMode::drawBarrier() {
    if (!m_barrierEnabled) {
        m_barrier->setBool("u_enabled", false);
        return;
    } else m_barrier->setBool("u_enabled", true);
    
    if (m_barrier) {
        m_barrier->setVec3("u_cameraPos", m_camera->transform.position);
        m_barrier->setMat4("u_invView", m_camera->getComponent<CameraComponent>()->getCamera().getInvViewMatrix(m_camera->transform));
        m_barrier->setMat4("u_invProj", m_camera->getComponent<CameraComponent>()->getCamera().getInvProjMatrix());

        m_barrier->setFloat("u_near", m_camera->getComponent<CameraComponent>()->getNear());
        m_barrier->setFloat("u_far", m_camera->getComponent<CameraComponent>()->getFar());
        m_barrier->setFloat("u_time", m_game.GetEngine().getTime());

        m_barrier->setVec3("u_cageCenter", Vec3(0.0f, 0.5f, 0.0f));
        m_barrier->setFloat("u_cageSize", 10.5f);
        m_barrier->setFloat("u_barrierHeight", 2.0f);
    }
}

Vec3 SpacecraftBuilderMode::RotateEuler(const Vec3& v, const Vec3& eulerDegrees) const {
    const float pi = 3.1415926535f;
    float rx = eulerDegrees.x * (pi / 180.0f);
    float ry = eulerDegrees.y * (pi / 180.0f);
    float rz = eulerDegrees.z * (pi / 180.0f);

    // Yaw (Y)
    Vec3 p(v.x * std::cos(ry) + v.z * std::sin(ry),
           v.y,
           -v.x * std::sin(ry) + v.z * std::cos(ry));

    // Pitch (X)
    Vec3 q(p.x,
           p.y * std::cos(rx) - p.z * std::sin(rx),
           p.y * std::sin(rx) + p.z * std::cos(rx));

    // Roll (Z)
    Vec3 r(q.x * std::cos(rz) - q.y * std::sin(rz),
           q.x * std::sin(rz) + q.y * std::cos(rz),
           q.z);

    return r;
}

Vec3 SpacecraftBuilderMode::RotationToAlignNormals(const Vec3& from, const Vec3& to, float fallbackYawDegrees) const {
    const float epsilon = 1e-6f;

    bool toIsVertical = (to.x * to.x + to.z * to.z) < epsilon;
    if (toIsVertical) {
        return Vec3(0.0f, fallbackYawDegrees, 0.0f);
    }

    bool fromIsVertical = (from.x * from.x + from.z * from.z) < epsilon;
    if (!fromIsVertical) {
        float angleFrom = std::atan2(from.z, from.x);
        float angleTo = std::atan2(to.z, to.x);
        float yaw = (angleFrom - angleTo) * (180.0f / 3.1415926535f);
        return Vec3(0.0f, yaw, 0.0f);
    }

    Vec3 f = from.normalize();
    Vec3 t = to.normalize();
    float cosAngle = std::max(-1.0f, std::min(1.0f, dot(f, t)));

    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;

    if (cosAngle > 1.0f - epsilon) {
        m00 = 1.0f; m01 = 0.0f; m02 = 0.0f;
        m10 = 0.0f; m11 = 1.0f; m12 = 0.0f;
        m20 = 0.0f; m21 = 0.0f; m22 = 1.0f;
    } else if (cosAngle < -1.0f + epsilon) {
        Vec3 axis = cross(f, Vec3(0.0f, 1.0f, 0.0f));
        if (axis.length() < epsilon) axis = cross(f, Vec3(1.0f, 0.0f, 0.0f));
        axis = axis.normalize();

        float kx = axis.x, ky = axis.y, kz = axis.z;
        m00 = 2.0f * kx * kx - 1.0f; m01 = 2.0f * kx * ky;        m02 = 2.0f * kx * kz;
        m10 = 2.0f * kx * ky;        m11 = 2.0f * ky * ky - 1.0f; m12 = 2.0f * ky * kz;
        m20 = 2.0f * kx * kz;        m21 = 2.0f * ky * kz;        m22 = 2.0f * kz * kz - 1.0f;
    } else {
        Vec3 axis = cross(f, t).normalize();
        float angle = std::acos(cosAngle);
        float s = std::sin(angle);
        float c = cosAngle;
        float kx = axis.x, ky = axis.y, kz = axis.z;

        m00 = c + kx * kx * (1.0f - c);
        m01 = kx * ky * (1.0f - c) - kz * s;
        m02 = kx * kz * (1.0f - c) + ky * s;

        m10 = ky * kx * (1.0f - c) + kz * s;
        m11 = c + ky * ky * (1.0f - c);
        m12 = ky * kz * (1.0f - c) - kx * s;

        m20 = kz * kx * (1.0f - c) - ky * s;
        m21 = kz * ky * (1.0f - c) + kx * s;
        m22 = c + kz * kz * (1.0f - c);
    }

    float sinPitch = std::max(-1.0f, std::min(1.0f, m21));
    float pitch = std::asin(sinPitch);
    float cosPitch = std::cos(pitch);

    float yaw, roll;
    if (std::abs(cosPitch) > epsilon) {
        yaw  = std::atan2(-m20, m22);
        roll = std::atan2(-m01, m11);
    } else {
        roll = 0.0f;
        yaw  = std::atan2(m02, m00);
    }

    const float toDegrees = 180.0f / 3.1415926535f;
    return Vec3(pitch * toDegrees, yaw * toDegrees, roll * toDegrees);
}

void SpacecraftBuilderMode::StartGhostPlacement(Part& part) {
    CancelGhostPlacement();

    if (m_placedParts.empty()) {
        PlacePartInstant(part, Vec3(0.0f, 10.0f, 0.0f), Vec3(0.0f));
        return;
    }

    m_ghostPartDef = &part;
    m_ghostEntity = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity(part.name);
    m_ghostEntity->addComponent<RenderComponent>(part.modelPath);
    m_ghostSnapped = false;
}

void SpacecraftBuilderMode::PlacePartInstant(Part& part, Vec3 position, Vec3 rotation) {
    Entity* entity = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity(part.name);
    entity->addComponent<RenderComponent>(part.modelPath);
    entity->transform.position = position;
    entity->transform.rotation = rotation;

    m_game.GetAudioManager().playSound("assets/audio/drill.ogg", AudioContext::SFX, false, -0.5f);

    PlacedPart placed;
    placed.entity = entity;
    placed.partDef = &part;
    placed.usedAttachments = std::vector<bool>(part.attachmentPoints.size(), false);
    placed.id = m_nextPlacedPartId++;
    m_placedParts.push_back(placed);
    m_spaceship.parts.push_back(part);

    m_spaceship.mass += part.mass;
    m_spaceship.thrust += part.thrust;
    m_spaceship.fuelCapacity += part.fuelCapacity;
}

// TODO: move this to engine or a utility class, since it's useful in general
bool SpacecraftBuilderMode::WorldToScreen(const Vec3& worldPos, ImVec2& outScreen) const {
    Vec2 winSize  = m_game.GetEngine().getWindow().getSize();
    auto* camComp = m_camera->getComponent<CameraComponent>();
    if (!camComp) return false;
    Mat4 proj; camComp->getCamera().getProjectionMatrix(proj);

    Vec3 toPoint  = worldPos - m_camera->transform.position;
    Vec3 lookDir  = (m_target->transform.position - m_camera->transform.position).normalize();
    Vec3 worldUp(0.0f, 1.0f, 0.0f);
    Vec3 rightDir = cross(lookDir, worldUp).normalize();
    Vec3 upDir    = cross(rightDir, lookDir).normalize();

    float depth = dot(toPoint, lookDir);
    if (depth <= 0.0f) return false;

    float projX = dot(toPoint, rightDir) / depth * proj[0][0];
    float projY = dot(toPoint, upDir)    / depth * proj[1][1];

    outScreen.x = (projX + 1.0f) * 0.5f * winSize.x;
    outScreen.y = (1.0f - projY) * 0.5f * winSize.y;
    return true;
}

void SpacecraftBuilderMode::DrawAttachmentPointGizmos() {
    if (!m_ghostEntity || !m_ghostPartDef) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    for (auto& placed : m_placedParts) {
        if (!placed.entity || !placed.partDef) continue;

        for (size_t idx = 0; idx < placed.partDef->attachmentPoints.size(); ++idx) {
            if (idx < placed.usedAttachments.size() && placed.usedAttachments[idx]) continue; // already mated

            auto& attach = placed.partDef->attachmentPoints[idx];
            Vec3 worldPos = placed.entity->transform.position +
                RotateEuler(attach.position, placed.entity->transform.rotation);

            ImVec2 screenPos;
            if (!WorldToScreen(worldPos, screenPos)) continue;

            bool allowed = attach.Allows(m_ghostPartDef->attachTag);
            if (allowed) {
                // usable point: bright and prominent, this is what you're aiming for
                dl->AddCircleFilled(screenPos, 5.0f, IM_COL32(100, 255, 200, 230));
                dl->AddCircle(screenPos, 7.0f, IM_COL32(100, 255, 200, 120), 16, 1.0f);
            } else {
                // incompatible point: faint grey reference only, not a candidate to snap to
                dl->AddCircleFilled(screenPos, 4.5f, IM_COL32(150, 150, 150, 50));
                dl->AddCircle(screenPos, 6.0f, IM_COL32(150, 150, 150, 35), 12, 1.0f);
            }
        }
    }
}

bool SpacecraftBuilderMode::FindSnapPoint(const Vec2& mousePos, Vec3& outPos, Vec3& outRot,
                    PlacedPart*& outTargetPart, int& outTargetIdx, int& outGhostIdx) {
    if (!m_ghostPartDef) return false;

    float bestPixelDist = k_snapPixelThreshold;
    bool found = false;
    Vec3 bestPos{0.0f};
    Vec3 bestRot{0.0f};
    PlacedPart* bestTargetPart = nullptr;
    int bestTargetIdx = -1;
    int bestGhostIdx = -1;

    for (auto& placed : m_placedParts) {
        if (!placed.entity || !placed.partDef) continue;

        for (size_t idx = 0; idx < placed.partDef->attachmentPoints.size(); ++idx) {
            if (idx < placed.usedAttachments.size() && placed.usedAttachments[idx]) continue;

            auto& attach = placed.partDef->attachmentPoints[idx];
            if (!attach.Allows(m_ghostPartDef->attachTag)) continue; // incompatible: not a valid snap target

            Vec3 worldPos = placed.entity->transform.position +
                RotateEuler(attach.position, placed.entity->transform.rotation);
            Vec3 worldNormal = RotateEuler(attach.normal, placed.entity->transform.rotation).normalize();

            ImVec2 screenPos;
            if (!WorldToScreen(worldPos, screenPos)) continue;

            float dx = screenPos.x - mousePos.x;
            float dy = screenPos.y - mousePos.y;
            float pixelDist = std::sqrt(dx * dx + dy * dy);
            if (pixelDist >= bestPixelDist) continue;

            Vec3 snapRotation = placed.entity->transform.rotation; // fallback: ghost has no attachment points at all
            Vec3 snapPosition = worldPos;
            int matedGhostIdx = -1;

            if (!m_ghostPartDef->attachmentPoints.empty()) {
                Vec3 desiredDir = worldNormal * -1.0f;

                float bestDot = -1e9f;
                for (size_t g = 0; g < m_ghostPartDef->attachmentPoints.size(); ++g) {
                    auto& ghostAttach = m_ghostPartDef->attachmentPoints[g];
                    if (!ghostAttach.Allows(placed.partDef->attachTag)) continue;

                    float d = dot(ghostAttach.normal, desiredDir);
                    if (d > bestDot) { bestDot = d; matedGhostIdx = static_cast<int>(g); }
                }

                if (matedGhostIdx < 0) continue; // ghost has no point that could accept this parent

                auto& ghostAttach = m_ghostPartDef->attachmentPoints[matedGhostIdx];

                snapRotation = RotationToAlignNormals(
                    ghostAttach.normal, desiredDir, placed.entity->transform.rotation.y);

                Vec3 ghostOffset = RotateEuler(ghostAttach.position, snapRotation);
                snapPosition = worldPos - ghostOffset;
            }

            bestPixelDist = pixelDist;
            found = true;
            bestPos = snapPosition;
            bestRot = snapRotation;
            bestTargetPart = &placed;
            bestTargetIdx = static_cast<int>(idx);
            bestGhostIdx = matedGhostIdx;
        }
    }

    if (found) {
        outPos = bestPos;
        outRot = bestRot;
        outTargetPart = bestTargetPart;
        outTargetIdx = bestTargetIdx;
        outGhostIdx = bestGhostIdx;
    }
    return found;
}

void SpacecraftBuilderMode::ConfirmGhostPlacement(PlacedPart* targetPart, int targetAttachIdx, int ghostAttachIdx) {
    if (!m_ghostEntity || !m_ghostPartDef || !m_ghostSnapped) return;

    m_ghostEntity->transform.position = m_ghostSnapPosition;
    m_ghostEntity->transform.rotation = m_ghostSnapRotation;

    m_game.GetAudioManager().playSound("assets/audio/drill.ogg", AudioContext::SFX);

    auto* rc = m_ghostEntity->getComponent<RenderComponent>();
    if (rc) rc->setBaseColor(Vec3(1.0f, 1.0f, 1.0f));

    if (targetPart && targetAttachIdx >= 0 &&
        targetAttachIdx < static_cast<int>(targetPart->usedAttachments.size())) {
        targetPart->usedAttachments[targetAttachIdx] = true;
    }

    std::vector<bool> ghostUsed(m_ghostPartDef->attachmentPoints.size(), false);
    if (ghostAttachIdx >= 0 && ghostAttachIdx < static_cast<int>(ghostUsed.size())) {
        ghostUsed[ghostAttachIdx] = true;
    }

    int newId = m_nextPlacedPartId++;
    if (targetPart) targetPart->childIds.push_back(newId);

    PlacedPart placed;
    placed.entity = m_ghostEntity;
    placed.partDef = m_ghostPartDef;
    placed.usedAttachments = ghostUsed;
    placed.id = newId;
    placed.parentId = targetPart ? targetPart->id : -1;
    placed.parentAttachIdx = targetAttachIdx;
    m_placedParts.push_back(placed);
    m_spaceship.parts.push_back(*m_ghostPartDef);

    m_spaceship.mass += m_ghostPartDef->mass;
    m_spaceship.thrust += m_ghostPartDef->thrust;
    m_spaceship.fuelCapacity += m_ghostPartDef->fuelCapacity;

    m_ghostEntity = nullptr;
    m_ghostPartDef = nullptr;
    m_ghostSnapped = false;
}

void SpacecraftBuilderMode::CancelGhostPlacement() {
    if (m_ghostEntity) {
        m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(m_ghostEntity);
        m_ghostEntity = nullptr;
    }
    m_ghostPartDef = nullptr;
    m_ghostSnapped = false;
}

void SpacecraftBuilderMode::CollectDescendants(int partId, std::vector<int>& outIds) const {
    // Gather this part plus every descendant (parts mated onto it, transitively).
    std::vector<int> pending{partId};
    while (!pending.empty()) {
        int id = pending.back();
        pending.pop_back();
        outIds.push_back(id);

        for (auto& p : m_placedParts) {
            if (p.id == id) {
                for (int childId : p.childIds) pending.push_back(childId);
                break;
            }
        }
    }
}

void SpacecraftBuilderMode::DeletePart(int partId) {
    if (partId < 0) return;

    std::vector<int> toDelete;
    CollectDescendants(partId, toDelete);

    for (auto& p : m_placedParts) {
        if (p.id != partId || p.parentId < 0) continue;

        for (auto& parentCandidate : m_placedParts) {
            if (parentCandidate.id != p.parentId) continue;

            if (p.parentAttachIdx >= 0 &&
                p.parentAttachIdx < static_cast<int>(parentCandidate.usedAttachments.size())) {
                parentCandidate.usedAttachments[p.parentAttachIdx] = false;
            }

            auto& siblings = parentCandidate.childIds;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), partId), siblings.end());
            break;
        }
        break;
    }

    for (size_t i = 0; i < m_placedParts.size();) {
        bool remove = std::find(toDelete.begin(), toDelete.end(), m_placedParts[i].id) != toDelete.end();
        if (!remove) { ++i; continue; }

        PlacedPart& p = m_placedParts[i];
        if (p.partDef) {
            m_spaceship.mass -= p.partDef->mass;
            m_spaceship.thrust -= p.partDef->thrust;
            m_spaceship.fuelCapacity -= p.partDef->fuelCapacity;
        }
        if (p.entity) {
            m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(p.entity);
        }

        m_placedParts.erase(m_placedParts.begin() + i);
        if (i < m_spaceship.parts.size()) m_spaceship.parts.erase(m_spaceship.parts.begin() + i);
    }
}

void SpacecraftBuilderMode::DeleteAllParts() {
    CancelGhostPlacement();

    for (auto& p : m_placedParts) {
        if (p.entity) m_game.GetEngine().getSceneManager().getActiveScene()->destroyEntity(p.entity);
    }

    m_placedParts.clear();
    m_spaceship.parts.clear();
    m_spaceship.mass = 0.0f;
    m_spaceship.thrust = 0.0f;
    m_spaceship.fuelCapacity = 0.0f;
}

void SpacecraftBuilderMode::HandlePartPlacement() {
    bool leftPressed = m_input.isMouseButtonPressed(MOUSE_LEFT);
    bool rightPressed = m_input.isMouseButtonPressed(MOUSE_RIGHT);
    bool leftJustPressed = leftPressed && !m_prevLeftPressed;
    bool rightJustPressed = rightPressed && !m_prevRightPressed;
    m_prevLeftPressed = leftPressed;
    m_prevRightPressed = rightPressed;

    if (!m_ghostEntity || !m_ghostPartDef) return;

    auto* camComp = m_camera->getComponent<CameraComponent>();
    if (!camComp) return;

    Vec2 mousePos = m_input.getMousePos();
    Ray ray = m_game.GetEngine().getRenderer().pickRay(
        mousePos.x, mousePos.y, camComp->getCamera(), m_camera->transform);

    Vec3 snapPos{0.0f};
    Vec3 snapRot{0.0f};
    PlacedPart* targetPart = nullptr;
    int targetIdx = -1;
    int ghostIdx = -1;
    bool found = FindSnapPoint(mousePos, snapPos, snapRot, targetPart, targetIdx, ghostIdx);

    m_ghostSnapped = found;
    if (found) {
        m_ghostSnapPosition = snapPos;
        m_ghostSnapRotation = snapRot;
        m_ghostEntity->transform.position = snapPos;
        m_ghostEntity->transform.rotation = snapRot;
    } else {
        m_ghostEntity->transform.position = ray.origin + ray.direction * k_ghostFloatDistance;
    }

    float pulse = std::sin(m_game.GetEngine().getTime() * 5.0f) * 0.25f + 0.5f;
    auto* rc = m_ghostEntity->getComponent<RenderComponent>();
    if (rc) {
        rc->setBaseColor(found ? Vec3(0.5f, 1.0f, 0.5f) * pulse : Vec3(1.0f, 0.6f, 0.3f) * pulse);
    }

    if (ImGui::GetIO().WantCaptureMouse) return;

    if (leftJustPressed && found) { ConfirmGhostPlacement(targetPart, targetIdx, ghostIdx); return; }
    if (rightJustPressed)         { CancelGhostPlacement();  return; }
}

void SpacecraftBuilderMode::LateUpdate() {
    updateCenterOfMass();
    updateGeometryCenter();

    if (!m_isTransitioning && m_moveMode == MoveMode::CameraOrbit) {
        DrawAttachmentPointGizmos();
    }

    drawMenuBar();
    ApplyPartHighlight();

    if (m_selector.isOpen()) {
        m_selector.Draw();
    }

    std::string selectedFile = m_selector.ConsumeSelection();
    if (!selectedFile.empty()) {
        Load(selectedFile);
    }

    // shortcuts
    if (!m_isTransitioning && m_input.isKeyPressed(KEY_1)) StartTransitionTo(MoveMode::CameraOrbit);
    if (!m_isTransitioning && m_input.isKeyPressed(KEY_2)) StartTransitionTo(MoveMode::CameraFirstPerson);
}

void SpacecraftBuilderMode::drawMenuBar() {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    if (ImGui::BeginMainMenuBar()) {
        if (m_ui.beginMenu("tm.game")) {
            if (m_ui.menuItem("tm.mm")) m_ui.loadMainMenu();
            if (m_ui.menuItem("tm.settings"))  m_game.showSettings = !m_game.showSettings;
            if (m_ui.menuItem("tm.help")) { m_game.showHelp = !m_game.showHelp; }
            if (m_ui.menuItem("tm.quit")) m_game.GetEngine().stop();
            ImGui::EndMenu();
        }
        if (m_ui.beginMenu("scb")) {
            if (m_ui.menuItem("tm.scb.new")) m_game.SetGameMode(std::make_unique<SpacecraftBuilderMode>(m_game), true, true);
            if (m_ui.menuItem("tm.scb.save")) { Save(); }
            if (m_ui.menuItem("tm.scb.load")) { m_selector.toggleOpen(); }
            ImGui::EndMenu();
        }
        bool wasTransitioning = m_isTransitioning;
        if (wasTransitioning) ImGui::BeginDisabled();
        if (m_ui.beginMenu("tm.view")) {
            if (m_ui.menuItem("scb.ocv", "1")) StartTransitionTo(MoveMode::CameraOrbit);
            if (m_ui.menuItem("scb.fpv", "2")) StartTransitionTo(MoveMode::CameraFirstPerson);
            
            bool isBlueprintDisabled = m_spaceship.parts.empty();
            
            if (isBlueprintDisabled) ImGui::BeginDisabled();
            if (m_ui.menuItem("scb.blueprintview")) StartTransitionTo(MoveMode::Blueprint);
            if (isBlueprintDisabled) ImGui::EndDisabled();

            if (isBlueprintDisabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip(m_ui.getText("scb.addpart"));
            ImGui::Separator();
            if (m_ui.menuItem("scb.view.com")) m_showCenterOfMass = !m_showCenterOfMass;
            if (m_ui.menuItem("scb.view.cog")) m_showGeometryCenter = !m_showGeometryCenter;
            ImGui::EndMenu();
        }
        if (wasTransitioning) ImGui::EndDisabled();
        if (m_moveMode != MoveMode::CameraOrbit) ImGui::BeginDisabled();
        m_hoveredMenuPartIds.clear();
        if (m_ui.beginMenu("scb.parts")) {
            if (m_ui.menuItem("scb.deleteall")) DeleteAllParts();
            ImGui::Separator();

            std::vector<PlacedPart*> sortedParts;
            sortedParts.reserve(m_placedParts.size());
            for (auto& p : m_placedParts) sortedParts.push_back(&p);
            std::sort(sortedParts.begin(), sortedParts.end(), [](PlacedPart* a, PlacedPart* b) {
                return static_cast<int>(a->partDef->category) < static_cast<int>(b->partDef->category);
            });

            int deleteRequestId = -1;

            for (PlacedPart* p : sortedParts) {
                if (!p->partDef) continue;

                ImGui::PushID(p->id);

                const float rowHeight = ImGui::GetFrameHeight();
                const char* name = p->partDef->name.c_str();
                const char* category = CategoryToString(p->partDef->category);

                const float leftPad = 4.0f;
                const float nameCatGap = 10.0f;
                const float rightPad = 6.0f;

                ImVec2 nameSize = ImGui::CalcTextSize(name);
                ImVec2 catSize = ImGui::CalcTextSize(category);

                const float labelWidth = leftPad + nameSize.x + nameCatGap + catSize.x + rightPad;

                float buttonWidth = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                float contentMaxX = ImGui::GetWindowContentRegionMax().x;
                float buttonPosX = contentMaxX - buttonWidth;
                
                if (buttonPosX < labelWidth + ImGui::GetStyle().ItemSpacing.x) {
                    buttonPosX = labelWidth + ImGui::GetStyle().ItemSpacing.x;
                }

                ImVec2 rowStart = ImGui::GetCursorScreenPos();

                ImGui::InvisibleButton("##partrow", ImVec2(labelWidth, rowHeight));
                bool rowHovered = ImGui::IsItemHovered();
                if (rowHovered) m_hoveredMenuPartIds = {p->id};
                ShowPartTooltip(*p->partDef);

                ImDrawList* dl = ImGui::GetWindowDrawList();
                if (rowHovered) {
                    dl->AddRectFilled(rowStart, ImVec2(rowStart.x + labelWidth, rowStart.y + rowHeight),
                        ImGui::GetColorU32(ImGuiCol_HeaderHovered));
                }

                float textY = rowStart.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;

                ImVec2 namePos(rowStart.x + leftPad, textY);
                dl->AddText(nullptr, 0.0f, namePos, ImGui::GetColorU32(ImGuiCol_Text), name);

                ImVec2 catPos(namePos.x + nameSize.x + nameCatGap, textY);
                dl->AddText(nullptr, 0.0f, catPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), category);

                ImGui::SameLine(buttonPosX);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
                bool xClicked = m_ui.button("X");
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    m_hoveredMenuPartIds.clear();
                    CollectDescendants(p->id, m_hoveredMenuPartIds);
                }
                if (xClicked) deleteRequestId = p->id;
                ImGui::PopID();
            }
            if (sortedParts.empty()) ImGui::TextDisabled(m_ui.getText("scb.npp"));
            if (deleteRequestId >= 0) DeletePart(deleteRequestId);

            ImGui::EndMenu();
        }
        if (m_moveMode != MoveMode::CameraOrbit) ImGui::EndDisabled();
        ImGui::BeginDisabled();
        if (m_ui.beginMenu("scb.launch")) {
            ImGui::EndMenu();
        }
        ImGui::EndDisabled();
        std::string launchCheck = m_ui.getText("notimplemented");
        launchCheck += "\n";

        bool hasEngine = false;
        bool hasCockpit = false;
        for (auto& p : m_placedParts) {
            if (!p.partDef) continue;
            if (p.partDef->category == Category::Engine) hasEngine = true;
            if (p.partDef->category == Category::Cockpit) hasCockpit = true;
        }
        if (!hasEngine || !hasCockpit) {
            launchCheck += m_ui.getText("scb.launch.error");
            launchCheck += "\n";
        }
        if (!hasEngine) {
            launchCheck += m_ui.getText("scb.launch.noengine");
            launchCheck += "\n";
        }
        if (!hasCockpit) {
            launchCheck += m_ui.getText("scb.launch.nocockpit");
            launchCheck += "\n";
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip(launchCheck.c_str());
    }
    ImGui::EndMainMenuBar();
    ImGui::PopStyleColor(2);
}

void SpacecraftBuilderMode::ShowPartTooltip(const Part& part) const {
    if (!ImGui::IsItemHovered()) return;

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
    ImGui::TextUnformatted(part.name.c_str());
    ImGui::Separator();
    ImGui::TextUnformatted(part.description.c_str());

    if (part.mass > 0.0f) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "+%.2f kg mass", part.mass);
    if (part.thrust > 0.0f) ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "+%.2f N thrust", part.thrust);
    if (part.fuelCapacity > 0.0f) ImGui::TextColored(ImVec4(0.6f, 0.6f, 1.0f, 1.0f), "+%.2f L fuel", part.fuelCapacity);

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void SpacecraftBuilderMode::ApplyPartHighlight() {
    for (auto& p : m_placedParts) {
        if (!p.entity) continue;
        auto* rc = p.entity->getComponent<RenderComponent>();
        if (!rc) continue;

        bool highlighted = std::find(m_hoveredMenuPartIds.begin(), m_hoveredMenuPartIds.end(), p.id) != m_hoveredMenuPartIds.end();
        if (highlighted) {
            float pulse = std::sin(m_game.GetEngine().getTime() * 6.0f) * 0.2f + 0.8f;
            rc->setBaseColor(Vec3(1.0f, 0.85f, 0.25f) * pulse);
        } else {
            rc->setBaseColor(Vec3(1.0f, 1.0f, 1.0f));
        }
    }
}

void SpacecraftBuilderMode::updateCenterOfMass() {
    if (!m_showCenterOfMass) return;
    if (m_placedParts.empty()) return;

    Vec3 com(0.0f);
float totalMass = 0.0f;

    for (auto& placed : m_placedParts) {
        if (!placed.entity || !placed.partDef) continue;
        com += placed.entity->transform.position * placed.partDef->mass;
        totalMass += placed.partDef->mass;
    }

    if (totalMass > 0.0f) {
        com = com / totalMass;
        m_spaceship.centerOfMass = com;
    }

    if (m_moveMode != MoveMode::CameraOrbit || m_isTransitioning) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 screenPos;
    if (WorldToScreen(com, screenPos)) {
        dl->AddCircleFilled(screenPos, 6.0f, IM_COL32(255, 255, 0, 255));
        dl->AddCircle(screenPos, 8.0f, IM_COL32(255, 255, 0, 150), 16, 1.0f);
    }
}

void SpacecraftBuilderMode::updateGeometryCenter() {
    if (!m_showGeometryCenter) return;
    if (m_placedParts.empty()) return;

    Vec3 minPos(1e9f, 1e9f, 1e9f);
    Vec3 maxPos(-1e9f, -1e9f, -1e9f);
    int count = 0;

    for (auto& placed : m_placedParts) {
        if (!placed.entity || !placed.partDef) continue;
        const Vec3& pos = placed.entity->transform.position;
        minPos.x = std::min(minPos.x, pos.x);
        minPos.y = std::min(minPos.y, pos.y);
        minPos.z = std::min(minPos.z, pos.z);
        maxPos.x = std::max(maxPos.x, pos.x);
        maxPos.y = std::max(maxPos.y, pos.y);
        maxPos.z = std::max(maxPos.z, pos.z);
        count++;
    }

    Vec3 geomCenter = (minPos + maxPos) * 0.5f;

    if (count > 0) {
        m_spaceship.geometryCenter = geomCenter;
    }

    if (m_moveMode != MoveMode::CameraOrbit || m_isTransitioning) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 screenPos;
    if (WorldToScreen(geomCenter, screenPos)) {
        dl->AddCircleFilled(screenPos, 6.0f, IM_COL32(0, 255, 255, 255));
        dl->AddCircle(screenPos, 8.0f, IM_COL32(0, 255, 255, 150), 16, 1.0f);
    }
}

void SpacecraftBuilderMode::showAssemblyWindow() {
    if (m_moveMode == MoveMode::CameraFirstPerson || m_moveMode == MoveMode::Blueprint || m_isTransitioning) return;

    ImGuiIO& io = ImGui::GetIO();
    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;

    float panelHeight = screenH * 0.4f;
    float panelWidth = std::max(300.0f, screenW * 0.2f);
    float panelY = (screenH - panelHeight) * 0.5f;
    float panelX = screenW - panelWidth;

    const float buttonWidth = 48.0f;
    const float buttonHeight = 64.0f;
    float buttonY = panelY + (panelHeight - buttonHeight) * 0.5f;
    float buttonX = m_assemblyPanelVisible ? (panelX - buttonWidth) : (screenW - buttonWidth);

    ImGui::SetNextWindowPos(ImVec2(buttonX, buttonY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(buttonWidth, buttonHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::Begin("##AssemblyToggleButton", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10.0f);
    m_ui.setFont(1); 
    if (m_ui.button(m_assemblyPanelVisible ? ">" : "<", ImVec2(buttonWidth, buttonHeight))) {
        m_assemblyPanelVisible = !m_assemblyPanelVisible;
    }
    m_ui.resetFont();

    ImGui::End();
    ImGui::PopStyleColor();

    if (!m_assemblyPanelVisible) return;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(panelWidth, panelHeight), ImVec2(panelWidth, panelHeight));

    ImGui::Begin("Assembly", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);

    const float categoryBarWidth = 56.0f;
    const float categoryButtonSize = 48.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    float contentHeight = ImGui::GetContentRegionAvail().y;
    float partsListWidth = ImGui::GetContentRegionAvail().x - categoryBarWidth - spacing;

    ImGui::BeginChild("##PartsList", ImVec2(partsListWidth, contentHeight), false);
    {
        if (m_selectedCategory == Category::Information) {
            drawInformation();
        } else if (m_selectedCategory == Category::Customization) {
            drawCustomization();
        } else {
            const float cellWidth = ImGui::GetContentRegionAvail().x * 0.5f;
            const float cellHeight = 96.0f;
            const float textPadding = 8.0f;

            std::vector<Part*> visibleParts;
            for (auto& part : m_parts) {
                if (part.category == m_selectedCategory) visibleParts.push_back(&part);
            }

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            auto drawCell = [&](Part* part, int idInRow) {
                ImVec2 cellSize(cellWidth - spacing, cellHeight);
                ImVec2 cellStartScreen = ImGui::GetCursorScreenPos();

                bool shouldDisable = m_placedParts.empty() && !part->canBePlacedFirst;
                if (shouldDisable) ImGui::BeginDisabled();
                ImGui::PushID(idInRow);
                if (m_ui.button((std::string("##PartCell_") + part->name).c_str(), cellSize)) {
                    StartGhostPlacement(*part);
                }
                if (shouldDisable && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(m_ui.getText("scb.firstpart"));
                    ImGui::EndTooltip();
                }
                ImGui::PopID();
                if (shouldDisable) ImGui::EndDisabled();
                
                ShowPartTooltip(*part);
                float wrapWidth = cellSize.x - textPadding * 2.0f;
                ImVec2 titlePos(cellStartScreen.x + textPadding, cellStartScreen.y + textPadding);
                drawList->AddText(nullptr, 0.0f, titlePos, ImGui::GetColorU32(ImGuiCol_Text),
                    part->name.c_str(), nullptr, wrapWidth);

                ImVec2 titleSize = ImGui::CalcTextSize(part->name.c_str(), nullptr, false, wrapWidth);
                ImVec2 subtitlePos(titlePos.x, titlePos.y + titleSize.y + 2.0f);
                drawList->AddText(nullptr, 0.0f, subtitlePos, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                    AttachTagToString(part->attachTag), nullptr, wrapWidth);
            };

            for (size_t i = 0; i < visibleParts.size(); i += 2) {
                drawCell(visibleParts[i], static_cast<int>(i));

                if (i + 1 < visibleParts.size()) {
                    ImGui::SameLine(0.0f, spacing);
                    drawCell(visibleParts[i + 1], static_cast<int>(i + 1));
                }
            }

            if (visibleParts.empty()) {
                ImGui::TextDisabled(m_ui.getText("scb.noparts"));
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // right side category bar
    ImGui::BeginChild("##CategorySidebar", ImVec2(categoryBarWidth, contentHeight), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        for (Category cat : m_categorys) {
            bool isSelected = (cat == m_selectedCategory);

            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }

            ImGui::PushID(static_cast<int>(cat));
            if (m_ui.button(CategoryAbbrev(cat), ImVec2(categoryButtonSize, categoryButtonSize))) {
                m_selectedCategory = cat;
            }
            ImGui::PopID();

            if (isSelected) {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(CategoryToString(cat));
                ImGui::EndTooltip();
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void SpacecraftBuilderMode::drawInformation() {
    //categorybarwidht = 56.0f
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 56.0f);
    m_ui.setFont(1);
    ImGui::TextUnformatted(m_ui.getText("scb.info"));
    m_ui.resetFont();
    ImGui::Separator();

    char nameBuffer[128];
    std::strncpy(nameBuffer, m_spaceship.name.c_str(), sizeof(nameBuffer));
    if (ImGui::InputText(m_ui.getText("scb.info.name"), nameBuffer, sizeof(nameBuffer))) {
        m_spaceship.name = std::string(nameBuffer);
    }

    char versionBuffer[32];
    std::strncpy(versionBuffer, m_spaceship.version.c_str(), sizeof(versionBuffer));
    if (ImGui::InputText(m_ui.getText("scb.info.version"), versionBuffer, sizeof(versionBuffer))) {
        m_spaceship.version = std::string(versionBuffer);
    }

    char descBuffer[512];
    std::strncpy(descBuffer, m_spaceship.description.c_str(), sizeof(descBuffer));
    if (ImGui::InputTextMultiline(m_ui.getText("scb.info.desc"), descBuffer, sizeof(descBuffer), ImVec2(-1.0f, 150.0f))) {
        m_spaceship.description = std::string(descBuffer);
    }

    ImGui::BeginDisabled();
    if (ImGui::DragFloat(m_ui.getText("mass"), &m_spaceship.mass, 0.1f, 1.0f)) {}
    if (ImGui::DragFloat(m_ui.getText("thrust"), &m_spaceship.thrust, 0.1f, 1.0f)) {} 
    if (ImGui::DragFloat(m_ui.getText("fuel"), &m_spaceship.fuelCapacity, 0.1f, 1.0f)) {}
    ImGui::EndDisabled();
}

void SpacecraftBuilderMode::drawCustomization() {
    ImGui::TextUnformatted("Customization");
}

void SpacecraftBuilderMode::Save() {
    std::string baseDir = Path::resolve("user/spacecrafts/");

    nlohmann::json j;
    j["name"]        = m_spaceship.name;
    j["description"] = m_spaceship.description;
    j["version"]     = m_spaceship.version;

    nlohmann::json partsArray = nlohmann::json::array();
    for (const auto& placed : m_placedParts) {
        if (!placed.entity || !placed.partDef) continue;

        nlohmann::json jp;
        jp["id"]              = placed.id;
        jp["parentId"]        = placed.parentId;
        jp["parentAttachIdx"] = placed.parentAttachIdx;
        jp["partName"]        = placed.partDef->name;
        jp["position"]        = { placed.entity->transform.position.x,
                                   placed.entity->transform.position.y,
                                   placed.entity->transform.position.z };
        jp["rotation"]        = { placed.entity->transform.rotation.x,
                                   placed.entity->transform.rotation.y,
                                   placed.entity->transform.rotation.z };
        jp["usedAttachments"] = placed.usedAttachments;

        partsArray.push_back(jp);
    }
    j["parts"] = partsArray;

    std::string savePath = baseDir + m_spaceship.name + ".spacecraft";
    std::ofstream file(savePath);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
        Logger::info("Saved spacecraft to " + savePath);
    } else {
        Logger::error("Failed to save spacecraft to " + savePath);
    }
}

void SpacecraftBuilderMode::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::error("SpacecraftBuilder: could not open '" + filepath + "'");
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        Logger::error("SpacecraftBuilder: failed to parse '" + filepath + "'");
        return;
    }

    DeleteAllParts();

    m_spaceship.name        = j.value("name", m_spaceship.name);
    m_spaceship.description = j.value("description", m_spaceship.description);
    m_spaceship.version     = j.value("version", m_spaceship.version);

    int maxId = -1;
    if (j.contains("parts")) {
        for (const auto& jp : j["parts"]) {
            std::string partName = jp.value("partName", "");

            Part* partDef = nullptr;
            for (auto& p : m_parts) {
                if (p.name == partName) { partDef = &p; break; }
            }
            if (!partDef) {
                Logger::error("SpacecraftBuilder: unknown part '" + partName + "' in '" + filepath + "'");
                continue;
            }

            Vec3 position(0.0f), rotation(0.0f);
            if (jp.contains("position") && jp["position"].size() == 3) {
                position = Vec3(jp["position"][0].get<float>(),
                                 jp["position"][1].get<float>(),
                                 jp["position"][2].get<float>());
            }
            if (jp.contains("rotation") && jp["rotation"].size() == 3) {
                rotation = Vec3(jp["rotation"][0].get<float>(),
                                 jp["rotation"][1].get<float>(),
                                 jp["rotation"][2].get<float>());
            }

            Entity* entity = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity(partDef->name);
            entity->addComponent<RenderComponent>(partDef->modelPath);
            entity->transform.position = position;
            entity->transform.rotation = rotation;

            PlacedPart placed;
            placed.entity          = entity;
            placed.partDef         = partDef;
            placed.id              = jp.value("id", 0);
            placed.parentId        = jp.value("parentId", -1);
            placed.parentAttachIdx = jp.value("parentAttachIdx", -1);

            placed.usedAttachments = std::vector<bool>(partDef->attachmentPoints.size(), false);
            if (jp.contains("usedAttachments")) {
                const auto& ua = jp["usedAttachments"];
                for (size_t i = 0; i < ua.size() && i < placed.usedAttachments.size(); ++i) {
                    placed.usedAttachments[i] = ua[i].get<bool>();
                }
            }

            maxId = std::max(maxId, placed.id);

            m_placedParts.push_back(placed);
            m_spaceship.parts.push_back(*partDef);

            m_spaceship.mass         += partDef->mass;
            m_spaceship.thrust       += partDef->thrust;
            m_spaceship.fuelCapacity += partDef->fuelCapacity;
        }
    }

    for (auto& child : m_placedParts) {
        if (child.parentId < 0) continue;
        for (auto& parent : m_placedParts) {
            if (parent.id == child.parentId) {
                parent.childIds.push_back(child.id);
                break;
            }
        }
    }

    m_nextPlacedPartId = maxId + 1;

    Logger::info("Loaded spacecraft from " + filepath);
}