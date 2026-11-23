#version 410 core
// bright.frag - BLOOM step 1: keep only the parts brighter than 1.0
// (candle flames, glowing dust, moonlit highlights).
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D hdrScene;
uniform float threshold;

void main() {
    vec3 c = texture(hdrScene, vUV).rgb;
    float brightness = dot(c, vec3(0.2126, 0.7152, 0.0722));   // luminance
    // Soft knee: fade in instead of a hard cut-off.
    float keep = smoothstep(threshold, threshold * 1.6, brightness);
    FragColor = vec4(c * keep, 1.0);
}
