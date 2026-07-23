#pragma once

#include "engine/components/entity.h"
#include "engine/components/rigidbodyComponent.h"
#include "engine/components/rendererComponent.h"
#include "components/planetComponent.h"
#include "registries/planetRegistry.h"
#include "game.h"

#include <string>
#include <vector>
#include <optional>

// "Planet" inside the explorer mode
struct Planet {
    std::string name;
    std::string highLodModelPath = "assets/models/planetbase.fbx";
    std::string lowLodModelPath = "assets/models/planetbase.fbx";
    Entity* entity;
    PlanetComponent* component;
};

struct HalcyonPlanet { // "Planet" for the sandbox mode
    std::string name;

    float radius = 100.0f;
    std::optional<float> mass;
    std::optional<float> sunIntensity;

    bool hasAtmosphere = false;
    std::optional<float> atmosphereThickness;
    std::optional<float> atmosphereEntryThickness;
    std::optional<float> atmosphereEdgeFalloff;
    std::optional<float> atmosphereDensity;
    std::optional<Vec3> atmosphereRayleigh;

    float semiMajorAxis          = 0.0f;
    float eccentricity           = 0.0f;
    float inclination            = 0.0f;
    float argOfPeriapsis         = 0.0f;
    float longitudeAscendingNode = 0.0f;
    float initialMeanAnomaly     = 0.0f;
    float orbitalPeriod          = 1.0f;
    int   orbitDirection         = 1;
};

inline const std::vector<HalcyonPlanet>& GetHalcyonPlanets() {
    static const std::vector<HalcyonPlanet> defs = []() {
        std::vector<HalcyonPlanet> v;

        {
            HalcyonPlanet d;
            d.name = "Yin";
            d.radius = 100.0f;
            d.semiMajorAxis = 3000.0f;
            d.eccentricity = 0.2f;
            d.inclination = 40.0f;
            d.argOfPeriapsis = 45.0f;
            d.longitudeAscendingNode = 0.0f;
            d.initialMeanAnomaly = 0.0f;
            d.orbitalPeriod = 12.0f;
            d.orbitDirection = 1;
            v.push_back(d);
        }
        {
            HalcyonPlanet d;
            d.name = "Yang";
            d.radius = 100.0f;
            d.semiMajorAxis = 3000.0f;
            d.eccentricity = 0.2f;
            d.inclination = 40.0f;
            d.argOfPeriapsis = 45.0f;
            d.longitudeAscendingNode = 0.0f;
            d.initialMeanAnomaly = 180.0f; // opposite side of the same ellipse
            d.orbitalPeriod = 12.0f;
            d.orbitDirection = 1;
            v.push_back(d);
        }
        {
            HalcyonPlanet d;
            d.name = "Gaia";
            d.radius = 1000.0f;
            d.mass = 10000.0f;
            d.semiMajorAxis = 5000.0f;
            d.eccentricity = 0.05f;
            d.inclination = 0.0f;
            d.argOfPeriapsis = 0.0f;
            d.longitudeAscendingNode = 15.0f;
            d.initialMeanAnomaly = 60.0f;
            d.orbitalPeriod = 20.0f;
            d.orbitDirection = 1;
            d.hasAtmosphere = true;
            d.atmosphereThickness = 350.0f;
            d.atmosphereDensity = 3.0f;
            d.atmosphereEntryThickness = 600.0f;
            v.push_back(d);
        }
        {
            HalcyonPlanet d;
            d.name = "Kepler";
            d.radius = 500.0f;
            d.mass = 1000.0f; // DEBUG
            d.semiMajorAxis = 8000.0f;
            d.eccentricity = 0.4f;
            d.inclination = 15.0f;
            d.argOfPeriapsis = 200.0f;
            d.longitudeAscendingNode = 80.0f;
            d.initialMeanAnomaly = 300.0f;
            d.orbitalPeriod = 35.0f;
            d.orbitDirection = -1; // orbits the wrong way
            v.push_back(d);
        }
        {
            HalcyonPlanet d;
            d.name = "VestaStar";
            d.radius = 80.0f;
            d.sunIntensity = 30.0f;
            d.semiMajorAxis = 800.0f;
            d.eccentricity = 0.0f;
            d.inclination = 0.0f;
            d.argOfPeriapsis = 0.0f;
            d.longitudeAscendingNode = 0.0f;
            d.initialMeanAnomaly = 0.0f;
            d.orbitalPeriod = 3.0f;
            d.orbitDirection = 1;
            d.hasAtmosphere = true;
            d.atmosphereThickness = 550.0f;
            d.atmosphereEdgeFalloff = 1200.0f;
            d.atmosphereRayleigh = Vec3(0.003f, 0.0012f, 0.0002f);
            d.atmosphereDensity = 2.5f;
            v.push_back(d);
        }

        return v;
    }();
    return defs;
}

