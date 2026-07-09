#version 410 core

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

uniform mat4 u_invView;
uniform mat4 u_invProj;
uniform float u_near;
uniform float u_far;

uniform vec3  u_center;
uniform float u_radius;

uniform float u_lightWrapRadius;
uniform vec3  u_glowColor;
uniform float u_glowIntensity;
uniform float u_distortionStrength;

uniform float u_time;
uniform float u_swirlStrength;

vec2 raySphereIntersect(vec3 ro, vec3 rd, vec3 center, float radius) {
    vec3  oc = ro - center;
    float b  = dot(oc, rd);
    float c  = dot(oc, oc) - radius * radius;
    float h  = b * b - c;
    if (h < 0.0) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

float lineariseDepth(float d) {
    float z_ndc = d * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near + z_ndc * (u_far - u_near));
}

void main() {
    vec4 clipRay  = vec4(TexCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay  = u_invProj * clipRay;
    viewRay = vec4(viewRay.xy, -1.0, 0.0);
    vec3 rayDir    = normalize((u_invView * viewRay).xyz);
    vec3 rayOrigin = u_invView[3].xyz; // camera position lives in the invView translation column

    vec4  sceneColor  = texture(screenTexture, TexCoords);
    float rawDepth    = texture(depthTexture, TexCoords).r;
    float sceneLinear = lineariseDepth(rawDepth);
    bool  hasSceneGeo = rawDepth > 0.0 && rawDepth < 1.0;

    float outerRadius = u_radius + max(u_lightWrapRadius, 0.0);

    vec2 horizonHit = raySphereIntersect(rayOrigin, rayDir, u_center, u_radius);
    bool horizonInFront = horizonHit.x > 0.0 && (!hasSceneGeo || horizonHit.x < sceneLinear);

    if (horizonInFront) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 outerHit = raySphereIntersect(rayOrigin, rayDir, u_center, outerRadius);
    bool outerInFront = outerHit.x > 0.0 && outerHit.y > 0.0 &&
                         (!hasSceneGeo || outerHit.x < sceneLinear);

    if (u_lightWrapRadius <= 0.0 || !outerInFront) {
        FragColor = sceneColor;
        return;
    }

    vec3  oc        = u_center - rayOrigin;
    float tClosest   = max(dot(oc, rayDir), 0.0);
    vec3  closestPt  = rayOrigin + rayDir * tClosest;
    float impactParam = length(u_center - closestPt);

    float band = clamp((impactParam - u_radius) / max(u_lightWrapRadius, 0.0001), 0.0, 1.0);

    mat4 proj = inverse(u_invProj);
    mat4 view = inverse(u_invView);
    vec4 clipCenter = proj * view * vec4(u_center, 1.0);
    vec2 screenCenter = (clipCenter.xy / clipCenter.w) * 0.5 + 0.5;

    vec2  dirToCenter = screenCenter - TexCoords;
    float distToCenter = length(dirToCenter);
    vec2  dirNorm = distToCenter > 0.0001 ? dirToCenter / distToCenter : vec2(0.0);

    float swirlAngle = u_swirlStrength * (1.0 - band) * (1.0 + 0.25 * sin(u_time));
    float s = sin(swirlAngle);
    float c = cos(swirlAngle);
    vec2 dirSwirled = vec2(dirNorm.x * c - dirNorm.y * s,
                            dirNorm.x * s + dirNorm.y * c);

    float pull = u_distortionStrength * pow(1.0 - band, 2.0);
    vec2 warpedUV = TexCoords + dirSwirled * pull;
    warpedUV = clamp(warpedUV, 0.0, 1.0);

    vec3 warpedColor = texture(screenTexture, warpedUV).rgb;

    float rim  = pow(1.0 - band, 3.0);
    vec3  glow = u_glowColor * rim * u_glowIntensity;

    vec3 finalColor = warpedColor * (1.0 - rim * 0.5) + glow;

    FragColor = vec4(finalColor, 1.0);
}