#version 410 core
// blur.frag - BLOOM step 2: separable Gaussian blur.
// Run once horizontally, then vertically, several times (ping-pong).
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D image;
uniform int horizontal;   // 1 = blur along x, 0 = along y

// 9-tap Gaussian weights (centre + 4 each side)
const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texel = 1.0 / vec2(textureSize(image, 0));
    vec2 dir = (horizontal == 1) ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);
    vec3 result = texture(image, vUV).rgb * weight[0];
    for (int i = 1; i < 5; ++i) {
        result += texture(image, vUV + dir * float(i)).rgb * weight[i];
        result += texture(image, vUV - dir * float(i)).rgb * weight[i];
    }
    FragColor = vec4(result, 1.0);
}
