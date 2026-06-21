#version 410 core

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

uniform mat4 u_invView;
uniform mat4 u_invProj;
uniform vec3  u_cameraPos;
uniform vec3  u_planetCenter;
uniform float u_planetRadius;
uniform float u_atmosphereRadius;
uniform float u_edgeFalloff;
uniform vec3  u_sunDir;
uniform vec3  u_rayleighCoeff;
uniform float u_rayleighScaleH;
uniform float u_sunIntensity;
uniform float u_near;
uniform float u_far;

const int   NUM_VIEW_SAMPLES  = 16;
const int   NUM_LIGHT_SAMPLES = 8;
const float PI = 3.14159265359;

vec2 raySphereIntersect(vec3 ro, vec3 rd, vec3 center, float radius) {
    vec3  oc = ro - center;
    float b  = dot(oc, rd);
    float c  = dot(oc, oc) - radius * radius;
    float h  = b * b - c;
    if (h < 0.0) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float rayleighDensity(vec3 point) {
    float h = length(point - u_planetCenter) - u_planetRadius;
    float thickness = u_atmosphereRadius - u_planetRadius;
    float fade = 1.0 - smoothstep(thickness - u_edgeFalloff, thickness, h);
    return exp(-max(h, 0.0) / u_rayleighScaleH) * fade;
}

float opticalDepth(vec3 origin, vec3 dir, float rayLen) {
    float stepSize = rayLen / float(NUM_LIGHT_SAMPLES);
    float depth    = 0.0;
    for (int i = 0; i < NUM_LIGHT_SAMPLES; ++i) {
        vec3 p = origin + dir * (float(i) + 0.5) * stepSize;
        depth += rayleighDensity(p) * stepSize;
    }
    return depth;
}

float lineariseDepth(float d) {
    float z_ndc = d * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near + z_ndc * (u_far - u_near));
}

void main() {
    vec4  clipRay  = vec4(TexCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4  viewRay  = u_invProj * clipRay;
    viewRay = vec4(viewRay.xy, -1.0, 0.0);
    vec3  worldRay = normalize((u_invView * viewRay).xyz);
    vec3 rayOrigin = u_cameraPos;
    vec3 rayDir    = worldRay;

    vec2 atmoHit = raySphereIntersect(rayOrigin, rayDir, u_planetCenter, u_atmosphereRadius);
    vec4 sceneColor = texture(screenTexture, TexCoords);

    if (atmoHit.x > atmoHit.y || atmoHit.y < 0.0) {
        FragColor = sceneColor;
        return;
    }

    float tMin = max(atmoHit.x, 0.0);
    float tMax = atmoHit.y;
    
    vec2 planetHit = raySphereIntersect(rayOrigin, rayDir, u_planetCenter, u_planetRadius);
    if (planetHit.x > 0.0) {
        tMax = min(tMax, planetHit.x);
    }

    float rawDepth = texture(depthTexture, TexCoords).r;
    float sceneLinear = lineariseDepth(rawDepth);
    if (rawDepth > 0.0 && sceneLinear < tMax) {
        tMax = sceneLinear;
    }

    if (tMin >= tMax) {
        FragColor = sceneColor;
        return;
    }

    float segmentLen = tMax - tMin;
    float stepSize   = segmentLen / float(NUM_VIEW_SAMPLES);
    float cosTheta = dot(rayDir, u_sunDir);
    float phase    = rayleighPhase(cosTheta);
    vec3  scatteredLight  = vec3(0.0);
    float viewOpticalDepth = 0.0;

    for (int i = 0; i < NUM_VIEW_SAMPLES; ++i) {
        float t = tMin + (float(i) + 0.5) * stepSize;
        vec3  p = rayOrigin + rayDir * t;

        float density = rayleighDensity(p);
        viewOpticalDepth += density * stepSize;
        vec2 sunHit = raySphereIntersect(p, u_sunDir, u_planetCenter, u_atmosphereRadius);
        float sunRayLen = (sunHit.y > 0.0) ? sunHit.y : 0.0;
        float sunOpticalDepth = opticalDepth(p, u_sunDir, sunRayLen);

        vec3 extinction = exp(-(u_rayleighCoeff * (viewOpticalDepth + sunOpticalDepth)));
        scatteredLight += density * stepSize * extinction;
    }

    scatteredLight *= u_rayleighCoeff * phase * u_sunIntensity;
    scatteredLight = scatteredLight / (scatteredLight + vec3(1.0));
    float alpha = clamp(length(scatteredLight) * 2.5, 0.0, 1.0);
    FragColor = vec4(sceneColor.rgb * (1.0 - alpha) + scatteredLight, 1.0);
}