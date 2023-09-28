#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texture_coord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 texture_coord;

	vec3 normal_TBN;
	vec3 tangent_TBN;
	vec3 binormal_TBN;
} vs_out;

void main()
{
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));

	vs_out.texture_coord = texture_coord;

	vs_out.normal_TBN = normal;
	vs_out.tangent_TBN = tangent;
	vs_out.binormal_TBN = binormal;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}

