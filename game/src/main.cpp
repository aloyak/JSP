#include "engine/engine.h"
#include "engine/input/input.h"
#include "engine/scene/sceneManager.h"

#include "engine/components/entity.h"
#include "engine/components/cameraComponent.h"
#include "engine/components/rendererComponent.h"

#include "engine/utils/logger.h"

#include <iostream>

int main() {
    // Engine Setup -------------

    Engine engine(1600, 900, "JSP");

    engine.getRenderer().setupRenderTarget(600, 400);
    engine.getRenderer().setPixelArt(true, 8);

    Input& input = engine.getInput();
    SceneManager& sm = engine.getSceneManager();

    input.setCursorMode(true); // true = locked
    engine.getWindow().setFullscreen(false);
    engine.getWindow().enableVSync(false);

    engine.getRenderer().setMinimumAmbientLight(1.7f);

    // ---------------------------

    Entity* orbitAround = engine.createEntity("OrbitAround");
    orbitAround->transform.position = Vec3(0.0f, 0.0f, 0.0f);
    orbitAround->addComponent<RenderComponent>("assets/models/earth.fbx");
    orbitAround->transform.scale = Vec3(0.1f, 0.1f, 0.1f);

    Entity* camera = engine.createEntity("Camera");
    camera->transform.position = Vec3(0.0f, 0.0f, 5.0f);
    camera->addComponent<CameraComponent>(45.0f, 1600.0f/900.0f, 0.1f, 10000.0f);
    engine.setActiveCamera(camera);

    engine.run([&]() {
        float time = engine.getTime();
        float radius = 250.0f;
        
        camera->transform.position = Vec3(
            orbitAround->transform.position.x + radius * cosf(time), 
            orbitAround->transform.position.y, 
            orbitAround->transform.position.z + radius * sinf(time)
        );
        
        camera->getComponent<CameraComponent>()->lookAt(*orbitAround);
    });

    return 0;
}