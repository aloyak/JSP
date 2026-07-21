#pragma once

#include "engine/utils/logger.h"
#include "engine/components/entity.h"
#include "engine/components/rigidbodyComponent.h"
#include "components/planetComponent.h"
#include "engine/input/input.h"
#include <cmath>
#include <algorithm>

// kind of like the first person view but affected by multiple gravity sources

// TODO: NOTE: The "m_gravInfluenceRadius" is just the maximum distance from the player where it will calculate
// the gravity vector based on nearby celestial bodies
// TODO: NOTE: On the other hand "m_playerEnterPlanetRadius" is the distance, ONLY counting the closest celestial body, 
// where the player will be considered "on" the planet. This means smooth camera alignment, friction determination
// and, most importantly, make the player be inside the players "local" physics world, so that the player can walk on the planet's
// surface even tho the surface is globally moving

#include "engine/physics/world.h"

class GravityBasedCamera {
private:
    Game* m_game = nullptr;

    Entity* m_camera = nullptr;
    Entity* m_player = nullptr;

    Entity* m_closestValidPlanet = nullptr;

    float m_moveSpeed = 75.0f;
    float m_sensitivity = 1.0f;

    float m_playerHeight = 1.0f;

    float m_gravInfluenceRadius = 1000.0f;
    float m_playerEnterPlanetRadius = 100.0f;

    float m_alignmentSpeed = 5.0f;
    float m_maxSlopeAngle = 45.0f; 

    Vec3 m_velocity = Vec3(0.0f);
    Vec3 m_gravity = Vec3(0.0f);


    Quat m_alignRot = Quat();

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    PhysicsWorld* m_localWorld = nullptr;
public:
    GravityBasedCamera(Entity* camera, Game* game) : m_camera(camera), m_game(game) {
        m_player = game->GetEngine().getSceneManager().getActiveScene()->createEntity("Player");
        auto* rigidbody = m_player->addComponent<RigidbodyComponent>();
        m_player->transform.rotation = Vec3(0.0f, 0.0f, 0.0f);
        rigidbody->setPhysicsWorld(&game->GetEngine().getPhysicsWorld());
        rigidbody->setBodyType(RigidbodyComponent::BodyType::Dynamic);
        rigidbody->setColliderType(RigidbodyComponent::ColliderType::Capsule);
        rigidbody->setFreezeRotationX(true);
        rigidbody->setFreezeRotationY(true);
        rigidbody->setFreezeRotationZ(true);
        rigidbody->setFriction(0.1f);

        m_localWorld = &game->GetEngine().createPhysicsWorld();
    }

    ~GravityBasedCamera() {
        m_game->GetEngine().destroyPhysicsWorld(*m_localWorld);
    }

    void Update(Input* input, float dt) {
        if (!m_camera || !m_player) return;

        m_closestValidPlanet = getClosestPlanet();

        Vec2 mouseDelta = input->getMouseDelta();
        m_pitch += mouseDelta.y * m_sensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        m_yaw += mouseDelta.x * m_sensitivity;
        if (m_yaw > 180.0f) m_yaw -= 360.0f;
        if (m_yaw < -180.0f) m_yaw += 360.0f;

        m_player->getComponent<RigidbodyComponent>()->getPhysicsWorld()->setGravity(m_gravity * 50.0f);

        alignWithGravity(m_closestValidPlanet, dt);

        constexpr float DEG2RAD = 3.1415926535f / 180.0f;
        Quat pitchQuat = fromAxisAngle(Vec3(0.0f, 0.0f, -1.0f), m_pitch * DEG2RAD);
        Quat yawQuat = fromAxisAngle(Vec3(0.0f, -1.0f, 0.0f), m_yaw * DEG2RAD);

        Quat finalRot = normalize(m_alignRot * yawQuat * pitchQuat);

        Vec3 finalForward = finalRot * Vec3(1.0f, 0.0f, 0.0f);
        Vec3 finalUp = finalRot * Vec3(0.0f, 1.0f, 0.0f);
        
        float pitch = std::asin(std::clamp(finalForward.y, -1.0f, 1.0f)) * (180.0f / 3.1415926535f);
        float yaw = std::atan2(finalForward.z, finalForward.x) * (180.0f / 3.1415926535f);
        
        Vec3 unrolledRight = cross(Vec3(0.0f, 1.0f, 0.0f), finalForward);
        if (unrolledRight.length() < 0.001f) {
            unrolledRight = Vec3(0.0f, 0.0f, -1.0f);
        } else {
            unrolledRight = unrolledRight.normalize();
        }
        Vec3 unrolledUp = cross(finalForward, unrolledRight).normalize();
        float roll = std::atan2(dot(cross(unrolledUp, finalUp), finalForward), dot(unrolledUp, finalUp)) * (180.0f / 3.1415926535f);

        m_camera->transform.rotation = Vec3(pitch, yaw, roll);

        Vec3 forward = m_camera->transform.forward();
        Vec3 right = m_camera->transform.right();
        Vec3 up = m_camera->transform.up();

        if (input->isKeyDown(KEY_W)) {
            m_player->getComponent<RigidbodyComponent>()->applyForce(forward * m_moveSpeed);
        }
        if (input->isKeyDown(KEY_S)) {
            m_player->getComponent<RigidbodyComponent>()->applyForce(forward * (-m_moveSpeed));
        }
        if (input->isKeyDown(KEY_A)) {
            m_player->getComponent<RigidbodyComponent>()->applyForce(right * m_moveSpeed);    
        }
        if (input->isKeyDown(KEY_D)) {
            m_player->getComponent<RigidbodyComponent>()->applyForce(right * (-m_moveSpeed));
        }

        // TODO: roll
        if (input->isKeyDown(KEY_Q)) {
            m_player->getComponent<RigidbodyComponent>()->applyTorque(up * m_moveSpeed);
        }
        if (input->isKeyDown(KEY_E)) {
            m_player->getComponent<RigidbodyComponent>()->applyTorque(up * (-m_moveSpeed));
        }

        m_camera->transform.position = m_player->transform.position + (up * m_playerHeight);

        m_velocity = m_player->getComponent<RigidbodyComponent>()->getLinearVelocity();
        calculateDrag(m_closestValidPlanet);
    }

