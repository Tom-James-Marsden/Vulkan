#version 450

layout(location = 0) in vec3 inPos;    // from vertex buffer
layout(location = 1) in vec3 inColor;  // from vertex buffer

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} pc;

layout(location = 0) out vec3 fragColor; // pass to fragment shader

void main() {
    gl_Position = pc.proj * pc.view * pc.model * vec4(inPos, 1.0); // convert 2D -> 4D for Vulkan
    fragColor = inColor;
}