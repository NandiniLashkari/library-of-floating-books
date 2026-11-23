#version 410 core
// text.vert - on-screen labels. Positions are in pixels, (0,0) = top-left.
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform vec2 screenSize;
out vec2 vUV;

void main() {
    vUV = aUV;
    vec2 ndc = vec2(aPos.x / screenSize.x * 2.0 - 1.0,
                    1.0 - aPos.y / screenSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
