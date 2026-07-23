#include "gamemodes/explore.h"

// IMPORTANT TODOs:
// Custom physics world for when inside a planet, to allow walking on the surface while the planet is rotating and moving in orbit
// far future: chunked collision mesh per planet, not just a sphere collider

void ExploreMode::OnEnter() {
    m_game.showHelp = false;
    m_game.showSettings = false;
    m_game.timeScale = 1.0f;

    m_game.GetEngine().getInput().setCursorMode(true);

    m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
    m_camera->addComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 250000.0f); // ouch

    auto& settings = m_game.settingsManager.Get();
    m_freeCamera = new FreeCamera(m_camera);
    m_freeCamera->setSensitivity(settings.mouseSens);

    m_gravityBasedCamera = new GravityBasedCamera(m_camera, &m_game);
    m_gravityBasedCamera->setSensitivity(settings.mouseSens);

    setupPlanets();
    setupBlackHole();
}

void ExploreMode::setupPlanets() {
    m_planets = GetPlanetsFromScene(
        m_game,
        m_game.GetEngine().getSceneManager().getActiveScene(),
        m_camera
    );
}

void ExploreMode::setupBlackHole() {
    m_blackHole = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("BlackHole")->get();

    m_blackHole->removeComponent<RenderComponent>();
    m_blackHoleShader = &m_game.GetEngine().getRenderer().addPostProcessor(
        "assets/shaders/default_vert.glsl", 
        "assets/shaders/blackhole_frag.glsl"
    );    
}

void ExploreMode::OnExit() {
    if (m_blackHole) {
        m_game.GetEngine().getRenderer().removePostProcessor(m_blackHoleShader);
        m_blackHole = nullptr;
    }

    delete m_freeCamera;
    delete m_gravityBasedCamera;

    m_game.GetEngine().getInput().setCursorMode(false);
}

void ExploreMode::Update() {
    updateLods(m_game.GetEngine().getDeltaTime());

    // DEBUG
    if (m_game.GetEngine().getInput().isKeyPressed(KEY_F1)) {
        m_gravityBasedCamera->syncPlayerToCamera();
        m_useFreeCamera = !m_useFreeCamera;
    }

    if (!m_game.GetEngine().getInput().isKeyDown(KEY_LALT)) {
        m_game.GetEngine().getInput().setCursorMode(true);
        if (m_useFreeCamera) {
            m_freeCamera->Update(&m_game.GetEngine().getInput(), m_game.GetEngine().getDeltaTime());
        } else {
            m_gravityBasedCamera->Update(&m_game.GetEngine().getInput(), m_game.GetEngine().getDeltaTime());
        }
    } else 
        m_game.GetEngine().getInput().setCursorMode(false);
    
    // update free camera speed with scroll wheel
    float scrollDelta = m_game.GetEngine().getInput().getScrollDelta().y;
    if (scrollDelta != 0.0f) { 
        float speed = m_freeCamera->getMoveSpeed();
        speed *= std::pow(1.15f, scrollDelta);
        speed = std::clamp(speed, 0.1f, 10000.0f);
        m_freeCamera->setMoveSpeed(speed);
    }

    if (m_blackHoleShader) {
        auto* camComp = m_camera->getComponent<CameraComponent>();
        m_blackHoleShader->setMat4("u_invView", camComp->getCamera().getInvViewMatrix(m_camera->transform));
        m_blackHoleShader->setMat4("u_invProj", camComp->getCamera().getInvProjMatrix());
        m_blackHoleShader->setFloat("u_near", camComp->getNear());
        m_blackHoleShader->setFloat("u_far", camComp->getFar());

        m_blackHoleShader->setFloat("u_time", m_game.GetEngine().getTime());
        m_blackHoleShader->setFloat("u_swirlStrength", 0.1f);
                
        m_blackHoleShader->setVec3("u_center", m_blackHole->transform.position);
        m_blackHoleShader->setFloat("u_radius", m_blackHole->transform.scale.x);
        m_blackHoleShader->setFloat("u_lightWrapRadius", 300.0f);
        m_blackHoleShader->setFloat("u_glowIntensity", 0.0f);
        m_blackHoleShader->setFloat("u_distortionStrength", 0.6f);
        m_blackHoleShader->setVec3("u_glowColor", Vec3(1.0f, 5.0f, 5.0f));
    }

    drawDebugInfo();
}

