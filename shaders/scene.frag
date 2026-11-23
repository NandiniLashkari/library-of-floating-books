#version 410 core
// scene.frag - runs once per pixel (fragment) of every surface.
// 1. Texture mapping: decide the texture coordinates and read the textures.
// 2. Normal mapping: bend the surface normal using a normal map.
// 3. Blinn-Phong lighting from 1 moonlight + up to 16 point lights.
// 4. Shadow mapping for the moonlight.
// 5. Environment (cubemap) reflections and self-lit (emissive) objects.
// 6. Fog, then output HDR colour (tone mapping happens in post.frag).

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;
in vec2 vUV;
in vec4 vLightSpacePos;
in vec3 vMoonDiffuse;     // Gouraud: lighting already done per vertex
in vec3 vMoonSpecular;
in vec3 vPointDiffuse;
in vec3 vPointSpecular;

out vec4 FragColor;

// ---------------- textures (texture unit in brackets) ----------------
uniform sampler2D albedoMap;    // [0] base colour, sRGB
uniform sampler2D normalMap;    // [1] tangent-space normals
uniform sampler2D ormMap;       // [2] glTF metallic-roughness (G rough, B metal)
uniform samplerCube skybox;     // [3] night-sky cubemap
uniform sampler2D shadowMap;    // [4] depth seen from the moon
uniform sampler2D rugMap;       // [5] rug decal on the floor

// ---------------- what this object is ----------------
uniform int mapping;            // 0 mesh UV, 1 floor, 2 box, 3 window sky, 4 ceiling
uniform vec2 uvScale;
uniform vec3 baseColor;         // tint (or the colour when textures are off)
uniform int hasAlbedoMap;
uniform int hasNormalMap;
uniform int hasOrmMap;
uniform float shininess;        // Blinn-Phong exponent: bigger = smaller, sharper highlight
uniform float specularStrength;
uniform float reflectivity;     // 0 = none, 1 = mirror (environment mapping)
uniform float emissive;         // self-lit amount (flames, glowing dust)
uniform float normalStrength;   // how strongly the normal map bends the normal

// ---------------- lights ----------------
uniform vec3 ambientColor;
uniform vec3 moonDirection;     // direction TOWARDS the moon
uniform vec3 moonColor;         // (0,0,0) when the moon is switched off
const int MAX_POINT_LIGHTS = 16;
uniform int numPointLights;
uniform vec3 pointPos[MAX_POINT_LIGHTS];
uniform vec3 pointColor[MAX_POINT_LIGHTS];

// ---------------- camera + switches ----------------
uniform vec3 cameraPos;
uniform float time;
uniform int viewMode;           // 0 full, 1 ambient, 2 diffuse, 3 specular, 4 normals
uniform int shadingModel;       // 0 Phong (per pixel), 1 Gouraud (per vertex), 2 Flat (per face)
uniform int texturesOn;
uniform int normalMappingOn;
uniform int shadowsOn;
uniform int reflectionsOn;

const int MAP_MESH_UV = 0;
const int MAP_FLOOR   = 1;
const int MAP_BOX     = 2;
const int MAP_SKY     = 3;
const int MAP_CEILING = 4;

// ---------------------------------------------------------------------
// Shadow mapping: is this point hidden from the moon?
// Returns 1.0 = fully lit, 0.0 = fully in shadow.
float moonShadow(vec3 N, vec3 L) {
    if (shadowsOn == 0) return 1.0;

    // Light-space position -> [0,1] texture coordinates + depth.
    vec3 p = vLightSpacePos.xyz / vLightSpacePos.w;
    p = p * 0.5 + 0.5;
    if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0) return 1.0;

    // Bias avoids "shadow acne" (a surface shadowing itself); larger when
    // the light hits the surface at a grazing angle.
    float bias = max(0.0025 * (1.0 - dot(N, L)), 0.0006);

    // PCF: average 5x5 depth comparisons for soft edges.
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float lit = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float closest = texture(shadowMap, p.xy + vec2(x, y) * texel).r;
            lit += (p.z - bias > closest) ? 0.0 : 1.0;
        }
    }
    return lit / 25.0;
}

// Blinn-Phong for one light. Adds into diffuse and specular totals.
void blinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightColor, float shine,
                float specStr, vec3 specTint, inout vec3 diff, inout vec3 spec) {
    float NdotL = max(dot(N, L), 0.0);           // Lambert: facing the light?
    vec3 H = normalize(L + V);                   // halfway vector
    float s = (NdotL > 0.0) ? pow(max(dot(N, H), 0.0), shine) : 0.0;
    diff += lightColor * NdotL;
    spec += lightColor * specTint * s * specStr;
}

