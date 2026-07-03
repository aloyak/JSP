// This should only be used for spaceship building!
#pragma once

#include "engine/render/postprocess/postProcessor.h"
#include "engine/components/entity.h"
#include "game.h"

class Blueprint {
private:
    Game* m_game = nullptr;

    Entity* m_camera = nullptr;
    PostProcessor* m_blueprint;

    float m_fixedDistance = 20.0f;

public:
    Blueprint(Entity* camera, Game &game) 
        : m_game(&game),
          m_camera(camera) {

        m_blueprint = &m_game->GetEngine().getRenderer().addPostProcessor(
              "assets/shaders/default_vert.glsl", "assets/shaders/blueprint_frag.glsl"
        );
    }

    ~Blueprint() {
        m_game->GetEngine().getRenderer().removePostProcessor(m_blueprint);
    }

    void update(float deltaTime) {
        
    }
};