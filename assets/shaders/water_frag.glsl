#version 410 core

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture; 

uniform float u_near;
uniform float u_far;
uniform vec3  u_cameraPos;
uniform vec3  u_planetCenter;
uniform float u_planetRadius;
uniform mat4  u_invView;
uniform mat4  u_invProj;

uniform float u_waterLevel;
uniform float u_depthMultiplier;
uniform float u_alphaMultiplier;
uniform vec3  u_waterColorA;
uniform vec3  u_waterColorB;

uniform float u_sunIntensity;
uniform vec3  u_sunDir;

vec2 raySphereIntersect(vec3 ro, vec3 rd, vec3 center, float radius) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

// TODO: fix depth texture z fighting
void main() {
    vec4 sceneColor = texture(screenTexture, TexCoords);
    vec4 clipRay = vec4(TexCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay = u_invProj * clipRay;
    viewRay = vec4(viewRay.xy, -1.0, 0.0);
    vec3 rayDir = normalize((u_invView * viewRay).xyz);
    vec3 rayPos = u_cameraPos;

    vec2 hitInfo = raySphereIntersect(rayPos, rayDir, u_planetCenter, u_planetRadius + u_waterLevel);

    if (hitInfo.y < 0.0) {
        FragColor = sceneColor;
        return;
    }

    float dstToOcean = max(0.0, hitInfo.x);
    float dstThroughOcean = hitInfo.y - dstToOcean;

    float rawDepth = texture(depthTexture, TexCoords).r;
    
    vec4 ndc = vec4(TexCoords * 2.0 - 1.0, rawDepth * 2.0 - 1.0, 1.0);
    vec4 viewPos = u_invProj * ndc;
    viewPos /= viewPos.w;
    float sceneDepth = length(viewPos.xyz);
    
    if (rawDepth == 0.0) {
        sceneDepth = 1000000.0;
    }

    float oceanViewDepth = min(dstThroughOcean, sceneDepth - dstToOcean);

    if (oceanViewDepth > 0.0) {
        float opticalDepth01 = 1.0 - exp(-oceanViewDepth * u_depthMultiplier);
        float alpha = 1.0 - exp(-oceanViewDepth * u_alphaMultiplier);
        vec3 oceanCol = mix(u_waterColorA, u_waterColorB, opticalDepth01);

        vec3 surfaceNormal = normalize((rayPos + rayDir * dstToOcean) - u_planetCenter);
        float shadow = mix(0.15, 1.0, max(0.0, dot(surfaceNormal, normalize(u_sunDir))));
        oceanCol *= shadow;
        
        FragColor = vec4(mix(sceneColor.rgb, oceanCol, alpha), 1.0);
        return;
    } 
    FragColor = sceneColor;
}
