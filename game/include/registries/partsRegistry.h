#pragma once

#include "engine/core/math.h"

#include <algorithm>
#include <string>
#include <vector>

enum class Category {
    Information,
    Customization,
    Engine,
    FuelTank,
    Cockpit,
    Structural,
    RocketBooster,
    Electrical,
    Utility
};

inline const char* CategoryToString(Category cat) {
    switch (cat) {
        case Category::Information:   return "Information";
        case Category::Customization: return "Customization";
        case Category::Engine:        return "Engine";
        case Category::FuelTank:      return "Fuel Tank";
        case Category::Cockpit:       return "Cockpit";
        case Category::Structural:    return "Structural";
        case Category::RocketBooster: return "Rocket Booster";
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
        case Category::RocketBooster: return "Bstr";
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
    Category category = Category::Cockpit;
    std::vector<Attachment> attachmentPoints;
    float mass = 10.0f;
    float thrust = 0.0f; 
    float fuelCapacity = 0.0f;
};

struct Spaceship {
    std::string name = "Unnamed Spaceship";
    std::string description = "No description.";
    std::vector<Part> parts;
    float mass = 0.0f;
    float thrust = 0.0f;
    float fuelCapacity = 0.0f;
};

inline std::vector<Part> CreateDefaultParts() {
    return {
        Part{"Fuel Tank", "A basic fuel tank.", "assets/models/spaceship/fuel.fbx", Category::FuelTank,
            std::vector<Attachment>{
                {Vec3(0.0f, 2.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::FuelTank, Category::Structural}},
                {Vec3(0.0f, -2.5f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank, Category::Engine}}
            }, 100.0f, 0.0f, 100.0f},

        Part{"Fuel Tank (large)", "A larger fuel tank.", "assets/models/spaceship/fuel_2.fbx", Category::FuelTank,
            std::vector<Attachment>{
                {Vec3(0.0f, 4.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {Category::FuelTank, Category::Structural}},
                {Vec3(0.0f, -4.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank, Category::Engine}},

                {Vec3(2.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {Category::RocketBooster}},
                {Vec3(-2.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {Category::RocketBooster}},
                {Vec3(0.0f, 0.0f, 2.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::RocketBooster}},
                {Vec3(0.0f, 0.0f, -2.0f), Vec3(0.0f, 0.0f, -1.0f), {Category::RocketBooster}}
            }, 250.0f, 0.0f, 200.0f},

        Part{"Rocket Booster", "A basic rocket booster.", "assets/models/spaceship/booster.fbx", Category::RocketBooster,
            std::vector<Attachment>{
                {Vec3(1.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {Category::FuelTank}},
                {Vec3(-1.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {Category::Engine}}
            }, 20.0f, 100.0f, 0.0f},

        Part{"Engine", "A basic engine.", "assets/models/spaceship/engine.fbx", Category::Engine,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::FuelTank}}
            }, 10.0f, 100.0f, 0.0f},
        Part{"Engine 3x", "A more powerful engine.", "assets/models/spaceship/engine_2.fbx", Category::Engine,
            std::vector<Attachment>{
                {Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), {Category::FuelTank}}
            }, 25.0f, 150.0f, 0.0f},

        Part{"Structural-5", "A structural part with 5 attachment points.", "assets/models/spaceship/structural_5.fbx", Category::Structural,
            std::vector<Attachment>{
                // 5 attachments, two on x, two on z, one on y
                {Vec3(1.5f, 2.5f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), {}},
                {Vec3(-1.5f, 2.5f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), {}},
                {Vec3(0.0f, 2.5f, 1.5f), Vec3(0.0f, 0.0f, 1.0f), {}},
                {Vec3(0.0f, 2.5f, -1.5f), Vec3(0.0f, 0.0f, -1.0f), {}},
                {Vec3(0.0f, 4.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), {}},
                
                // connection in the bottom
                {Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), {Category::FuelTank}}
            }, 5.0f, 0.0f, 0.0f}

    };
}