#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2D texSampler;

layout (location = 0) out vec4 uFragColor;

void main() {
//    uFragColor = vec4(1.0, 0.0, 0.0, 1.0);
 //   uFragColor = vec4(fragTexCoord.xy, 1, 1.0);
    uFragColor = texture(texSampler, fragTexCoord);
}