    Entity* getClosestPlanet() {
        Entity* closest = nullptr;
        float closestSurfaceDist = m_playerEnterPlanetRadius;

        for (auto& entity : m_game->GetEngine().getSceneManager().getActiveScene()->getEntities()) {
            if (entity.get() == m_player) continue;
            if (!entity->hasComponent<PlanetComponent>()) continue;

            float planetRadius = entity->getComponent<PlanetComponent>()->getPlanetParams().radius;
            float distance = (entity->transform.position - m_player->transform.position).length();
            float surfaceDistance = distance - planetRadius;

            if (surfaceDistance < closestSurfaceDist) {
                closestSurfaceDist = surfaceDistance;
                closest = entity.get();
            }
        }
        //Logger::info("Closest planet: " + (closest ? closest->name : "None") + ", Surface distance: " + std::to_string(closestSurfaceDist));
        return closest;
    }

    void alignWithGravity(Entity* planet, float dt) {
        Vec3 targetUp;
        if (planet) {
            targetUp = (m_player->transform.position - planet->transform.position).normalize();
        } else if (length(m_gravity) > 0.0001f) {
            targetUp = (-m_gravity).normalize();
        } else {
            return;
        }

        Vec3 currentUp = m_alignRot * Vec3(0.0f, 1.0f, 0.0f);
        float d = std::clamp(dot(currentUp, targetUp), -1.0f, 1.0f);

        if (d > 0.9999f) return; // already aligned, nothing to do

        Vec3 axis;
        float angle;
        if (d < -0.9999f) {
            Vec3 arbitrary = (std::abs(currentUp.x) < 0.9f) ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 0.0f, 1.0f);
            axis = cross(currentUp, arbitrary).normalize();
            angle = 3.1415926535f;
        } else {
            axis = cross(currentUp, targetUp).normalize();
            angle = std::acos(d);
        }

        Quat fullDelta = fromAxisAngle(axis, angle);
        float t = std::clamp(1.0f - std::exp(-m_alignmentSpeed * dt), 0.0f, 1.0f);
        Quat stepDelta = Quat::slerp(Quat(), fullDelta, t);

        m_alignRot = normalize(stepDelta * m_alignRot);
    }

    void calculateDrag(Entity* planet) {
        if (!planet) return;
        
        Vec3 dragForce = -m_velocity * 0.5f; 
        m_player->getComponent<RigidbodyComponent>()->applyForce(dragForce);
        
        // TODO: use each planet's params
        // planet->getComponent<PlanetComponent>()->getAtmosphere().drag
    }

    void syncPlayerToCamera() {
        if (!m_camera || !m_player) return;
        Vec3 newPos = m_camera->transform.position - (m_camera->transform.up() * m_playerHeight);
        m_player->getComponent<RigidbodyComponent>()->teleport(newPos, Vec3(0.0f, 0.0f, 0.0f));
    }

    void setSensitivity(float newSensitivity) { m_sensitivity = newSensitivity; }
    float getSensitivity() const { return m_sensitivity; }

    void setMoveSpeed(float newSpeed) { m_moveSpeed = newSpeed; }
    float getMoveSpeed() const { return m_moveSpeed; }

    float getInfluenceRadius() const { return m_gravInfluenceRadius; }
    float getVelocity() const { return m_velocity.length(); }

    void setGravityVector(const Vec3& gravity) { m_gravity = gravity; }
    Vec3 getGravityVector() const { return m_gravity; }

    Entity* getPlayer() const { return m_player; }

    float getDistanceToClosestPlanet() const {
        if (!m_closestValidPlanet) return -1.0f;
        float planetRadius = m_closestValidPlanet->getComponent<PlanetComponent>()->getPlanetParams().radius;
        float distance = (m_closestValidPlanet->transform.position - m_player->transform.position).length();
        return distance - planetRadius; // surface distance
    }
};