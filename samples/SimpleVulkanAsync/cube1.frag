#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D samplerTex;

layout(location = 0) in vec2 fragTex;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(samplerTex,fragTex);
}