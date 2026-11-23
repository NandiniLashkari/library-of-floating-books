#version 410 core
// scene.vert - runs once per vertex for everything in the library.
// Job: move the vertex to the screen and prepare the data the fragment
// shader needs for lighting, texturing, normal mapping and shadows.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aTangent;   // xyz tangent, w = bitangent sign

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpace;   // world -> moonlight's view (for shadow mapping)

uniform int mapping;       // how texture coordinates are made (see scene.frag)
uniform vec2 uvScale;      // texture repeats per world unit

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;
out vec2 vUV;
out vec4 vLightSpacePos;

// ---- GOURAUD shading: lighting is worked out HERE, once per vertex, and the
// rasteriser blends the results across the triangle. ----
uniform int shadingModel;          // 0 Phong (per pixel), 1 Gouraud (per vertex), 2 Flat
uniform vec3 cameraPos;
uniform vec3 moonDirection;
uniform vec3 moonColor;
const int MAX_POINT_LIGHTS = 16;
uniform int numPointLights;
uniform vec3 pointPos[MAX_POINT_LIGHTS];
uniform vec3 pointColor[MAX_POINT_LIGHTS];
uniform float shininess;
uniform float specularStrength;

out vec3 vMoonDiffuse;    // moonlight kept separate so the fragment shader
out vec3 vMoonSpecular;   // can still apply the shadow to it
out vec3 vPointDiffuse;
out vec3 vPointSpecular;

// Same Blinn-Phong maths as scene.frag, but evaluated at a vertex.
void vertexLight(vec3 N, vec3 V, vec3 L, vec3 color, inout vec3 diff, inout vec3 spec) {
    float NdotL = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float s = (NdotL > 0.0) ? pow(max(dot(N, H), 0.0), shininess) : 0.0;
    diff += color * NdotL;
    spec += color * s * specularStrength;
}

const int MAP_MESH_UV = 0;
const int MAP_BOX     = 2;

void main() {
    vec4 world = model * vec4(aPos, 1.0);
    vWorldPos = world.xyz;

    // Normals use the inverse-transpose so non-uniform scaling keeps them
    // perpendicular to the surface.
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vNormal = normalize(normalMatrix * aNormal);

    vec3 tangentObj = aTangent.xyz;
    float handedness = aTangent.w;
    vUV = aUV * uvScale;

    if (mapping == MAP_BOX) {
        // BOX MAPPING for the stretched cubes (walls, shelves, pillars):
        // project the texture onto each face using the object's own size, so
        // a long shelf repeats the wood instead of stretching one copy.
        vec3 size = vec3(length(model[0].xyz), length(model[1].xyz),
                         length(model[2].xyz));
        vec3 q = aPos * size;                    // position in real units
        vec3 n = aNormal;
        vec3 bitangentObj;
        if (abs(n.y) > 0.5) {                    // top / bottom faces
            vUV = q.xz;  tangentObj = vec3(1, 0, 0);  bitangentObj = vec3(0, 0, 1);
        } else if (abs(n.x) > 0.5) {             // left / right faces
            vUV = vec2(q.z, -q.y);  tangentObj = vec3(0, 0, 1);  bitangentObj = vec3(0, -1, 0);
        } else {                                 // front / back faces
            vUV = vec2(q.x, -q.y);  tangentObj = vec3(1, 0, 0);  bitangentObj = vec3(0, -1, 0);
        }
        vUV *= uvScale;
        handedness = dot(cross(n, tangentObj), bitangentObj) < 0.0 ? -1.0 : 1.0;
    }

    // Tangent frame in world space (Gram-Schmidt keeps T perpendicular to N).
    vec3 T = normalize(mat3(model) * tangentObj);
    T = normalize(T - dot(T, vNormal) * vNormal);
    vTangent = T;
    vBitangent = cross(vNormal, T) * handedness;

    vLightSpacePos = lightSpace * world;

    vMoonDiffuse = vMoonSpecular = vPointDiffuse = vPointSpecular = vec3(0.0);
    if (shadingModel == 1) {
        vec3 V = normalize(cameraPos - vWorldPos);
        vertexLight(vNormal, V, normalize(moonDirection), moonColor,
                    vMoonDiffuse, vMoonSpecular);
        for (int i = 0; i < numPointLights; ++i) {
            vec3 toLight = pointPos[i] - vWorldPos;
            float d = length(toLight);
            float attenuation = 1.0 / (1.0 + 0.5 * d + 0.6 * d * d);
            vertexLight(vNormal, V, toLight / d, pointColor[i] * attenuation,
                        vPointDiffuse, vPointSpecular);
        }
    }
    gl_Position = projection * view * world;
}
