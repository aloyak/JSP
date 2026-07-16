#pragma once

#include "engine/core/math.h"

#include <algorithm>
#include <string>
#include <vector>

// TODO: util parachute, stackable 1 part structural, structural-panel, oxigen
enum class Category {
    Information,
    Customization,
    Engine,
    FuelTank,
    Cockpit,
    Structural,
    Electrical,
    Utility,
};

inline const char* CategoryToString(Category cat) {
    switch (cat) {
        case Category::Information:   return "Information";
        case Category::Customization: return "Customization";
        case Category::Engine:        return "Engine";
        case Category::FuelTank:      return "Fuel Tank";
        case Category::Cockpit:       return "Cockpit";
        case Category::Structural:    return "Structural";
        case Category::Electrical:    return "Electrical";
        case Category::Utility:       return "Utility";
    }
    return "Unknown";
}

inline const char* CategoryAbbrev(Category cat) {
    switch (cat) {
        case Category::Information:   return "Info";
        case Category::Customization: return "Cstm";
        case Category::Engine:        return "Eng";
        case Category::FuelTank:      return "Fuel";
        case Category::Cockpit:       return "Ckpt";
        case Category::Structural:    return "Strct";
        case Category::Electrical:    return "Elec";
        case Category::Utility:       return "Util";
    }
    return "???";
}

enum class AttachTag {
    FuelTank,
    Structural,
    Engine,
    RocketBooster,
    Cockpit,
    Electrical,
    LandingGear,
    ModuleEngine,
    SolarPanel,
    OxigenTank,
};

inline const char* AttachTagToString(AttachTag tag) {
    switch (tag) {
        case AttachTag::FuelTank:      return "Fuel Tank";
        case AttachTag::Structural:    return "Structural";
        case AttachTag::Engine:        return "Engine";
        case AttachTag::RocketBooster: return "Rocket Booster";
        case AttachTag::Cockpit:       return "Cockpit";
        case AttachTag::Electrical:    return "Electrical";
        case AttachTag::LandingGear:   return "Landing Gear";
        case AttachTag::ModuleEngine:  return "Module Engine";
        case AttachTag::SolarPanel:    return "Solar Panel";
        case AttachTag::OxigenTank:    return "Oxygen Tank";
    }
    return "Unknown";
}

struct Part; // forward declaration so Attachment can point back to it

struct Attachment {
    Vec3 position;
    Vec3 normal;
    std::vector<AttachTag> allowedTags; // empty = any tag allowed

    bool Allows(AttachTag tag) const {
        if (allowedTags.empty()) return true; // undefined -> allow anything
        return std::find(allowedTags.begin(), allowedTags.end(), tag) != allowedTags.end();
    }
};

struct Part {
    std::string name = "Unnamed Part";
    std::string description = "No description available.";
    std::string modelPath = "assets/models/parts/partbase.fbx";

    Category category = Category::Cockpit;      // which menu section this part is listed under
    AttachTag attachTag = AttachTag::Structural; // what this part presents when mating to another part's attachment point

    std::vector<Attachment> attachmentPoints;
    float mass = 10.0f;
    float thrust = 0.0f;
    float fuelCapacity = 0.0f;

    bool canBePlacedFirst = false;
};

struct Spaceship {
    std::string name = "Unnamed Spaceship";
    std::string description = "No description.";
    std::string version = "0.1.0";
    std::vector<Part> parts;
    float mass = 0.0f;
    float thrust = 0.0f;
    float fuelCapacity = 0.0f;
    Vec3 centerOfMass = Vec3(0.0f);
    Vec3 geometryCenter = Vec3(0.0f);

    float lowestPartY = 0.0f;
    float highestPartY = 0.0f;
};