void main() {
    vec3 V = normalize(cameraPos - vWorldPos);

    // ---------------- 1. sky panels: sample the cubemap ----------------
    // Windows and the ceiling opening look out at the sky: the direction from
    // the eye through this pixel picks a point on the sky cube.
    bool isOculus = (mapping == MAP_CEILING && length(vWorldPos.xz) < 5.0);
    if (mapping == MAP_SKY || isOculus) {
        vec3 sky = texture(skybox, -V).rgb * 1.6;
        if (viewMode == 4) sky = vec3(0.5, 0.5, 1.0);
        FragColor = vec4(sky, 1.0);
        return;
    }

    // ---------------- 2. texture coordinates ----------------
    vec2 uv = vUV;
    vec3 T = vTangent, B = vBitangent, N0 = normalize(vNormal);
    if (mapping == MAP_FLOOR || mapping == MAP_CEILING) {
        // PLANAR MAPPING: the floor uses world x,z as its texture coordinates.
        uv = vWorldPos.xz * uvScale;
        T = vec3(1, 0, 0);
        B = vec3(0, 0, 1);
    }

    // ---------------- 3. read the material textures ----------------
    vec3 albedo = baseColor;
    float roughness = 0.6, metallic = 0.0;
    if (texturesOn == 1) {
        if (hasAlbedoMap == 1) albedo = texture(albedoMap, uv).rgb * baseColor;

        // Rug DECAL: one copy of the rug image stretched over a 14 x 14 area.
        if (mapping == MAP_FLOOR) {
            vec2 rugUV = vWorldPos.xz / 14.0 + 0.5;
            vec2 d = rugUV - 0.5;
            if (dot(d, d) < 0.25) {                       // inside the circle
                vec3 rug = texture(rugMap, rugUV).rgb;
                float edge = smoothstep(0.25, 0.235, dot(d, d));
                albedo = mix(albedo, rug, edge);
            }
        }
        if (hasOrmMap == 1) {
            vec3 orm = texture(ormMap, uv).rgb;
            roughness = orm.g;                            // glTF: green = roughness
            metallic = orm.b;                             //       blue  = metallic
        }
    }

    // ---------------- 4. normal mapping ----------------
    // FLAT shading: one normal per triangle. dFdx/dFdy give how the position
    // changes between neighbouring pixels; their cross product is the face normal.
    if (shadingModel == 2) {
        vec3 face = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
        N0 = (dot(face, N0) < 0.0) ? -face : face;
    }
    vec3 N = N0;
    if (normalMappingOn == 1 && hasNormalMap == 1 && shadingModel == 0) {
        vec3 tn = texture(normalMap, uv).rgb * 2.0 - 1.0;   // [0,1] -> [-1,1]
        tn.xy *= normalStrength;                            // exaggerate the bumps
        mat3 TBN = mat3(normalize(T), normalize(B), N0);    // tangent -> world
        N = normalize(TBN * tn);
    }
    if (!gl_FrontFacing) N = -N;   // cube faces are seen from inside sometimes

    // Material response. Rough surfaces get a wide, weak highlight.
    float shine = shininess;
    float specStr = specularStrength;
    if (hasOrmMap == 1 && texturesOn == 1) {
        shine = mix(96.0, 8.0, roughness);          // roughness map = specular map
        specStr = specularStrength * (1.0 - roughness * 0.8);
    }
    vec3 specTint = mix(vec3(1.0), albedo, metallic);   // metals tint highlights

    // ---------------- 5. lighting ----------------
    vec3 ambient = ambientColor;
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);

    // a) moonlight: directional, casts shadows
    vec3 Lm = normalize(moonDirection);
    float shadow = moonShadow(N0, Lm);
    vec3 dMoon = vec3(0.0), sMoon = vec3(0.0);
    blinnPhong(N, V, Lm, moonColor, shine, specStr, specTint, dMoon, sMoon);
    diffuseLight += dMoon * shadow;
    specularLight += sMoon * shadow;

    // b) point lights: candles + magic light, fading with distance
    for (int i = 0; i < numPointLights && shadingModel != 1; ++i) {
        vec3 toLight = pointPos[i] - vWorldPos;
        float d = length(toLight);
        vec3 L = toLight / d;
        // attenuation = 1 / (constant + linear*d + quadratic*d^2)
        float attenuation = 1.0 / (1.0 + 0.5 * d + 0.6 * d * d);
        blinnPhong(N, V, L, pointColor[i] * attenuation, shine, specStr,
                   specTint, diffuseLight, specularLight);
    }

    // Gouraud: throw away the per-pixel result and use the per-vertex one,
    // smoothly interpolated across the triangle by the rasteriser.
    if (shadingModel == 1) {
        diffuseLight = vMoonDiffuse * shadow + vPointDiffuse;
        specularLight = vMoonSpecular * shadow + vPointSpecular;
    }

    vec3 color;
    if (viewMode == 1)      color = albedo * ambient * 4.0;          // ambient only (brightened)
    else if (viewMode == 2) color = albedo * diffuseLight;           // diffuse only
    else if (viewMode == 3) color = specularLight;                   // specular only
    else if (viewMode == 4) color = N * 0.5 + 0.5;                   // normals as colours
    else {
        color = albedo * (ambient + diffuseLight) + specularLight;
        // Environment mapping: reflect the view ray off the surface and look
        // up the sky cubemap. Fresnel makes edges more mirror-like.
        if (reflectionsOn == 1 && reflectivity > 0.0) {
            vec3 R = reflect(-V, N);
            vec3 env = texture(skybox, R).rgb * 3.0;
            float fresnel = 0.6 + 0.4 * pow(1.0 - max(dot(N, V), 0.0), 3.0);
            color = mix(color, env, reflectivity * fresnel);
        }
        color += albedo * emissive;                                  // self-lit glow

        // Fog: far pixels fade into the dark warm air of the library.
        float fog = exp(-0.028 * length(cameraPos - vWorldPos));
        color = mix(vec3(0.012, 0.009, 0.008), color, fog);
    }

    FragColor = vec4(color, 1.0);   // linear HDR colour, may be > 1.0
}
