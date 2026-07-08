#include "components/planetComponent.h"

PlanetComponent::PlanetComponent(Game& game, float period, float radius, float mass)
    : m_planetParams{Vec3(0.0f, 0.0f, 0.0f), period, radius, mass}, m_game(&game) {}

PlanetComponent::~PlanetComponent() {
    if (m_atmosphere && m_game) {
        m_game->GetEngine().getRenderer().removePostProcessor(m_atmosphere);
        m_atmosphere = nullptr;
    }
    if (m_water && m_game) {
        m_game->GetEngine().getRenderer().removePostProcessor(m_water);
        m_water = nullptr;
    }
}

void PlanetComponent::initialize() {
    if (!entity) return;

    Transform& transform = entity->transform;
    transform.scale = Vec3(m_planetParams.radius, m_planetParams.radius, m_planetParams.radius);
    m_inicialRot = transform.rotation;
    transform.rotation = Vec3(-90.0f, m_inicialRot.y, m_inicialRot.z); 
}

void PlanetComponent::update(float dt) {
    if (!entity || !isEnabled) return;

    float speed = 0.0f;
    if (m_planetParams.period != 0.0f) {
        speed = (360.0f / (m_planetParams.period * 3600.0f)); 
    } 
    m_currentRotation += speed * dt;

    entity->transform.rotation.y = m_currentRotation + m_inicialRot.y;

    if (m_hasAtmosphere && m_atmosphere && entity && m_game) {
        auto& engine = m_game->GetEngine();

        m_atmosphere->setVec3 ("u_planetCenter",     entity->transform.position);
        m_atmosphere->setFloat("u_planetRadius",     m_planetParams.radius);
        m_atmosphere->setFloat("u_atmosphereRadius", m_planetParams.radius + m_atmosphereParams.thickness);

        Entity* cam = engine.getActiveCamera();
        if (cam) {
            m_atmosphere->setVec3("u_cameraPos", cam->transform.position);

            auto* camComp = cam->getComponent<CameraComponent>();
            if (camComp) {
                m_atmosphere->setMat4("u_invView", camComp->getCamera().getInvViewMatrix(cam->transform));
                m_atmosphere->setMat4("u_invProj", camComp->getCamera().getInvProjMatrix());
                m_atmosphere->setFloat("u_near", camComp->getNear());
                m_atmosphere->setFloat("u_far",  camComp->getFar());
            }
        }

        m_atmosphere->setVec3("u_sunDir", m_planetParams.sunDir);

        float atmoRadius = m_planetParams.radius + m_atmosphereParams.thickness;
        m_atmosphere->setVec3 ("u_rayleighCoeff", m_atmosphereParams.rayleighCoeff);
        m_atmosphere->setFloat("u_rayleighScaleH", atmoRadius * 0.08f);
        m_atmosphere->setFloat("u_sunIntensity", m_planetParams.sunIntensity);
        m_atmosphere->setFloat("u_edgeFalloff", m_atmosphereParams.edgeFalloff);
    }
    if (m_hasWater && m_water && entity && m_game) {
        auto& engine = m_game->GetEngine();

        m_water->setVec3 ("u_planetCenter", entity->transform.position);
        m_water->setFloat("u_planetRadius", m_planetParams.radius);
        m_water->setFloat("u_waterLevel",   m_waterParams.level);
        m_water->setVec3 ("u_waterColorA",   m_waterParams.colorA);
        m_water->setVec3 ("u_waterColorB",   m_waterParams.colorB);
        m_water->setFloat("u_depthMultiplier", m_waterParams.depthMultiplier);
        m_water->setFloat("u_alphaMultiplier", m_waterParams.alphaMultiplier);
        
        m_water->setVec3 ("u_sunDir",       m_planetParams.sunDir);
        m_water->setFloat("u_sunIntensity", m_planetParams.sunIntensity);

        Entity* cam = engine.getActiveCamera();
        if (cam) {
            m_water->setVec3("u_cameraPos", cam->transform.position);

            auto* camComp = cam->getComponent<CameraComponent>();
            if (camComp) {
                m_water->setMat4("u_invView", camComp->getCamera().getInvViewMatrix(cam->transform));
                m_water->setMat4("u_invProj", camComp->getCamera().getInvProjMatrix());
                m_water->setFloat("u_near", camComp->getNear());
                m_water->setFloat("u_far",  camComp->getFar());
            }
        }
    }
}

std::unique_ptr<Component> PlanetComponent::clone() const {
    return std::make_unique<PlanetComponent>(*this);
}

PlanetParams& PlanetComponent::getPlanetParams() { 
    return m_planetParams; 
}

void PlanetComponent::applyScale() {
    if (entity) {
        entity->transform.scale = Vec3(m_planetParams.radius, m_planetParams.radius, m_planetParams.radius);
    }
}

void PlanetComponent::setHasAtmosphere(bool hasAtmosphere) { 
    m_hasAtmosphere = hasAtmosphere; 
    if (m_hasAtmosphere) {
        if (!m_atmosphere && m_game) {
            m_atmosphere = &m_game->GetEngine().getRenderer().addPostProcessor(
                "assets/shaders/default_vert.glsl", "assets/shaders/atmosphere_frag.glsl"
            );
        }
    } else {
        if (m_atmosphere && m_game) {
            m_game->GetEngine().getRenderer().removePostProcessor(m_atmosphere);
            m_atmosphere = nullptr;
        }
    }
    reorderLayers();
}

