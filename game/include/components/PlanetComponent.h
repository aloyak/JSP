#pragma once

#include "engine/core/transform.h"
#include "engine/components/component.h"
#include "engine/components/entity.h"

#include "engine/utils/logger.h"

// NOTE: 1000 km = 100 units

class PlanetComponent : public Component {
public:
    PlanetComponent(float period = 24.0f, float radius = 637.1f)
        : m_period(period), m_radius(radius) {}

    void adjustScale() {
        if (!entity) return;

        Transform& transform = entity->transform;
        transform.scale = Vec3(m_radius, m_radius, m_radius);
        transform.rotation = Vec3(-90.0f, 0, 0); // align the planet with +Y
    }

    void update(float dt) override {
        if (!entity || !isEnabled) return;

        float speed = (360.0f / (m_period * 3600.0f)); 
        m_currentRotation += speed * dt;

        entity->transform.rotation.y = m_currentRotation;
    }

    std::unique_ptr<Component> clone() const override {
        return std::make_unique<PlanetComponent>(*this);
    }

    Vec3 getVelocity() const { return m_velocity; }
    void setVelocity(const Vec3& vel) { m_velocity = vel; }

    float getPeriod() const { return m_period; }
    void setPeriod(float period) { m_period = period; }

    float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; }

    float getCurrentRotation() const { return m_currentRotation; }
    void setCurrentRotation(float rotation) { m_currentRotation = rotation; }

private:
    Vec3 m_velocity = {0, 0, 0};

    float m_period = 24.0f; 
    float m_radius = 637.1f;
    float m_currentRotation = 0.0f;
};