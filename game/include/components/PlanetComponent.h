#pragma once

#include "engine/core/transform.h"
#include "engine/components/component.h"
#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"

#include "engine/render/postprocess/postProcessor.h"

#include "engine/utils/logger.h"

class Game;

class PlanetComponent : public Component {
public:
    PlanetComponent(Game& game, float period = 24.0f, float radius = 637.1f)
        : m_period(period), m_radius(radius), m_game(&game) {}


    ~PlanetComponent() {
        if (m_atmosphere && m_game) {
            m_game->GetEngine().getRenderer().removePostProcessor(m_atmosphere);
            m_atmosphere = nullptr;
        }
    }

    void setGame(Game& game) { m_game = &game; }

    void initialize() {
        if (!entity) return;

        Transform& transform = entity->transform;
        transform.scale = Vec3(m_radius, m_radius, m_radius);
        transform.rotation = Vec3(-90.0f, 0, 0); 

        if (m_hasAtmosphere && !m_atmosphere && m_game) {
            m_atmosphere = &m_game->GetEngine().getRenderer().addPostProcessor(
                "assets/shaders/atmosphere_vert.glsl", "assets/shaders/atmosphere_frag.glsl"
            );
        }
    }

    void update(float dt) override {
        if (!entity || !isEnabled) return;

        float speed = (360.0f / (m_period * 3600.0f)); 
        m_currentRotation += speed * dt;

        entity->transform.rotation.y = m_currentRotation;

        if (m_hasAtmosphere && m_atmosphere && entity && m_game) {
            auto& engine = m_game->GetEngine();

            m_atmosphere->setVec3 ("u_planetCenter",     entity->transform.position);
            m_atmosphere->setFloat("u_planetRadius",     m_radius);
            m_atmosphere->setFloat("u_atmosphereRadius", m_radius + m_atmosphereThickness);

            Entity* cam = engine.getActiveCamera();
            if (cam) {
                m_atmosphere->setVec3("u_cameraPos", cam->transform.position);

                auto* camComp = cam->getComponent<CameraComponent>();
                if (camComp) {
                    m_atmosphere->setMat4("u_invView", camComp->getCamera().getInvViewMatrix(cam->transform));
                    m_atmosphere->setMat4("u_invProj", camComp->getCamera().getInvProjMatrix());
                    m_atmosphere->setFloat("u_near", camComp->getNear());
                    m_atmosphere->setFloat("u_far",  camComp->getFar());
                }
            }

            m_atmosphere->setVec3("u_sunDir", m_sunDir);

            float atmoRadius = m_radius + m_atmosphereThickness;
            m_atmosphere->setVec3 ("u_rayleighCoeff",  m_rayleighCoeff);
            m_atmosphere->setFloat("u_rayleighScaleH", atmoRadius * 0.08f);
            m_atmosphere->setFloat("u_sunIntensity",   m_sunIntensity);
            m_atmosphere->setFloat("u_edgeFalloff",     m_edgeFalloff);
        }
    }

    void setHasAtmosphere(bool hasAtmosphere) { 
        m_hasAtmosphere = hasAtmosphere; 
        if (m_hasAtmosphere) {
            if (!m_atmosphere && m_game) {
                m_atmosphere = &m_game->GetEngine().getRenderer().addPostProcessor(
                    "assets/shaders/default_vert.glsl", "assets/shaders/atmosphere_frag.glsl"
                );
            }
        } else {
            if (m_atmosphere && m_game) {
                m_game->GetEngine().getRenderer().removePostProcessor(m_atmosphere);
                m_atmosphere = nullptr;
            }
        }
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

    bool hasAtmosphere() const { return m_hasAtmosphere; }

    float getAtmosphereThickness() const { return m_atmosphereThickness; }
    void  setAtmosphereThickness(float t) { m_atmosphereThickness = t; }

    Vec3 getRayleighCoeff() const { return m_rayleighCoeff; }
    void setRayleighCoeff(const Vec3& c) { m_rayleighCoeff = c; }

    Vec3 getSunDir() const { return m_sunDir; }
    void setSunDir(const Vec3& dir) { m_sunDir = dir; }

    float getSunIntensity() const { return m_sunIntensity; }
    void  setSunIntensity(float i) { m_sunIntensity = i; }

    float getEdgeFalloff() const { return m_edgeFalloff; }
    void  setEdgeFalloff(float f) { m_edgeFalloff = f; }
private:
    Game* m_game;

    Vec3 m_velocity = {0, 0, 0};

    float m_period = 24.0f; 
    float m_radius = 637.1f;
    float m_currentRotation = 0.0f;

    bool m_hasAtmosphere = false;
    PostProcessor* m_atmosphere = nullptr;

    float m_atmosphereThickness = 100;

    Vec3  m_rayleighCoeff = Vec3(5.8e-3f, 13.5e-3f, 33.1e-3f);

    Vec3  m_sunDir = Vec3(1.0f, 0.0f, 0.0f);
    float m_sunIntensity = 10.0f;

    float m_edgeFalloff = 0.5f;
};