inline std::vector<Part> CreateDefaultParts() {
    std::vector<Part> parts;

    {
        Part p;
        p.name = "Fuel Tank";
        p.description = "A basic fuel tank.";
        p.modelPath = "assets/models/spaceship/fuel.fbx";
        p.category = Category::FuelTank;
        p.attachTag = AttachTag::FuelTank;
        p.attachmentPoints = {
            {Vec3(0.0f, 2.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::Structural, AttachTag::OxigenTank}},
            {Vec3(0.0f, -2.5f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::Engine}},
        };
        p.mass = 300.0f;
        p.fuelCapacity = 100.0f;
        p.canBePlacedFirst = true;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Fuel Tank 2";
        p.description = "A larger fuel tank.";
        p.modelPath = "assets/models/spaceship/fuel_2.fbx";
        p.category = Category::FuelTank;
        p.attachTag = AttachTag::FuelTank;
        p.attachmentPoints = {
            {Vec3(0.0f, 4.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::Structural, AttachTag::OxigenTank}},
            {Vec3(0.0f, -4.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::Engine}},

            {Vec3(0.0f, 0.0f, 2.0f), Vec3(0.0f, 0.0f, 1.0f), {AttachTag::RocketBooster}},
            {Vec3(0.0f, 0.0f, -2.0f), Vec3(0.0f, 0.0f, -1.0f), {AttachTag::RocketBooster}},
        };
        p.mass = 500.0f;
        p.fuelCapacity = 200.0f;
        p.canBePlacedFirst = true;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Fuel Tank 3";
        p.description = "A huge fuel tank.";
        p.modelPath = "assets/models/spaceship/fuel_3.fbx";
        p.category = Category::FuelTank;
        p.attachTag = AttachTag::FuelTank;
        p.attachmentPoints = {
            {Vec3(0.0f, 4.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::Structural, AttachTag::OxigenTank}},
            {Vec3(0.0f, -4.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::Engine}},

            {Vec3(2.3f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {AttachTag::RocketBooster}},
            {Vec3(-2.3f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {AttachTag::RocketBooster}},
            {Vec3(0.0f, 0.0f, 2.3f), Vec3(0.0f, 0.0f, 1.0f), {AttachTag::RocketBooster}},
            {Vec3(0.0f, 0.0f, -2.3f), Vec3(0.0f, 0.0f, -1.0f), {AttachTag::RocketBooster}},
        };
        p.mass = 600.0f;
        p.fuelCapacity = 275.0f;
        p.canBePlacedFirst = true;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Rocket Booster";
        p.description = "A basic rocket booster.";
        p.modelPath = "assets/models/spaceship/booster.fbx";
        p.category = Category::Engine;          // listed under Engine in the menu
        p.attachTag = AttachTag::RocketBooster;  // but only mates with RocketBooster-accepting points
        p.attachmentPoints = {
            {Vec3(1.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {AttachTag::FuelTank}},
            {Vec3(-1.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {AttachTag::Engine}},
        };
        p.mass = 150.0f;
        p.thrust = 650.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Engine";
        p.description = "A basic engine.";
        p.modelPath = "assets/models/spaceship/engine.fbx";
        p.category = Category::Engine;
        p.attachTag = AttachTag::Engine;
        p.attachmentPoints = {
            {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), {AttachTag::FuelTank, AttachTag::Structural}},
        };
        p.mass = 100.0f;
        p.thrust = 2000.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Engine 3x";
        p.description = "A more powerful engine.";
        p.modelPath = "assets/models/spaceship/engine_2.fbx";
        p.category = Category::Engine;
        p.attachTag = AttachTag::Engine;
        p.attachmentPoints = {
            {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), {AttachTag::FuelTank}},
        };
        p.mass = 125.0f;
        p.thrust = 2400.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Module Engine";
        p.description = "An engine for travel module";
        p.modelPath = "assets/models/spaceship/mengine.fbx";
        p.category = Category::Engine;          // listed under Engine in the menu...
        p.attachTag = AttachTag::ModuleEngine;   // ...but only mates with attach points that accept ModuleEngine
        p.attachmentPoints = {
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural}},
            {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::LandingGear}},
        };
        p.mass = 50.0f;
        p.thrust = 500.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Structural-5";
        p.description = "A structural part with 5 attachment points.";
        p.modelPath = "assets/models/spaceship/structural_5.fbx";
        p.category = Category::Structural;
        p.attachTag = AttachTag::Structural;
        p.attachmentPoints = {
            // 4 side attachments (x/z), one on top
            {Vec3(2.0f, 1.9f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {AttachTag::ModuleEngine, AttachTag::Structural, AttachTag::Electrical}},
            {Vec3(-2.0f, 1.9f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {AttachTag::ModuleEngine, AttachTag::Structural, AttachTag::Electrical}},
            {Vec3(0.0f, 1.9f, 2.0f), Vec3(0.0f, 0.0f, 1.0f), {AttachTag::ModuleEngine, AttachTag::Structural, AttachTag::Electrical}},
            {Vec3(0.0f, 1.9f, -2.0f), Vec3(0.0f, 0.0f, -1.0f), {AttachTag::ModuleEngine, AttachTag::Structural, AttachTag::Electrical}},
            {Vec3(0.0f, 4.1f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Cockpit}},

            {Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::FuelTank, AttachTag::OxigenTank}},
        };
        p.mass = 75.0f;
        parts.push_back(p);
    }
    {
        // think of this as a panel for the structural-5 that has different attachment points
        Part p;
        p.name = "Structural-4";
        p.description = "A structural part with 4 attachment points.";
        p.modelPath = "assets/models/spaceship/structural_4.fbx";
        p.category = Category::Structural;
        p.attachTag = AttachTag::Structural;
        p.attachmentPoints = {
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural}},
        };
        p.mass = 40.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Oxigen Tank";
        p.description = "A basic oxigen tank.";
        p.modelPath = "assets/models/spaceship/oxigen_1.fbx";
        p.category = Category::Utility;
        p.attachTag = AttachTag::OxigenTank;
        p.attachmentPoints = {
            {Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural, AttachTag::OxigenTank}},
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::FuelTank}},
        };
        p.mass = 300.0f;
        p.canBePlacedFirst = true;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Panel";
        p.description = "Simple panel";
        p.modelPath = "assets/models/spaceship/panel.fbx";
        p.category = Category::Structural;
        p.attachTag = AttachTag::Structural;
        p.attachmentPoints = {
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural}},
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {AttachTag::SolarPanel}},
        };
        p.mass = 10.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Landing Gear";
        p.description = "A basic landing gear.";
        p.modelPath = "assets/models/spaceship/landinggear.fbx";
        p.category = Category::Structural;      // listed under Structural in the menu...
        p.attachTag = AttachTag::LandingGear;    // ...but only mates with attach points that accept LandingGear
        p.attachmentPoints = {
            {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::ModuleEngine}},
        };
        p.mass = 25.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Cockpit";
        p.description = "A basic cockpit.";
        p.modelPath = "assets/models/spaceship/cockpit.fbx";
        p.category = Category::Cockpit;
        p.attachTag = AttachTag::Cockpit;
        p.attachmentPoints = {
            {Vec3(0.0f, -0.75f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural}},
        };
        p.mass = 50.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Cockpit 2";
        p.description = "";
        p.modelPath = "assets/models/spaceship/cockpit_2.fbx";
        p.category = Category::Cockpit;
        p.attachTag = AttachTag::Cockpit;
        p.attachmentPoints = {
            {Vec3(0.0f, -0.75f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural}},
        };
        p.mass = 50.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Small Battery";
        p.description = "A small battery for electrical power.";
        p.modelPath = "assets/models/spaceship/battery.fbx";
        p.category = Category::Electrical;
        p.attachTag = AttachTag::Electrical;
        p.attachmentPoints = {
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::Structural}},
            {Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {AttachTag::SolarPanel}},
        };
        p.mass = 100.0f;
        parts.push_back(p);
    }
    {
        Part p;
        p.name = "Solar Panel";
        p.description = "A solar panel for generating electrical power.";
        p.modelPath = "assets/models/spaceship/solarpanel.fbx";
        p.category = Category::Electrical;
        p.attachTag = AttachTag::SolarPanel;
        p.attachmentPoints = {
            {Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {AttachTag::Structural}},
        };
        p.mass = 50.0f;
        parts.push_back(p);
    }

    return parts;
}