#include "gamemodes/explore.h"

void ExploreMode::OnEnter() {
    m_game.showHelp = false;
    m_game.showSettings = false;
    m_game.timeScale = 1.0f;

    m_game.GetEngine().getInput().setCursorMode(true);

    m_camera = m_game.GetEngine().getSceneManager().getActiveScene()->createEntity("Camera");
    m_camera->addComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 10000.0f);

    m_freeCamera = new FreeCamera(m_camera);
    auto& settings = m_game.settingsManager.Get();
    m_freeCamera->setSensitivity(settings.mouseSens);

    setupPlanets();
    setupBlackHole();
}

void ExploreMode::setupPlanets() {
    m_planets = GetPlanetsFromScene(
        m_game,
        m_game.GetEngine().getSceneManager().getActiveScene()
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

    m_game.GetEngine().getInput().setCursorMode(false);
}

void ExploreMode::Update() {
    m_freeCamera->Update(&m_game.GetEngine().getInput(), m_game.GetEngine().getDeltaTime());

    // update free camera speed with scroll wheel
    float scrollDelta = m_game.GetEngine().getInput().getScrollDelta().y;
    if (scrollDelta != 0.0f) { 
        float speed = m_freeCamera->getMoveSpeed();
        speed *= std::pow(1.15f, scrollDelta);
        speed = std::clamp(speed, 0.1f, 1000.0f);
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
}

void ExploreMode::LateUpdate() {
    updateOrbits(); // based on timescale, special case for vesta star
}

void ExploreMode::updateOrbits() {
}