bool PlanetComponent::hasAtmosphere() const { 
    return m_hasAtmosphere; 
}

AtmosphereParams& PlanetComponent::getAtmosphere() { 
    return m_atmosphereParams; 
}

void PlanetComponent::setHasWater(bool hasWater) { 
    m_hasWater = hasWater; 
    if (m_hasWater) {
        if (!m_water && m_game) {
            m_water = &m_game->GetEngine().getRenderer().addPostProcessor(
                "assets/shaders/default_vert.glsl", "assets/shaders/water_frag.glsl"
            );
        }
    } else {
        if (m_water && m_game) {
            m_game->GetEngine().getRenderer().removePostProcessor(m_water);
            m_water = nullptr;
        }
    }
    reorderLayers();
}

bool PlanetComponent::hasWater() const { 
    return m_hasWater; 
}

WaterParams& PlanetComponent::getWater() { 
    return m_waterParams; 
}

void PlanetComponent::loadFromFile(const std::string& filepath, std::shared_ptr<Texture>& paintTexture) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::error("Failed to open " + filepath);
        return;
    }
    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        Logger::error("JSON parse error: " + std::string(e.what()));
        return;
    }
    if (j.contains("name") && entity) entity->name = j["name"];
    if (j.contains("mass")) m_planetParams.mass = j["mass"];
    if (j.contains("hasWater")) setHasWater(j["hasWater"]);
    if (j.contains("waterLevel")) m_waterParams.level = j["waterLevel"];
    if (j.contains("waterColorA")) m_waterParams.colorA = Vec3(j["waterColorA"][0], j["waterColorA"][1], j["waterColorA"][2]);
    if (j.contains("waterColorB")) m_waterParams.colorB = Vec3(j["waterColorB"][0], j["waterColorB"][1], j["waterColorB"][2]);
    if (j.contains("waterDepthMultiplier")) m_waterParams.depthMultiplier = j["waterDepthMultiplier"];
    if (j.contains("waterAlphaMultiplier")) m_waterParams.alphaMultiplier = j["waterAlphaMultiplier"];
    if (j.contains("radius")) m_planetParams.radius = j["radius"];
    if (j.contains("period")) m_planetParams.period = j["period"];
    if (j.contains("hasAtmosphere")) setHasAtmosphere(j["hasAtmosphere"]);
    if (j.contains("atmosphereThickness")) m_atmosphereParams.thickness = j["atmosphereThickness"];
    if (j.contains("sunIntensity")) m_planetParams.sunIntensity = j["sunIntensity"];
    if (j.contains("sunDir")) m_planetParams.sunDir = Vec3(j["sunDir"][0], j["sunDir"][1], j["sunDir"][2]);
    if (j.contains("rayleighCoeff")) m_atmosphereParams.rayleighCoeff = Vec3(j["rayleighCoeff"][0], j["rayleighCoeff"][1], j["rayleighCoeff"][2]);
    if (j.contains("edgeFalloff")) m_atmosphereParams.edgeFalloff = j["edgeFalloff"];

    initialize();

    if (j.contains("texture") && entity) {
        std::string texPath = j["texture"];
        if (!texPath.empty() && texPath.front() == '/') texPath = texPath.substr(1);
        std::string resolvedTexPath = Path::resolve(texPath);
        if (std::filesystem::exists(resolvedTexPath)) {
            paintTexture.reset(Texture::createPaintable(resolvedTexPath));
            auto* renderComponent = entity->getComponent<RenderComponent>();
            if (renderComponent && paintTexture) renderComponent->setPaintableTexture(paintTexture);
        } else {
            Logger::error("Texture not found: " + resolvedTexPath);
        }
    }
    if (j.contains("geometry") && entity) {
        std::string geomPath = j["geometry"];
        if (!geomPath.empty() && geomPath.front() == '/') geomPath = geomPath.substr(1);
        std::string resolvedGeomPath = Path::resolve(geomPath);
        std::ifstream geomFile(resolvedGeomPath, std::ios::binary);
        if (geomFile.is_open()) {
            auto* renderComp = entity->getComponent<RenderComponent>();
            if (renderComp && renderComp->getModel()) {
                bool anyModified = false;
                for (auto& mesh : renderComp->getModel()->getMeshes()) {
                    if (!mesh.isDynamic()) continue;
                    for (auto& v : mesh.vertices) {
                        if (geomFile.read(reinterpret_cast<char*>(&v.Position), sizeof(v.Position))) anyModified = true;
                    }
                    if (anyModified) {
                        mesh.recalculateNormals();
                        mesh.updateVertexBuffer();
                    }
                }
            }
            geomFile.close();
        } else {
            Logger::error("Geometry file not found: " + resolvedGeomPath);
        }
    }
}

void PlanetComponent::reorderLayers() {
    if (!m_game) return;
    auto& renderer = m_game->GetEngine().getRenderer();

    if (m_water) {
        renderer.removePostProcessor(m_water);
        m_water = nullptr;
    }
    if (m_atmosphere) {
        renderer.removePostProcessor(m_atmosphere);
        m_atmosphere = nullptr;
    }

    if (m_hasWater) {
        m_water = &renderer.addPostProcessor(
            "assets/shaders/default_vert.glsl", "assets/shaders/water_frag.glsl"
        );
    }
    if (m_hasAtmosphere) {
        m_atmosphere = &renderer.addPostProcessor(
            "assets/shaders/default_vert.glsl", "assets/shaders/atmosphere_frag.glsl"
        );
    }
}