void ExploreMode::drawDebugInfo() {
    ImGui::Begin("DEBUG");
    ImGui::SliderFloat("Time Scale", &m_game.timeScale, 0.0f, 100.0f);
    ImGui::SliderFloat("Gravity Scale", &m_gravityScale, 0.0f, 1000.0f);
    ImGui::Text("Free Camera Speed: %.2f", m_freeCamera->getMoveSpeed());
    ImGui::Text("Body Velocity: %.2f", m_gravityBasedCamera->getVelocity());
    ImGui::Text("Gravity Vector: (%.2f, %.2f, %.2f)", 
        m_gravityBasedCamera->getGravityVector().x, 
        m_gravityBasedCamera->getGravityVector().y, 
        m_gravityBasedCamera->getGravityVector().z
    );
    ImGui::Separator();
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", 
        m_camera->transform.position.x, 
        m_camera->transform.position.y, 
        m_camera->transform.position.z
    );
    ImGui::Text("Camera Rotation: (%.2f, %.2f, %.2f)", 
        m_camera->transform.rotation.x, 
        m_camera->transform.rotation.y, 
        m_camera->transform.rotation.z
    );
    ImGui::Separator();
    ImGui::Text("Body Rotation: (%.2f, %.2f, %.2f)", 
        m_gravityBasedCamera->getPlayer()->transform.rotation.x, 
        m_gravityBasedCamera->getPlayer()->transform.rotation.y, 
        m_gravityBasedCamera->getPlayer()->transform.rotation.z
    );

    ImGui::Text("Gravity Strenght (Gs): %.2f", m_gravityBasedCamera->getGravityVector().length());
    if (ImGui::Button("Return")) {
        m_game.GetUI().loadMainMenu();
    }

    ImGui::Separator();

    if (m_gravityBasedCamera->getClosestPlanet()) {
        AtmosphereParams& atmo = m_gravityBasedCamera->getClosestPlanet()->getComponent<PlanetComponent>()->getAtmosphere();
        if (ImGui::SliderFloat("thickness", &atmo.thickness, 1.0f, 1000.0f)) {}

        Vec3& rc = atmo.rayleighCoeff;
        static constexpr float k_rayleighMax = 0.08f; // displayable ceiling
        float skyColor[3] = {
            std::fmin(rc.x / k_rayleighMax, 1.0f),
            std::fmin(rc.y / k_rayleighMax, 1.0f),
            std::fmin(rc.z / k_rayleighMax, 1.0f)
        };
        if (ImGui::ColorEdit3("scatter", skyColor,
                ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueWheel)) {
            float maxC = std::fmax(skyColor[0], std::fmax(skyColor[1], skyColor[2]));
            if (maxC < 1e-5f) maxC = 1.0f;
            rc.x = (skyColor[0] / maxC) * k_rayleighMax;
            rc.y = (skyColor[1] / maxC) * k_rayleighMax;
            rc.z = (skyColor[2] / maxC) * k_rayleighMax;
        }

        float maxCoeff = std::fmax(rc.x, std::fmax(rc.y, rc.z));
        float scatterStrength = maxCoeff / k_rayleighMax; // [0,1]
        if (ImGui::SliderFloat("scatter", &scatterStrength, 0.01f, 1.0f)) {
            float prevMax = std::fmax(rc.x, std::fmax(rc.y, rc.z));
            if (prevMax > 1e-6f) {
                float scale = (scatterStrength * k_rayleighMax) / prevMax;
                rc.x *= scale;
                rc.y *= scale;
                rc.z *= scale;
            }
        }

        if (ImGui::SliderFloat("sharpness", &atmo.edgeFalloff, 0.0f, 1200.0f,
            "%.2f", ImGuiSliderFlags_Logarithmic)) {}

        if (ImGui::SliderFloat("density", &atmo.density, 0.0f, 10.0f,
            "%.2f", ImGuiSliderFlags_Logarithmic)) {}
    }

    ImGui::End();
}

