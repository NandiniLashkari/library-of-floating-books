#version 410 core
// post.vert - one big triangle that covers the whole screen.
// No vertex buffer: the 3 corners are made from gl_VertexID (0, 1, 2).
out vec2 vUV;

void main() {
    vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);   // (0,0) (2,0) (0,2)
    vUV = corner;
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
