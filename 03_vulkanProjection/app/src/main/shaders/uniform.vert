#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(binding = 0) uniform UniformBufferObject {
   mat4 mvpMtx;
} ubo;

void main() {
   gl_Position = ubo.mvpMtx * vec4(pos.xyz, 1.0);
}