#version 410 core
// debugquad.frag - shows the shadow map (a depth texture) as greyscale.
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D depthTexture;
void main() {
    float d = texture(depthTexture, vUV).r;
    FragColor = vec4(vec3(pow(d, 4.0)), 1.0);   // stretch contrast for viewing
}