inline std::vector<Planet> GetPlanetsFromScene(Game& game, Scene* scene, Entity* camera) { //explore
    std::vector<Planet> planets;

    // TODO: set planet's models, parameters and shaders
    // TODO: and disable those details you cant see
    for (const auto& def : GetHalcyonPlanets()) {
        Planet p;
        p.entity = scene->getEntityByName(def.name)->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        p.component->setCamera(camera);

        auto& params = p.component->getPlanetParams();
        params.radius = def.radius;
        if (def.mass)         params.mass         = *def.mass;
        if (def.sunIntensity) params.sunIntensity = *def.sunIntensity;

        params.semiMajorAxis          = def.semiMajorAxis;
        params.eccentricity           = def.eccentricity;
        params.inclination            = def.inclination;
        params.argOfPeriapsis         = def.argOfPeriapsis;
        params.longitudeAscendingNode = def.longitudeAscendingNode;
        params.initialMeanAnomaly     = def.initialMeanAnomaly;
        params.orbitalPeriod          = def.orbitalPeriod;
        params.orbitDirection         = def.orbitDirection;

        if (def.hasAtmosphere) {
            p.component->setHasAtmosphere(true);
            auto& atmo = p.component->getAtmosphere();
            if (def.atmosphereThickness)      atmo.thickness      = *def.atmosphereThickness;
            if (def.atmosphereEntryThickness) atmo.entryThickness = *def.atmosphereEntryThickness;
            if (def.atmosphereEdgeFalloff)    atmo.edgeFalloff    = *def.atmosphereEdgeFalloff;
            if (def.atmosphereDensity)        atmo.density        = *def.atmosphereDensity;
            if (def.atmosphereRayleigh)       atmo.rayleighCoeff  = *def.atmosphereRayleigh;
        }

        planets.push_back(p);
    }

    for (auto& planet : planets) {
        planet.component->setCamera(camera);
        planet.component->applyScale();

        
        planet.entity->getComponent<RenderComponent>()->setModelPath(planet.highLodModelPath);
        if (planet.entity->hasComponent<RigidbodyComponent>()) {
            planet.entity->getComponent<RigidbodyComponent>()->setMass(planet.component->getPlanetParams().mass);
        }
    }

    return planets;
}

inline std::vector<GravityBody> GetHalcyonGravityBodies() { // sandbox
    std::vector<GravityBody> bodies;

    float scaleFactor = 0.5f;

    for (const auto& def : GetHalcyonPlanets()) {
        GravityBody b;
        b.name   = def.name;
        b.isReal = false;
        b.radius = def.radius * scaleFactor;

        b.mass = def.mass.value_or(20.0f) * scaleFactor;

        if (def.sunIntensity) b.sunIntensity = *def.sunIntensity;

        b.atmosphere.hasAtmosphere = def.hasAtmosphere;
        if (def.hasAtmosphere) {
            if (def.atmosphereThickness)      b.atmosphere.thickness      = *def.atmosphereThickness * scaleFactor;
            if (def.atmosphereEntryThickness) b.atmosphere.entryThickness = *def.atmosphereEntryThickness * scaleFactor;
            if (def.atmosphereEdgeFalloff)    b.atmosphere.edgeFalloff    = *def.atmosphereEdgeFalloff;
            if (def.atmosphereDensity)        b.atmosphere.density        = *def.atmosphereDensity;
            if (def.atmosphereRayleigh)       b.atmosphere.rayleighCoeff  = *def.atmosphereRayleigh;
        }

        // for each saved planet, check if it is discovered
        // for (m_game.progress.discoveredPlanets) planet.discovered = m_game.progress... bla bla bla;
        b.discovered = false;
        bodies.push_back(b);
    }

    return bodies;
}