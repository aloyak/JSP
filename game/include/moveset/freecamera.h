#pragma once

#include "engine/components/entity.h"
#include "engine/input/input.h"

class FreeCamera {
private:
    Entity* m_camera = nullptr;

    float m_moveSpeed = 100.0f;
    float m_sensitivity = 1.0f;

public:
    FreeCamera(Entity* camera) : m_camera(camera) {}

    void Update(Input* input, float dt) {
        if (!m_camera || !input) return;

        Vec2 mouseDelta = input->getMouseDelta();
        m_camera->transform.rotation.x -= mouseDelta.y * m_sensitivity;
        m_camera->transform.rotation.x = std::clamp(m_camera->transform.rotation.x, -89.0f, 89.0f);
        m_camera->transform.rotation.y += mouseDelta.x * m_sensitivity;

        Vec3 dir(0.0f);
        Vec3 forward = m_camera->transform.forward();
        Vec3 right = m_camera->transform.right();

        if (input->isKeyDown(KEY_W)) dir += forward;
        if (input->isKeyDown(KEY_S)) dir -= forward;
        if (input->isKeyDown(KEY_D)) dir -= right;
        if (input->isKeyDown(KEY_A)) dir += right;

        bool isMoving = dir.length() > 0.0f;

        if (isMoving) {
            dir = dir.normalize();
            m_camera->transform.position += dir * m_moveSpeed * dt;
        }
    }

    void setSensitivity(float newSensitivity) { m_sensitivity = newSensitivity; }
    float getSensitivity() const { return m_sensitivity; }

    void setMoveSpeed(float newSpeed) { m_moveSpeed = newSpeed; }
    float getMoveSpeed() const { return m_moveSpeed; }
};