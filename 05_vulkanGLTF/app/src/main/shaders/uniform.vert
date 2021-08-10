#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 uv;

layout(binding = 0) uniform UniformBufferObject {
   mat4 mvpMtx;
} ubo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
   gl_Position = ubo.mvpMtx * vec4(pos.xyz, 1.0);
   vec3 v = (tangent.xyz + 1.0) / 2;
   fragColor = vec3(v.xyz);
   fragTexCoord = uv;
}