#version 450

layout(location = 0) in vec2 inPos;    // from vertex buffer
layout(location = 1) in vec3 inColor;  // from vertex buffer

layout(location = 0) out vec3 fragColor; // pass to fragment shader

void main() {
    gl_Position = vec4(inPos, 0.0, 1.0); // convert 2D -> 4D for Vulkan
    fragColor = inColor;
}