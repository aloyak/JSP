#pragma once

#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "components/PlanetComponent.h"
#include "engine/input/input.h"
#include <algorithm>
#include <cmath>
#include <limits>

class OrbitCamera {
private:
    Entity* m_camera = nullptr;
    Entity* m_target = nullptr;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_radius = 50.0f;

    float m_maxRadius = 10000.0f;

    float m_minHeight = -std::numeric_limits<float>::infinity();
    float m_maxHeight = std::numeric_limits<float>::infinity();

public:
    OrbitCamera(Entity* camera) : m_camera(camera) {}

    void SetTarget(Entity* target, float initialRadius = 50.0f) {
        m_target = target;
        m_radius = initialRadius;
    }

    void SetHeightLimits(float minHeight, float maxHeight) {
        m_minHeight = minHeight;
        m_maxHeight = maxHeight;
    }

    void ApplyPosition() {
        if (!m_camera || !m_target) return;

        Vec3 targetPos = m_target->transform.position;
        Vec3 pos(
            targetPos.x + m_radius * cosf(m_pitch) * cosf(m_yaw),
            targetPos.y + m_radius * sinf(m_pitch),
            targetPos.z + m_radius * cosf(m_pitch) * sinf(m_yaw));

        pos.y = std::clamp(pos.y, m_minHeight, m_maxHeight);

        m_camera->transform.position = pos;
        m_camera->getComponent<CameraComponent>()->lookAt(*m_target);
    }

    void Update(Input* input, float deltaTime) {
        if (!m_camera || !m_target) return;

        const float mouseSensitivity = 0.001f;
        const float pitchLimit = 89.5f * (M_PI / 180.0f);

        Vec2 delta = input->getMouseDelta();
        m_yaw += delta.x * mouseSensitivity;
        m_pitch -= delta.y * mouseSensitivity;
        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

        ApplyPosition();
    }
    
    void ApplyScroll(Input* input) {
        if (!m_camera || !m_target) return;

        const float scrollSensitivity = 0.1f;

        m_radius *= 1.0f - (input->getScrollDelta().y * scrollSensitivity);
        float minRadius = 1.0f;
        if (m_target->hasComponent<PlanetComponent>()) {
            minRadius = m_target->getComponent<PlanetComponent>()->getPlanetParams().radius + 10.0f;
        }
        m_radius = std::clamp(m_radius, minRadius, m_maxRadius);

        ApplyPosition();
    }

    void SyncFromCurrentPosition() {
        if (!m_camera || !m_target) return;

        Vec3 offset = m_camera->transform.position - m_target->transform.position;
        m_radius = sqrtf(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        if (m_radius < 0.001f) return;

        m_pitch = asinf(std::clamp(offset.y / m_radius, -1.0f, 1.0f));
        m_yaw   = atan2f(offset.z, offset.x);
    }

    void SetOrientation(float yaw, float pitch) {
        m_yaw = yaw;
        m_pitch = pitch;
    }

    void SetMaxRadius(float maxRadius) { m_maxRadius = maxRadius; }

    float& GetRadius() { return m_radius; }

    Vec3 GetComputedPosition() const {
        if (!m_target) return Vec3(0.0f);

        Vec3 targetPos = m_target->transform.position;
        Vec3 pos(
            targetPos.x + m_radius * cosf(m_pitch) * cosf(m_yaw),
            targetPos.y + m_radius * sinf(m_pitch),
            targetPos.z + m_radius * cosf(m_pitch) * sinf(m_yaw));

        pos.y = std::clamp(pos.y, m_minHeight, m_maxHeight);
        return pos;
    }
};