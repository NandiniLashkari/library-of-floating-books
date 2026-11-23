#version 410 core
// shadow.vert - shadow pass: draw the scene from the moon's point of view.
// Only depth is kept, so we only need the position.
layout(location = 0) in vec3 aPos;

uniform mat4 lightSpace;   // moon's projection * view
uniform mat4 model;

void main() {
    gl_Position = lightSpace * model * vec4(aPos, 1.0);
}
