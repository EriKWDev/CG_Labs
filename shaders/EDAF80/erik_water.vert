#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tex_coord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float t;

const float A1 = 1.0; //Amplitude
const float A2 = 0.5;
const vec2 D1 = vec2(-1, 0); //Direction
const vec2 D2 = vec2(-0.7, 0.7);
const float f1 = 0.2; // Frequency
const float f2 = 0.4;
const float p1 = 0.5; // Phase
const float p2 = 1.3;
const float k1 = 2.0; //Sharpness
const float k2 = 2.0;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 tangents_TBN;
	vec3 binormals_TBN;
	vec3 normals_TBN;

	vec2 coord0;
	vec2 coord1;
	vec2 coord2;
} vs_out;

void wave(vec3 vert, vec2 D, float A, float f, float p, float k,
          out vec3 displacement, out mat3 tbn){
	float alpha = sin((dot(D, vert.xz) * f) + (t * p)) * 0.5 + 0.5;

	/*
		NOTE: Calculate derivatives
	*/
	float cos_val = cos(dot(D, vert.xz) * f + t * p);
	float dGdx = (0.5 * k * f * A) * pow(alpha, k - 1) * (cos_val * D.x);
	float dGdz = (0.5 * k * f * A) * pow(alpha, k - 1) * (cos_val * D.y);

	vec3 tangent   = vec3(1, dGdx, 0 );
	vec3 bitangent = vec3(0, dGdz, 1);
	vec3 normal    = vec3(-dGdx, 1, -dGdz);
	tbn = mat3(tangent, bitangent, normal);

	/*
		NOTE: Calculate displacement
	*/
	float G = A * pow(alpha, k);
	displacement = vec3(0, G ,0);
}

void main()
{
	vec3 vertex_world = (vertex_model_to_world * vec4(vertex, 1.0)).xyz;

	/*
		NOTE: Wave 1
	*/
	vec3 disp1;
	mat3 tbn1;
	wave(vertex_world, D1, A1, f1, p1, k1, disp1, tbn1);

	/*
		NOTE: Wave 2
	*/
	vec3 disp2;
	mat3 tbn2;
	wave(vertex_world, D2, A2, f2, p2, k2, disp2, tbn2);

	vec3 displaced_vertex = vertex_world + disp1 + disp2;
	vs_out.vertex = displaced_vertex;
	gl_Position = vertex_world_to_clip * vec4(vs_out.vertex, 1.0);

	mat3 tbn = tbn1 + tbn2;
	vs_out.tangents_TBN  = tbn[0];
	vs_out.binormals_TBN = tbn[1];
	vs_out.normals_TBN   = tbn[2];

	vs_out.normal = (normal_model_to_world * vec4(tbn[2], 1.0)).xyz;

	vec2 tex_scale    = vec2(8, 4);
	float normal_time = mod(t, 100.0);
	vec2 normal_speed = vec2(-0.05, 0.01);

	vs_out.coord0 = tex_coord.xy * tex_scale     + normal_time * normal_speed;
	vs_out.coord1 = tex_coord.xy * tex_scale * 2 + normal_time * normal_speed * 4;
	vs_out.coord2 = tex_coord.xy * tex_scale * 4 + normal_time * normal_speed * 8;
}