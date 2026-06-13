#pragma once

#include "engine/components/component.h"
#include "engine/components/entity.h"

class PlanetComponent : public Component {
public:
    float period = 24.0f; 
    float radius = 637.1f;
    float currentRotation = 0.0f;

    Vec3 getVelocity() const { return velocity; }
    void setVelocity(const Vec3& vel) { velocity = vel; }

    void update(float dt) override {
        if (!entity || !isEnabled) return;

        float speed = (360.0f / (period * 3600.0f)); 
        currentRotation += speed * dt;

        entity->transform.rotation.y = currentRotation;
    }

    std::unique_ptr<Component> clone() const override {
        return std::make_unique<PlanetComponent>(*this);
    }

private:
    Vec3 velocity = {0, 0, 0};
};