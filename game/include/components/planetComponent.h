#pragma once

#include "game.h"

#include "engine/core/transform.h"
#include "engine/components/component.h"
#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "engine/render/postprocess/postProcessor.h"
#include "engine/utils/logger.h"

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "engine/components/rendererComponent.h"
#include "engine/render/texture.h"
#include "engine/render/render.h"

class Game;

struct PlanetParams {
    Vec3 velocity = Vec3(0.0f, 0.0f, 0.0f);
    float period = 24.0f;
    float radius = 637.1f;
    float mass = 100.0f;
    bool locked = false;

    Vec3 sunDir = Vec3(1.0f, 0.0f, 0.0f);
    float sunIntensity = 6.0f;
};

struct AtmosphereParams {
    bool hasAtmosphere = false;
    float thickness = 100.0f;
    Vec3 rayleighCoeff = Vec3(5.8e-3f, 13.5e-3f, 33.1e-3f);
    float edgeFalloff = 0.5f;
};

struct WaterParams {
    bool hasWater = false;
    float level = 20.0f;
    Vec3 colorA = Vec3(0.0f, 0.75f, 1.0f);
    Vec3 colorB = Vec3(0.0f, 0.3f, 0.6f);
    float depthMultiplier = 0.13f;
    float alphaMultiplier = 0.06f;
};

class PlanetComponent : public Component {
public:
    PlanetComponent(Game& game, float period = 24.0f, float radius = 637.1f, float mass = 100.0f);
    ~PlanetComponent();

    void initialize();
    void update(float dt) override;
    std::unique_ptr<Component> clone() const override;

    PlanetParams& getPlanetParams();
    void applyScale();

    void setHasAtmosphere(bool hasAtmosphere);
    bool hasAtmosphere() const;
    AtmosphereParams& getAtmosphere();
    
    void setHasWater(bool hasWater);
    bool hasWater() const;
    WaterParams& getWater();

    void loadFromFile(const std::string& filepath, std::shared_ptr<Texture>& paintTexture);

private:
    void reorderLayers();

    Game* m_game;

    float m_currentRotation = 0.0f;
    Vec3 m_inicialRot = Vec3(0.0f, 0.0f, 0.0f);

    PlanetParams m_planetParams;

    bool m_hasAtmosphere = false;
    PostProcessor* m_atmosphere = nullptr;
    AtmosphereParams m_atmosphereParams;

    bool m_hasWater = false;
    WaterParams m_waterParams;
    PostProcessor* m_water = nullptr;
};