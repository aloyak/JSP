#pragma once

#include "engine/components/entity.h"
#include "engine/input/input.h"

class FirstPersonCamera {
private:
    Entity* m_camera = nullptr;

    float m_sensitivity = 4.0f;

    float m_headBobAmplitude = 0.2f; // per settings
    
    float m_headBobTimer = 0.0f;
    float m_headBobWeight = 0.0f;
    float m_headBobFrequency = 4.0f;

    float m_moveSpeed = 3.5f;
    float m_fov = 75.0f; 
    
    Vec3 playerPosition = {0.0f, 6.0f, 0.0f};
    Vec3 m_allowedMovementAxes = {1.0f, 0.0f, 1.0f}; 
public:
    FirstPersonCamera(Entity* camera) : m_camera(camera) {
        
    }

    void SyncFromCurrentPosition() {
        playerPosition = m_camera->transform.position;
    }

    void SetPlayerPosition(const Vec3& position) {
        playerPosition = position;
        m_camera->transform.position = playerPosition;
    }

    void Update(Input* input, float deltaTime, float time) {
        Vec2 mouseDelta = input->getMouseDelta();
        m_camera->transform.rotation.x -= mouseDelta.y * m_sensitivity * deltaTime;
        m_camera->transform.rotation.x = std::clamp(m_camera->transform.rotation.x, -89.0f, 89.0f);
        m_camera->transform.rotation.y += mouseDelta.x * m_sensitivity * deltaTime;

        Vec3 forward = m_camera->transform.forward();
        Vec3 right = m_camera->transform.right();

        Vec3 dir(0.0f);

        if (input->isKeyDown(KEY_W)) dir += forward;
        if (input->isKeyDown(KEY_S)) dir -= forward;
        if (input->isKeyDown(KEY_D)) dir -= right;
        if (input->isKeyDown(KEY_A)) dir += right;

        bool isMoving = dir.length() > 0.0f;

        if (isMoving) {
            dir = dir.normalize();
            playerPosition += dir * m_allowedMovementAxes * m_moveSpeed * deltaTime;
        }

        m_camera->transform.position = playerPosition;

        if (isMoving) {
            m_headBobTimer += deltaTime * m_moveSpeed * m_headBobFrequency;
        }

        float targetWeight = isMoving ? 0.2f : 0.0f;
        float bobFadeSpeed = 8.0f; // tune to taste
        m_headBobWeight += (targetWeight - m_headBobWeight) * std::clamp(bobFadeSpeed * deltaTime, 0.0f, 1.0f);

        float bobOffset = std::sin(m_headBobTimer) * m_headBobAmplitude * m_headBobWeight;
        m_camera->transform.position.y += bobOffset;
    }

    Vec3 getPlayerPosition() const { return playerPosition; }

    void setSensitivity(float newSensitivity) { m_sensitivity = newSensitivity; }
    float getSensitivity() const { return m_sensitivity; }

    void setMoveSpeed(float newMoveSpeed) { m_moveSpeed = newMoveSpeed; }
    float getMoveSpeed() const { return m_moveSpeed; }

    void setFov(float newFov) { m_fov = newFov; }
    float getFov() const { return m_fov; }

    void setHeadBobAmplitude(float amplitude) { m_headBobAmplitude = amplitude; }
    float getHeadBobAmplitude() const { return m_headBobAmplitude; }
};