void ExploreMode::LateUpdate() {
    updateOrbits(m_game.timeScale, m_game.GetEngine().getDeltaTime());
    updateGravityVector();
    updateGlobalLighting();

    if (m_camera->transform.position.length() > m_playerMaxOriginDist) 
        resetOrigin();
}

void ExploreMode::updateOrbits(float timeScale, float dt) {
    m_orbitTime += static_cast<double>(dt) * static_cast<double>(timeScale);

    Vec3 blackHolePos = m_blackHole ? m_blackHole->transform.position : Vec3(0.0f, 0.0f, 0.0f);

    // Pass 1: planets orbiting the black hole directly (orbitParent empty)
    for (auto& planet : m_planets) {
        if (!planet.component) continue;
        planet.component->update(dt);
        const auto& params = planet.component->getPlanetParams();
        if (params.orbitParent.empty()) {
            planet.component->updateOrbit(m_orbitTime, blackHolePos);
        }
    }

    // Pass 2: planets orbiting another planet, resolved after pass 1 so the
    // parent's freshly-updated position is used as this orbit's focus
    for (auto& planet : m_planets) {
        if (!planet.component) continue;
        const auto& params = planet.component->getPlanetParams();
        if (params.orbitParent.empty()) continue;

        Vec3 focus = blackHolePos;
        for (auto& parentCandidate : m_planets) {
            if (parentCandidate.name == params.orbitParent && parentCandidate.entity) {
                focus = parentCandidate.entity->transform.position;
                break;
            }
        }

        planet.component->updateOrbit(m_orbitTime, focus);
    }
}

void ExploreMode::updateLods(float dt) {
    for (auto& planet : m_planets) {
        if (!planet.entity || !planet.component) continue;

        float dist = (planet.entity->transform.position - m_camera->transform.position).length();
        bool useHighLod = dist < LODDistance;

        if (!planet.entity->hasComponent<RenderComponent>()) return;

        if (useHighLod) {
            planet.entity->removeComponent<RenderComponent>();
            planet.entity->addComponent<RenderComponent>(planet.highLodModelPath);
        } else {
            planet.entity->removeComponent<RenderComponent>();
            planet.entity->addComponent<RenderComponent>(planet.lowLodModelPath);
        }
    }
}

void ExploreMode::updateGravityVector() {
    float influenceRadius = m_gravityBasedCamera->getInfluenceRadius();
    Vec3 playerPos = m_camera->transform.position;

    Vec3 gravitySum(0.0f, 0.0f, 0.0f);

    for (auto& planet : m_planets) {
        if (!planet.entity || !planet.component) continue;

        const auto& params = planet.component->getPlanetParams();

        Vec3 toPlanet = planet.entity->transform.position - playerPos;
        float dist = toPlanet.length();
        if (dist <= 0.0001f) continue;

        float surfaceDist = dist - params.radius;
        if (surfaceDist > influenceRadius) continue;

        Vec3 dir = toPlanet / dist;

        float strength = params.mass / (dist * dist);

        gravitySum = gravitySum + dir * strength;
    }

    m_gravityBasedCamera->setGravityVector(gravitySum * m_gravityScale);
}

void ExploreMode::updateGlobalLighting() {
    Entity* lightSource = m_game.GetEngine().getSceneManager().getActiveScene()->getEntityByName("VestaStar")->get();

    for (auto& planet : m_planets) {
        if (!planet.entity || !planet.component) continue;

        auto* planetComp = planet.entity->getComponent<PlanetComponent>();
        if (!planetComp) continue;
       
        planetComp->getPlanetParams().sunDir = 
            (lightSource->transform.position - planet.entity->transform.position).normalize();
    }
}

void ExploreMode::resetOrigin() {
    Vec3 offset = m_camera->transform.position;

    for (auto& entity : m_game.GetEngine().getSceneManager().getActiveScene()->getEntities()) {
        if (entity.get() == m_camera) continue;

        // if it has a rigidbody use teleport instead
        if (entity->hasComponent<RigidbodyComponent>()) {
            auto* rb = entity->getComponent<RigidbodyComponent>();
            if (rb) {
                rb->teleport(entity->transform.position - offset, entity->transform.rotation);
                continue;
            }
        } else {
            entity->transform.position = entity->transform.position - offset;
        }
    }

    m_camera->transform.position = Vec3(0.0f, 0.0f, 0.0f);
}