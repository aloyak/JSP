#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Per-instance model matrix — uploaded by Mesh::drawInstanced via glVertexAttribDivisor
layout (location = 5) in vec4 aInstanceMat0;
layout (location = 6) in vec4 aInstanceMat1;
layout (location = 7) in vec4 aInstanceMat2;
layout (location = 8) in vec4 aInstanceMat3;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 TexCoord;
out mat3 vTBN;

uniform mat4 u_View;
uniform mat4 u_Projection;

// Vertex snap still works — it operates in clip space after projection, same as before
uniform bool  u_VertexSnap;
uniform float u_SnapIntensity;

void main() {
    mat4 model = mat4(aInstanceMat0, aInstanceMat1, aInstanceMat2, aInstanceMat3);

    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 pos      = u_Projection * u_View * worldPos;

    if (u_VertexSnap) {
        pos.xyz  = pos.xyz / pos.w;
        pos.xy   = floor(pos.xy * u_SnapIntensity) / u_SnapIntensity;
        pos.xyz *= pos.w;
    }

    gl_Position = pos;
    vWorldPos   = worldPos.xyz;
    TexCoord    = aTexCoord;

    // Normal matrix derived from the per-instance model matrix — identical logic to vert.glsl
    mat3 normalMat = mat3(transpose(inverse(model)));
    vec3 N = normalize(normalMat * aNormal);
    vec3 T = normalize(normalMat * aTangent);
    T      = normalize(T - dot(T, N) * N);   // Gram-Schmidt re-orthogonalise
    vec3 B = normalize(cross(N, T));

    vNormal = N;
    vTBN    = mat3(T, B, N);
}