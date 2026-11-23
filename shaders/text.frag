#version 410 core
// text.frag - draws either a letter from the font atlas or a plain panel.
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D fontAtlas;
uniform vec4 color;
uniform int solid;   // 1 = plain rectangle (panel background)

void main() {
    if (solid == 1) { FragColor = color; return; }
    float a = texture(fontAtlas, vUV).a;   // letter shape lives in alpha
    FragColor = vec4(color.rgb, color.a * a);
}
