#pragma once

#include "engine/components/entity.h"
#include "engine/input/input.h"

// kind of like the first person view but affected by multiple gravity sources

class GravityBasedCamera {
private:
    Entity* m_camera = nullptr;

    float m_moveSpeed = 100.0f;
    float m_sensitivity = 1.0f;

    float m_playerHeight = 1.0f;

    // radius of which the player's gravity vector is influenced by nearby celestial bodies
    float m_gravInfluenceRadius = 1000.0f;

    float m_alignmentSpeed = 5.0f;
    float m_maxSlopeAngle = 45.0f;

    Vec3 m_gravity = Vec3(0.0f);
public:
    GravityBasedCamera(Entity* camera) : m_camera(camera) {}

    void Update(Input* input, float dt) {
        // First person camera view and movement, but affected by gravity vector
        // Camera always aligns with the gravity vector with a smooth transition, and movement is relative to the camera's orientation
        // If we are on a surface and gravity is not close to 0, the player should move along the surface
        // TODO: handle player-surface collision, movement along surface, and gravity vector alignment with surface normal
        // works with max slope angle
    }

    void setSensitivity(float newSensitivity) { m_sensitivity = newSensitivity; }
    float getSensitivity() const { return m_sensitivity; }

    void setMoveSpeed(float newSpeed) { m_moveSpeed = newSpeed; }
    float getMoveSpeed() const { return m_moveSpeed; }

    float getInfluenceRadius() const { return m_gravInfluenceRadius; }

    void setGravityVector(const Vec3& gravity) { m_gravity = gravity; }
    Vec3 getGravityVector() const { return m_gravity; }
};