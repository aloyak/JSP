#version 410 core

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2D blueprintImageTexture;
uniform sampler2D drawMaskTexture;

uniform mat4 u_invView;
uniform mat4 u_invProj;
uniform vec3 u_cameraPos;

uniform float u_near;
uniform float u_far;
uniform float u_interiorSensitivity = 0.25;
uniform float u_objectDarkness = 0.7;

vec3 getViewPos(vec2 uv) {
    float d = texture(depthTexture, uv).r;
    vec4 ndc = vec4(uv * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);
    vec4 view = u_invProj * ndc;
    return view.xyz / view.w;
}

vec3 getNormal(vec2 uv, vec2 ts) {
    vec3 p0 = getViewPos(uv);
    vec3 p1 = getViewPos(uv + vec2(ts.x, 0.0));
    vec3 p2 = getViewPos(uv + vec2(0.0, ts.y));
    return normalize(cross(p1 - p0, p2 - p0));
}

void main() {
    float depth = texture(depthTexture, TexCoords).r;
    float mask = texture(drawMaskTexture, TexCoords).r;
    const float EPSILON = 1e-6;

    if (depth <= EPSILON || mask < 0.5) {
        FragColor = texture(blueprintImageTexture, TexCoords);
    }
    else {
        vec2 texelSize = 1.0 / textureSize(depthTexture, 0);
        float dx = texelSize.x;
        float dy = texelSize.y;

        float s00 = texture(depthTexture, TexCoords + vec2(-dx, -dy)).r;
        float s10 = texture(depthTexture, TexCoords + vec2( 0.0, -dy)).r;
        float s20 = texture(depthTexture, TexCoords + vec2( dx, -dy)).r;
        float s01 = texture(depthTexture, TexCoords + vec2(-dx,  0.0)).r;
        float s21 = texture(depthTexture, TexCoords + vec2( dx,  0.0)).r;
        float s02 = texture(depthTexture, TexCoords + vec2(-dx,  dy)).r;
        float s12 = texture(depthTexture, TexCoords + vec2( 0.0,  dy)).r;
        float s22 = texture(depthTexture, TexCoords + vec2( dx,  dy)).r;

        float edgeX = (s20 + 2.0 * s21 + s22) - (s00 + 2.0 * s01 + s02);
        float edgeY = (s02 + 2.0 * s12 + s22) - (s00 + 2.0 * s10 + s20);
        float depthEdge = length(vec2(edgeX, edgeY));

        vec3 n1 = getNormal(TexCoords, texelSize);
        vec3 n2 = getNormal(TexCoords + vec2(dx, dy), texelSize);
        vec3 n3 = getNormal(TexCoords + vec2(dx, 0.0), texelSize);
        vec3 n4 = getNormal(TexCoords + vec2(0.0, dy), texelSize);

        float normalEdge = length(n1 - n2) + length(n3 - n4);

        float edge = max(depthEdge * 50.0, normalEdge * u_interiorSensitivity);

        vec4 bpTex = texture(blueprintImageTexture, TexCoords);
        vec4 objColor = texture(screenTexture, TexCoords);

        vec3 objectBaseColor = bpTex.rgb * u_objectDarkness;
        vec3 finalColor = mix(objectBaseColor, vec3(1.0), clamp(edge, 0.0, 1.0));

        FragColor = vec4(finalColor, objColor.a);
    }
}