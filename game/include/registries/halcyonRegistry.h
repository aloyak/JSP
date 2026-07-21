#pragma once

#include "engine/components/entity.h"
#include "components/planetComponent.h"
#include "game.h"

#include <string>

struct Planet {
    std::string name;
    std::string highLodModelPath = "assets/models/planetbase.fbx";
    std::string lowLodModelPath = "assets/models/planetbase.fbx";
    Entity* entity;
    PlanetComponent* component;
};

inline std::vector<Planet> GetPlanetsFromScene(Game& game, Scene* scene, Entity* camera) {
    std::vector<Planet> planets;

    // TODO: set planet's models, parameters and shaders
    // TODO: and disable those details you cant see
    {
        Planet p;
        p.entity = scene->getEntityByName("Yin")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        p.component->setCamera(camera);

        auto& params = p.component->getPlanetParams();
        params.radius = 100.0f;

        params.semiMajorAxis = 3000.0f;
        params.eccentricity = 0.2f;
        params.inclination = 40.0f;
        params.argOfPeriapsis = 45.0f;
        params.longitudeAscendingNode = 0.0f;
        params.initialMeanAnomaly = 0.0f;
        params.orbitalPeriod = 12.0f;
        params.orbitDirection = 1;

        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("Yang")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        p.component->setCamera(camera);

        auto& params = p.component->getPlanetParams();
        params.radius = 100.0f;

        params.semiMajorAxis = 3000.0f;
        params.eccentricity = 0.2f;
        params.inclination = 40.0f;
        params.argOfPeriapsis = 45.0f;
        params.longitudeAscendingNode = 0.0f;
        params.initialMeanAnomaly = 180.0f; // opposite side of the same ellipse
        params.orbitalPeriod = 12.0f;
        params.orbitDirection = 1;

        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("Gaia")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        p.component->setCamera(camera);

        auto& params = p.component->getPlanetParams();
        params.radius = 100.0f;
        params.mass = 1000.0f;
        params.semiMajorAxis = 5000.0f;
        params.eccentricity = 0.05f;
        params.inclination = 0.0f;
        params.argOfPeriapsis = 0.0f;
        params.longitudeAscendingNode = 15.0f;
        params.initialMeanAnomaly = 60.0f;
        params.orbitalPeriod = 20.0f;
        params.orbitDirection = 1;

        p.component->setHasAtmosphere(true);
        p.component->getAtmosphere().thickness = 350.0f;
        p.component->getAtmosphere().density = 3.0f;

        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("Kepler")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        p.component->setCamera(camera);

        auto& params = p.component->getPlanetParams();
        params.radius = 500.0f;
        params.mass = 1000.0f; // DEBUG
        params.semiMajorAxis = 8000.0f;
        params.eccentricity = 0.4f;
        params.inclination = 15.0f;
        params.argOfPeriapsis = 200.0f;
        params.longitudeAscendingNode = 80.0f;
        params.initialMeanAnomaly = 300.0f;
        params.orbitalPeriod = 35.0f;
        params.orbitDirection = -1; // orbits the wrong way

        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("VestaStar")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);

        auto& params = p.component->getPlanetParams();
        params.radius = 80.0f;

        params.semiMajorAxis = 800.0f;
        params.eccentricity = 0.0f;
        params.inclination = 0.0f;
        params.argOfPeriapsis = 0.0f;
        params.longitudeAscendingNode = 0.0f;
        params.initialMeanAnomaly = 0.0f;
        params.orbitalPeriod = 3.0f;
        params.orbitDirection = 1;

        p.component->setHasAtmosphere(true);
        p.component->getAtmosphere().thickness = 550.0f;
        p.component->getAtmosphere().edgeFalloff = 1200.0f;
        p.component->getAtmosphere().rayleighCoeff = Vec3(0.003f, 0.0012f, 0.0002f);
        p.component->getPlanetParams().sunIntensity = 30.0f;
        p.component->getAtmosphere().density = 2.5f;

        planets.push_back(p);
    }

    for (auto& planet : planets) {
        planet.component->setCamera(camera);
        planet.component->applyScale();

        // start with high LOD so it gets registered and switching is fast, hopefully
        // TODO: in case doing renderComponent.setModel is too slow, use a two-entity system where it just gets disabled/enabled
        planet.entity->getComponent<RenderComponent>()->setModelPath(planet.highLodModelPath);
        if (planet.entity->hasComponent<RigidbodyComponent>()) {
            planet.entity->getComponent<RigidbodyComponent>()->setMass(planet.component->getPlanetParams().mass);
        }
    }

    return planets;
}