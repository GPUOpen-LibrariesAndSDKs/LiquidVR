#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform mvpMat{
	mat4 model;
	mat4 view;
	mat4 proj;
} mvp;

out gl_PerVertex{
	vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 incolor;

layout(location = 0) out vec3 fragColor;

void main() {
	gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPosition,1.0);

	fragColor = vec3(incolor,0);
}

//mvp.proj * mvp.view * mvp.model