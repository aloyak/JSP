#version 410 core

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

uniform mat4 u_invView;
uniform mat4 u_invProj;
uniform vec3 u_cameraPos;

uniform vec3  u_cageCenter;
uniform float u_cageSize;
uniform float u_barrierHeight; 

uniform float u_time;
uniform float u_near;
uniform float u_far;

const vec3  BASE_COLOR   = vec3(0.05, 0.6, 0.7);
const vec3  LINE_COLOR   = vec3(0.3, 1.0, 0.75);
const float GRID_SCALE   = 3.0;
const float LINE_WIDTH   = 0.05;
const float MAX_RAY_T    = 5000.0;
const float FADE_PORTION = 0.6;

uniform bool u_enabled = true;

float lineariseDepth(float d) {
    float z_ndc = d * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near + z_ndc * (u_far - u_near));
}

vec3 shadeWall(float t, vec3 rayOrigin, vec3 rayDir, vec3 rayDirN,
               float sceneLinear, bool depthValid, vec3 baseColor) {
    if (depthValid && sceneLinear < t) return baseColor; // real geometry occludes this wall

    vec3 hitPos = rayOrigin + rayDir * t;
    vec3 localPos = hitPos - u_cageCenter;

    float heightLocal = hitPos.y - u_cageCenter.y;
    if (heightLocal < 0.0 || heightLocal > u_barrierHeight) return baseColor;


    float heightFade = 1.0 - smoothstep(u_barrierHeight * FADE_PORTION, u_barrierHeight, heightLocal);

    vec3 normal;
    vec2 faceUV;
    if (abs(localPos.x) > abs(localPos.z)) {
        normal = vec3(sign(localPos.x), 0.0, 0.0);
        faceUV = vec2(localPos.z, localPos.y);
    } else {
        normal = vec3(0.0, 0.0, sign(localPos.z));
        faceUV = vec2(localPos.x, localPos.y);
    }

    float fresnel = pow(1.0 - abs(dot(normal, -rayDirN)), 2.0);

    float heightRatio = clamp(heightLocal / u_barrierHeight, 0.0, 1.0);
    float lineThickness = mix(0.7, 0.05, heightRatio); 
    
    float pattern = step(fract(faceUV.y * 3.0 - u_time * 0.5), lineThickness);

    float rim = (1.0 - smoothstep(0.0, u_barrierHeight * 0.15, abs(heightLocal - u_barrierHeight))) * 0.6;

    vec3 cageColor = mix(BASE_COLOR, LINE_COLOR, pattern);
    cageColor += LINE_COLOR * (pattern * 0.8 + rim + fresnel * 0.3);

    float alpha = clamp(pattern * 0.95 + fresnel * 0.08 + rim, 0.0, 1.0) * heightFade;

    return mix(baseColor, cageColor, alpha);
}

vec2 columnIntersect(vec3 ro, vec3 rd, vec2 centerXZ, float halfSize) {
    vec3 boxMin = vec3(centerXZ.x - halfSize, -1e7, centerXZ.y - halfSize);
    vec3 boxMax = vec3(centerXZ.x + halfSize,  1e7, centerXZ.y + halfSize);
    vec3 invRd = 1.0 / rd;
    vec3 t0 = (boxMin - ro) * invRd;
    vec3 t1 = (boxMax - ro) * invRd;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tnear = max(max(tmin.x, tmin.y), tmin.z);
    float tfar  = min(min(tmax.x, tmax.y), tmax.z);
    if (tnear > tfar || tfar < 0.0) return vec2(0.0, -1.0);
    return vec2(tnear, tfar);
}

void main() {
    if (!u_enabled) {
        FragColor = texture(screenTexture, TexCoords);
        return;
    }

    vec4 clipRay = vec4(TexCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay = u_invProj * clipRay;
    viewRay = vec4(viewRay.xy, -1.0, 0.0);

    vec3 rayDir = (u_invView * viewRay).xyz;
    vec3 rayOrigin = u_cameraPos;

    vec3 rayDirN = normalize(rayDir);

    vec4 sceneColor = texture(screenTexture, TexCoords);

    vec2 hit = columnIntersect(rayOrigin, rayDir, u_cageCenter.xz, u_cageSize);

    if (hit.y < 0.0 || hit.y > MAX_RAY_T) {
        FragColor = sceneColor;
        return;
    }

    bool cameraInside = hit.x < 0.0;

    float rawDepth = texture(depthTexture, TexCoords).r;
    float sceneLinear = lineariseDepth(rawDepth);
    bool depthValid = rawDepth > 0.0;

    vec3 color = sceneColor.rgb;

    color = shadeWall(hit.y, rayOrigin, rayDir, rayDirN, sceneLinear, depthValid, color);

    if (!cameraInside) {
        color = shadeWall(hit.x, rayOrigin, rayDir, rayDirN, sceneLinear, depthValid, color);
    }

    FragColor = vec4(color, 1.0);
}