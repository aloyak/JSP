#version 410 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

uniform mat4 u_invView;
uniform mat4 u_invProj;
uniform vec3 u_cameraPos;

uniform float u_distance;
uniform float u_density;
uniform vec3 u_color;
uniform float u_maxFog;
uniform float u_skyFalloff;

out vec4 FragColor;

void main() {
    vec3 sceneColor = texture(screenTexture, TexCoords).rgb;
    float depth = texture(depthTexture, TexCoords).r;

    vec4 ndc = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_invView * u_invProj * ndc;
    worldPos /= worldPos.w;

    vec3 viewDir = worldPos.xyz - u_cameraPos;
    float dist = length(viewDir);
    viewDir = normalize(viewDir);
    
    float fogFactor = 1.0 - exp(-max(dist - u_distance, 0.0) * u_density);
    
    float horizonFactor = exp(-max(viewDir.y, 0.0) * u_skyFalloff);
    fogFactor *= horizonFactor;
    
    fogFactor = clamp(fogFactor, 0.0, u_maxFog);

    FragColor = vec4(mix(sceneColor, u_color, fogFactor), 1.0);
}