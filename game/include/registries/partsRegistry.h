#pragma once

#include "engine/core/math.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

enum class Category {
    // visible categories:
    Information,
    Customization,
    Engine,
    FuelTank,
    Cockpit,
    Structural,
    RocketBooster,
    Electrical,
    Utility,

    // hidden categories:
    LandingGear,
    ModuleEngine,
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
        default:                      return "Unknown";
    }
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
        default:                      return "???";
    }
}

struct Part; // forward declaration so Attachment can point back to it

struct Attachment {
    Vec3 position;
    Vec3 normal;
    std::vector<Category> allowedCategories; // empty/undefined = any category allowed

    bool AllowsCategory(Category cat) const {
        if (allowedCategories.empty()) return true; // undefined -> allow anything
        return std::find(allowedCategories.begin(), allowedCategories.end(), cat) != allowedCategories.end();
    }
};

struct Part {
    std::string name = "Unnamed Part";
    std::string description = "No description available.";
    std::string modelPath = "assets/models/parts/partbase.fbx";
    Category category = Category::Cockpit; // visible category: which UI section this part is listed under
    std::vector<Attachment> attachmentPoints;
    float mass = 10.0f;
    float thrust = 0.0f; 
    float fuelCapacity = 0.0f;

    bool canBePlacedFirst = false;
    std::optional<Category> placementCategory;

    Category GetPlacementCategory() const {
        return placementCategory.value_or(category);
    }
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
    return {
        Part{"Fuel Tank", "A basic fuel tank.", "assets/models/spaceship/fuel.fbx", Category::FuelTank,
            std::vector<Attachment>{
                {Vec3(0.0f, 2.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::FuelTank, Category::Structural}},
                {Vec3(0.0f, -2.5f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank, Category::Engine}}
            }, 300.0f, 0.0f, 100.0f, true},

        Part{"Fuel Tank 2", "A larger fuel tank.", "assets/models/spaceship/fuel_2.fbx", Category::FuelTank,
            std::vector<Attachment>{
                {Vec3(0.0f, 4.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::FuelTank, Category::Structural}},
                {Vec3(0.0f, -4.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank, Category::Engine}},

                {Vec3(0.0f, 0.0f, 2.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::RocketBooster}},
                {Vec3(0.0f, 0.0f, -2.0f), Vec3(0.0f, 0.0f, -1.0f), {Category::RocketBooster}}
            }, 500.0f, 0.0f, 200.0f, true},

        Part{"Fuel Tank 3", "A huge fuel tank.", "assets/models/spaceship/fuel_3.fbx", Category::FuelTank,
            std::vector<Attachment>{
                {Vec3(0.0f, 4.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::FuelTank, Category::Structural}},
                {Vec3(0.0f, -4.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank, Category::Engine}},

                {Vec3(2.3f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {Category::RocketBooster}},
                {Vec3(-2.3f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {Category::RocketBooster}},
                {Vec3(0.0f, 0.0f, 2.3f), Vec3(0.0f, 0.0f, 1.0f), {Category::RocketBooster}},
                {Vec3(0.0f, 0.0f, -2.3f), Vec3(0.0f, 0.0f, -1.0f), {Category::RocketBooster}}
            }, 600.0f, 0.0f, 275.0f, true},

        Part{"Rocket Booster", "A basic rocket booster.", "assets/models/spaceship/booster.fbx", Category::Engine,
            std::vector<Attachment>{
                {Vec3(1.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {Category::FuelTank}},
                {Vec3(-1.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {Category::Engine}}
            }, 150.0f, 650.0f, 0.0f, false, Category::RocketBooster},

        Part{"Engine", "A basic engine.", "assets/models/spaceship/engine.fbx", Category::Engine,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::FuelTank, Category::Structural}}
            }, 100.0f, 2000.0f, 0.0f},
        Part{"Engine 3x", "A more powerful engine.", "assets/models/spaceship/engine_2.fbx", Category::Engine,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::FuelTank}}
            }, 125.0f, 2400.0f, 0.0f},
        Part{"Module Engine", "An engine for travel module", "assets/models/spaceship/mengine.fbx", Category::Engine,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Structural}},
                {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::LandingGear}}
            }, 50.0f, 500.0f, 0.0f, false, Category::ModuleEngine}, // shows under Engine, but only mates with attach points that allow Category::ModuleEngine

        Part{"Structural-5", "A structural part with 5 attachment points.", "assets/models/spaceship/structural_5.fbx", Category::Structural,
            std::vector<Attachment>{
                // 5 attachments, two on x, two on z, one on y
                {Vec3(2.0f, 2.4f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {Category::ModuleEngine, Category::Structural, Category::Electrical}},
                {Vec3(-2.0f, 2.4f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {Category::ModuleEngine, Category::Structural, Category::Electrical}},
                {Vec3(0.0f, 2.4f, 2.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::ModuleEngine, Category::Structural, Category::Electrical}},
                {Vec3(0.0f, 2.4f, -2.0f), Vec3(0.0f, 0.0f, -1.0f), {Category::ModuleEngine, Category::Structural, Category::Electrical}},
                {Vec3(0.0f, 4.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Cockpit}},

                // special ones only for solar panels:
                {Vec3(2.0f, 2.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Electrical}},
                {Vec3(-2.0f, 2.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Electrical}},
                {Vec3(0.0f, 2.0f, 2.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Electrical}},
                {Vec3(0.0f, 2.0f, -2.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Electrical}},

                // connection in the bottom - accepts a continuing fuel tank, or landing gear
                {Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank, Category::Engine}}
            }, 75.0f, 0.0f, 0.0f},

        Part{"Panel", "Simple panel", "assets/models/spaceship/panel.fbx", Category::Structural,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Structural}}
            }, 10.0f, 0.0f, 0.0f},

        Part{"Landing Gear", "A basic landing gear.", "assets/models/spaceship/landinggear.fbx", Category::Structural,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::ModuleEngine}}
            }, 25.0f, 0.0f, 0.0f, false, Category::LandingGear}, 

        Part{"Cockpit", "A basic cockpit.", "assets/models/spaceship/cockpit.fbx", Category::Cockpit,
            std::vector<Attachment>{
                {Vec3(0.0f, -0.75f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Structural}}
            }, 50.0f, 0.0f, 0.0f},

        Part{"Cockpit 2", "", "assets/models/spaceship/cockpit_2.fbx", Category::Cockpit,
            std::vector<Attachment>{
                {Vec3(0.0f, -0.75f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Structural}}
            }, 50.0f, 0.0f, 0.0f},

        Part{"Small Battery", "A small battery for electrical power.", "assets/models/spaceship/battery.fbx", Category::Electrical,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Structural}}
            }, 100.0f, 0.0f, 0.0f},

        Part{"Solar Panel", "A solar panel for generating electrical power.", "assets/models/spaceship/solarpanel.fbx", Category::Electrical,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::Structural}}
            }, 50.0f, 0.0f, 0.0f
        }
    };
}