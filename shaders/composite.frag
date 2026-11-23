#version 410 core
// composite.frag - final step: HDR scene + bloom -> tone mapping -> gamma.
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D hdrScene;
uniform sampler2D bloom;
uniform int bloomOn;
uniform float bloomStrength;
uniform float exposure;
uniform int passthrough;   // 1 = debug view: show the raw values

// ACES filmic curve (Narkowicz fit): squeezes any brightness into 0..1
// with a gentle roll-off instead of hard clipping.
vec3 acesToneMap(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(hdrScene, vUV).rgb;
    if (bloomOn == 1) hdr += texture(bloom, vUV).rgb * bloomStrength;

    if (passthrough == 1) { FragColor = vec4(texture(hdrScene, vUV).rgb, 1.0); return; }
    vec3 mapped = acesToneMap(hdr * exposure);
    mapped = pow(mapped, vec3(1.0 / 2.2));      // gamma correction for the monitor
    FragColor = vec4(mapped, 1.0);
}
