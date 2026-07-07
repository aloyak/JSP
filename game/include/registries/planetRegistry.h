// USED FOR SANDBOX FOR NOW

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include "engine/utils/logger.h"
#include "components/planetComponent.h"

struct RegistryAtmosphere {
    bool  hasAtmosphere = false;
    float thickness     = 100.0f;
    Vec3  rayleighCoeff = Vec3(5.8e-3f, 13.5e-3f, 33.1e-3f);
    float edgeFalloff   = 0.5f;
};

struct GravityBody {
    std::string         modelPath    = "assets/models/planetbase.fbx";
    float               mass         = 1.0f;
    float               radius       = 1.0f;
    float               period       = 24.0f;
    float               sunIntensity = 4.5f;
    std::string         name         = "Unnamed";
    bool                isReal       = true;
    bool                isCustom     = false;
    std::string         customPath   = "";
    RegistryAtmosphere  atmosphere;

    void apply(PlanetComponent& comp) const {
        PlanetParams& p = comp.getPlanetParams();
        p.mass         = mass;
        p.radius       = radius;
        p.period       = period;
        p.sunIntensity = sunIntensity;

        comp.setHasAtmosphere(atmosphere.hasAtmosphere);
        if (atmosphere.hasAtmosphere) {
            AtmosphereParams& a = comp.getAtmosphere();
            a.thickness     = atmosphere.thickness;
            a.rayleighCoeff = atmosphere.rayleighCoeff;
            a.edgeFalloff   = atmosphere.edgeFalloff;
        }

        comp.applyScale();
    }
};

class PlanetRegistry {
public:
    std::vector<GravityBody> bodies;

    PlanetRegistry() {
        // Earth
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/earth.fbx";
            b.name         = "Earth";
            b.mass         = 50.0f;
            b.radius       = 637.1f;
            b.period       = 24.0f;
            b.atmosphere   = { true, 140.0f, Vec3(0.006f, 0.014f, 0.033f), 300.0f };
            bodies.push_back(b);
        }
        // Moon
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/moon.fbx";
            b.name         = "Moon";
            b.mass         = 7.0f;
            b.radius       = 173.7f;
            b.period       = 27.3f;
            b.atmosphere   = { false };
            bodies.push_back(b);
        }
        // Mars
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/mars.fbx";
            b.name         = "Mars";
            b.mass         = 43.0f;
            b.radius       = 338.9f;
            b.period       = 24.6f;
            b.atmosphere   = { true, 40.0f, Vec3(18.0e-3f, 5.0e-3f, 2.0e-3f), 0.3f };
            bodies.push_back(b);
        }
        // Venus
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/venus.fbx";
            b.name         = "Venus";
            b.mass         = 23.0f;
            b.radius       = 605.2f;
            b.period       = 240.0f;
            b.atmosphere   = { true, 280.0f, Vec3(8.0e-3f, 7.5e-3f, 2.0e-3f), 1.2f };
            bodies.push_back(b);
        }
        // Mercury
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/mercury.fbx";
            b.name         = "Mercury";
            b.mass         = 5.0f;
            b.radius       = 243.9f;
            b.period       = 1408.0f;
            b.atmosphere   = { false };
            bodies.push_back(b);
        }
        // Jupiter
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/jupiter.fbx";
            b.name         = "Jupiter";
            b.mass         = 132.0f;
            b.radius       = 6991.1f;
            b.period       = 9.9f;
            b.atmosphere   = { true, 800.0f, Vec3(12.0e-3f, 8.0e-3f, 3.5e-3f), 1.5f };
            bodies.push_back(b);
        }
        // Saturn
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/saturn.fbx";
            b.name         = "Saturn";
            b.mass         = 56.0f;
            b.radius       = 5823.2f;
            b.period       = 10.7f;
            b.atmosphere   = { true, 700.0f, Vec3(10.0e-3f, 8.5e-3f, 3.0e-3f), 1.4f };
            bodies.push_back(b);
        }
        // Uranus
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/uranus.fbx";
            b.name         = "Uranus";
            b.mass         = 75.0f;
            b.radius       = 2536.2f;
            b.period       = 17.2f;
            b.atmosphere   = { true, 500.0f, Vec3(3.0e-3f, 10.0e-3f, 14.0e-3f), 1.0f };
            bodies.push_back(b);
        }
        // Neptune
        {
            GravityBody b;
            b.modelPath    = "assets/models/planets/neptune.fbx";
            b.name         = "Neptune";
            b.mass         = 17.0f;
            b.radius       = 2462.2f;
            b.period       = 16.1f;
            b.atmosphere   = { true, 480.0f, Vec3(1.5e-3f, 5.0e-3f, 20.0e-3f), 1.0f };
            bodies.push_back(b);
        }
    }

    const GravityBody* find(const std::string& name) const {
        for (const auto& b : bodies)
            if (b.name == name) return &b;
        return nullptr;
    }

    int loadCustom(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logger::error("PlanetRegistry: could not open '" + filePath + "'");
            return -1;
        }

        try {
            nlohmann::json j;
            file >> j;

            GravityBody body;
            body.modelPath  = "assets/models/planetbase.fbx";
            body.isReal     = false;
            body.isCustom   = true;
            body.customPath = filePath;

            if (j.contains("name"))                body.name                        = j["name"].get<std::string>();
            if (j.contains("mass"))                body.mass                        = j["mass"].get<float>();
            if (j.contains("radius"))              body.radius                      = j["radius"].get<float>();
            if (j.contains("period"))              body.period                      = j["period"].get<float>();
            if (j.contains("sunIntensity"))        body.sunIntensity                = j["sunIntensity"].get<float>();
            if (j.contains("hasAtmosphere"))       body.atmosphere.hasAtmosphere    = j["hasAtmosphere"].get<bool>();
            if (j.contains("atmosphereThickness")) body.atmosphere.thickness        = j["atmosphereThickness"].get<float>();
            if (j.contains("edgeFalloff"))         body.atmosphere.edgeFalloff      = j["edgeFalloff"].get<float>();
            if (j.contains("rayleighCoeff"))       body.atmosphere.rayleighCoeff    = Vec3(
                j["rayleighCoeff"][0], j["rayleighCoeff"][1], j["rayleighCoeff"][2]
            );

            bodies.push_back(body);
            return static_cast<int>(bodies.size()) - 1;

        } catch (...) {
            Logger::error("PlanetRegistry: failed to parse '" + filePath + "'");
            return -1;
        }
    }
};