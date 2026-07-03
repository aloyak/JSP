#version 410 core

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

uniform mat4 u_invView;
uniform mat4 u_invProj;
uniform vec3 u_cameraPos;

// NOTE on convention: u_cageCenter.xz is the footprint center of the barrier,
// and u_cageCenter.y is the GROUND level (base) the barrier rises from.
uniform vec3  u_cageCenter;
uniform float u_cageSize;      // half-width of the barrier footprint on X/Z
uniform float u_barrierHeight; // how far up (from u_cageCenter.y) the barrier
                                // reaches before fading to fully transparent
uniform float u_time;
uniform float u_near;
uniform float u_far;

const vec3  BASE_COLOR   = vec3(0.05, 0.6, 0.7);
const vec3  LINE_COLOR   = vec3(0.3, 1.0, 0.75);
const float GRID_SCALE   = 3.0;
const float LINE_WIDTH   = 0.05;
const float MAX_RAY_T    = 5000.0; // treat farther-than-this hits as "no hit"
const float FADE_PORTION = 0.6;    // fraction of the height where fade begins

float lineariseDepth(float d) {
    float z_ndc = d * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near + z_ndc * (u_far - u_near));
}

// Shades the barrier surface at ray parameter t (one of the two wall
// crossings from columnIntersect) and composites it over `baseColor`.
// Returns baseColor unchanged if this particular wall is occluded by real
// scene geometry or is outside the barrier's height range - which lets the
// caller composite multiple walls back-to-front and have each one correctly
// "disappear" independently instead of an all-or-nothing decision for the
// whole pixel.
vec3 shadeWall(float t, vec3 rayOrigin, vec3 rayDir, vec3 rayDirN,
               float sceneLinear, bool depthValid, vec3 baseColor) {
    if (depthValid && sceneLinear < t) return baseColor; // real geometry occludes this wall

    vec3 hitPos = rayOrigin + rayDir * t;
    vec3 localPos = hitPos - u_cageCenter;

    // Height above ground, in [0, u_barrierHeight] to be visible at all.
    float heightLocal = hitPos.y - u_cageCenter.y;
    if (heightLocal < 0.0 || heightLocal > u_barrierHeight) return baseColor;

    // Full brightness near the ground, gradually transparent approaching
    // u_barrierHeight, fully transparent at/after it.
    float heightFade = 1.0 - smoothstep(u_barrierHeight * FADE_PORTION, u_barrierHeight, heightLocal);

    // Work out which of the 4 vertical faces we hit, so we get a stable
    // per-face UV and an outward-facing normal (used for the edge glow).
    vec3 normal;
    vec2 faceUV;
    if (abs(localPos.x) > abs(localPos.z)) {
        normal = vec3(sign(localPos.x), 0.0, 0.0);
        faceUV = vec2(localPos.z, localPos.y);
    } else {
        normal = vec3(0.0, 0.0, sign(localPos.z));
        faceUV = vec2(localPos.x, localPos.y);
    }

    // abs() makes this face-orientation agnostic, so it looks right whether
    // we're looking at the wall from outside or from inside.
    float fresnel = pow(1.0 - abs(dot(normal, -rayDirN)), 2.0);

    float heightRatio = clamp(heightLocal / u_barrierHeight, 0.0, 1.0);
    float lineThickness = mix(0.7, 0.05, heightRatio); 
    
    float pattern = step(fract(faceUV.y * 3.0 - u_time * 0.5), lineThickness);

    // Bright cap right at the fade-out edge, so the top reads as an
    // intentional energy rim rather than a hard clip.
    float rim = (1.0 - smoothstep(0.0, u_barrierHeight * 0.15, abs(heightLocal - u_barrierHeight))) * 0.6;

    vec3 cageColor = mix(BASE_COLOR, LINE_COLOR, pattern);
    cageColor += LINE_COLOR * (pattern * 0.8 + rim + fresnel * 0.3);

    float alpha = clamp(pattern * 0.95 + fresnel * 0.08 + rim, 0.0, 1.0) * heightFade;

    return mix(baseColor, cageColor, alpha);
}

// Intersect the ray with a box that is UNBOUNDED on Y — effectively an
// infinite vertical tube with a square footprint. Because it has no top or
// bottom faces, they can never be hit/rendered, from either inside or
// outside. Returns (tnear, tfar); returns y < 0 when there's no valid hit.
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
    vec4 clipRay = vec4(TexCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay = u_invProj * clipRay;
    viewRay = vec4(viewRay.xy, -1.0, 0.0);

    // IMPORTANT: do NOT normalize this. Its view-space Z is exactly -1 by
    // construction, which means that travelling a distance t along it changes
    // view-space depth by exactly t. That's what makes `t` from the box
    // intersection directly comparable to lineariseDepth()'s output below.
    // If we normalize it, t becomes a true Euclidean ray distance instead,
    // which only matches the linear scene depth for the dead-center pixel -
    // everywhere else the comparison silently goes wrong, more so the
    // further a pixel is from screen center (i.e. it gets worse with FOV/
    // viewing angle and looks "broken" in a view-dependent way).
    vec3 rayDir = (u_invView * viewRay).xyz;
    vec3 rayOrigin = u_cameraPos;

    // A properly normalized direction, kept separately, for angle-based
    // shading terms (e.g. fresnel) that need real unit-vector dot products.
    vec3 rayDirN = normalize(rayDir);

    vec4 sceneColor = texture(screenTexture, TexCoords);

    vec2 hit = columnIntersect(rayOrigin, rayDir, u_cageCenter.xz, u_cageSize);

    // No intersection at all, or the ray grazes the tube near-vertically
    // (tfar blows up towards infinity) -> render straight through, which is
    // exactly what we want where a top/bottom cap would otherwise have been.
    if (hit.y < 0.0 || hit.y > MAX_RAY_T) {
        FragColor = sceneColor;
        return;
    }

    bool cameraInside = hit.x < 0.0;

    float rawDepth = texture(depthTexture, TexCoords).r;
    float sceneLinear = lineariseDepth(rawDepth);
    bool depthValid = rawDepth > 0.0;

    vec3 color = sceneColor.rgb;

    // Far wall (hit.y): from outside, this is the interior back wall seen
    // through the translucent near wall; from inside, it's the only wall in
    // front of us. Always shaded first so it sits "behind" the near wall.
    color = shadeWall(hit.y, rayOrigin, rayDir, rayDirN, sceneLinear, depthValid, color);

    // Near wall (hit.x): only a distinct, closer surface when we're outside
    // the barrier. Shaded second so it composites on top of the far wall.
    if (!cameraInside) {
        color = shadeWall(hit.x, rayOrigin, rayDir, rayDirN, sceneLinear, depthValid, color);
    }

    FragColor = vec4(color, 1.0);
}