#pragma once

#include "engine/utils/logger.h"
#include "engine/components/entity.h"
#include "engine/components/rigidbodyComponent.h"
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

    float m_moveSpeed = 50.0f;
    float m_sensitivity = 1.0f;

    float m_playerHeight = 1.0f;

    float m_gravInfluenceRadius = 1000.0f;
    float m_playerEnterPlanetRadius = 200.0f;

    float m_alignmentSpeed = 5.0f;
    float m_maxSlopeAngle = 45.0f; 

    Vec3 m_velocity = Vec3(0.0f);
    Vec3 m_gravity = Vec3(0.0f);

    PhysicsWorld* m_localWorld = nullptr;
public:
    GravityBasedCamera(Entity* camera, Game* game) : m_camera(camera), m_game(game) {
        m_player = game->GetEngine().getSceneManager().getActiveScene()->createEntity("Player");
        auto* rigidbody = m_player->addComponent<RigidbodyComponent>();
        rigidbody->setPhysicsWorld(&game->GetEngine().getPhysicsWorld());
        rigidbody->setBodyType(RigidbodyComponent::BodyType::Dynamic);
        rigidbody->setColliderType(RigidbodyComponent::ColliderType::Capsule);
        rigidbody->setFreezeRotationX(true);
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
        m_camera->transform.rotation.x -= mouseDelta.y * m_sensitivity;
        m_camera->transform.rotation.x = std::clamp(m_camera->transform.rotation.x, -89.0f, 89.0f);
        m_camera->transform.rotation.y += mouseDelta.x * m_sensitivity;

        // DEBUG: added some weight
        m_player->getComponent<RigidbodyComponent>()->getPhysicsWorld()->setGravity(m_gravity * 100.0f);

        Vec3 forward = m_camera->transform.forward();
        Vec3 right = m_camera->transform.right();

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

        alignWithGravity(m_closestValidPlanet, dt);

        m_camera->transform.position = m_player->transform.position + (m_camera->transform.up() * m_playerHeight);

        m_velocity = m_player->getComponent<RigidbodyComponent>()->getLinearVelocity();
        //calculateDrag();
    }

    Entity* getClosestPlanet() {
        for (auto& entity : m_game->GetEngine().getSceneManager().getActiveScene()->getEntities()) {
            if (entity->hasComponent<RigidbodyComponent>()) {
                auto* rigidbody = entity->getComponent<RigidbodyComponent>();
                if (rigidbody->getBodyType() == RigidbodyComponent::BodyType::Static) {
                    float distance = (entity->transform.position - m_player->transform.position).length();
                    if (distance < m_playerEnterPlanetRadius) {
                        return entity.get();
                    }
                }
            }
        }
        return nullptr;
    }

    void alignWithGravity(Entity* planet, float dt) {
        if (!planet) return;
        Logger::info("aligning cam");
        auto* rigidbody = m_player->getComponent<RigidbodyComponent>();
        rigidbody->setFreezeRotationX(false);
        rigidbody->setFreezeRotationZ(false);

        Vec3 playerPos = m_player->transform.position;
        Vec3 targetUp = (playerPos - planet->transform.position).normalize();

        Vec3 origin = m_player->transform.localToWorld(Vec3(0, 0, 0));
        Vec3 currentUp = (m_player->transform.localToWorld(Vec3(0, 1, 0)) - origin).normalize();

        Vec3 axis = cross(currentUp, targetUp);
        float d = dot(currentUp, targetUp);
        Quat qRot;

        if (axis.length() > 0.0001f) {
            axis = axis.normalize();
            float angle = std::acos(std::clamp(d, -1.0f, 1.0f));
            float sinA = std::sin(angle * 0.5f);
            float cosA = std::cos(angle * 0.5f);
            qRot = Quat(axis.x * sinA, axis.y * sinA, axis.z * sinA, cosA);
        } else {
            if (d < 0.0f) {
                Vec3 perp = cross(currentUp, Vec3(1.0f, 0.0f, 0.0f));
                if (perp.length() < 0.001f) {
                    perp = cross(currentUp, Vec3(0.0f, 0.0f, 1.0f));
                }
                perp = perp.normalize();
                qRot = Quat(perp.x, perp.y, perp.z, 0.0f);
            } else {
                qRot = Quat(0.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        Quat qPlayerCurrent = Quat::fromEulerAngles(m_player->transform.rotation.x, m_player->transform.rotation.y, m_player->transform.rotation.z);
        Quat qPlayerTarget = qRot * qPlayerCurrent;
        Quat qPlayerNew = Quat::slerp(qPlayerCurrent, qPlayerTarget, std::clamp(m_alignmentSpeed * dt, 0.0f, 1.0f));
        m_player->transform.rotation = toEulerAngles(qPlayerNew);

        Quat qCamCurrent = Quat::fromEulerAngles(m_camera->transform.rotation.x, m_camera->transform.rotation.y, m_camera->transform.rotation.z);
        Quat qCamTarget = qRot * qCamCurrent;
        Quat qCamNew = Quat::slerp(qCamCurrent, qCamTarget, std::clamp(m_alignmentSpeed * dt, 0.0f, 1.0f));
        m_camera->transform.rotation = toEulerAngles(qCamNew);
    }

    void calculateDrag() {
        // should be based on the planets atmosphere's drag but for now its gonna be fixed
        // apply a contrary force to the player's velocity to simulate drag
        Vec3 dragForce = -m_velocity * 0.75f; 
        m_player->getComponent<RigidbodyComponent>()->applyForce(dragForce);
    }

    void syncPlayerToCamera() {
        if (!m_camera || !m_player) return;
        Vec3 newPos = m_camera->transform.position - (m_camera->transform.up() * m_playerHeight);
        m_player->getComponent<RigidbodyComponent>()->teleport(newPos, m_camera->transform.rotation);
    }

    void setSensitivity(float newSensitivity) { m_sensitivity = newSensitivity; }
    float getSensitivity() const { return m_sensitivity; }

    void setMoveSpeed(float newSpeed) { m_moveSpeed = newSpeed; }
    float getMoveSpeed() const { return m_moveSpeed; }

    float getInfluenceRadius() const { return m_gravInfluenceRadius; }
    float getVelocity() const { return m_velocity.length(); }

    void setGravityVector(const Vec3& gravity) { m_gravity = gravity; }
    Vec3 getGravityVector() const { return m_gravity; }
};