#version 410

uniform vec3 light_position;
uniform vec3 camera_position;

uniform vec3 diffuse_colour;
uniform vec3 ambient_colour;
uniform vec3 specular_colour;

uniform float shininess_value;
uniform bool use_normal_mapping;

uniform sampler2D diffuse_map;
uniform sampler2D normal_map;
uniform sampler2D rough_map;

uniform mat4 normal_model_to_world;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 texture_coord;

	vec3 normal_TBN;
	vec3 tangent_TBN;
	vec3 binormal_TBN;
} fs_in;

out vec4 frag_color;

void main() {
	vec3 V = normalize(camera_position - fs_in.vertex);
	vec3 L = normalize(light_position - fs_in.vertex);

	vec3 N = normalize(fs_in.normal);
	if (use_normal_mapping) {
		vec3 tangent_norm = fs_in.tangent_TBN;
		vec3 binormal_norm = fs_in.binormal_TBN;
		vec3 normal_norm = fs_in.normal_TBN;

		mat3 TBN = mat3(tangent_norm, binormal_norm, normal_norm);

		vec3 n = texture(normal_map, fs_in.texture_coord.xy).xyz * 2.0 - 1.0;  
		n = normalize(n);
		N = (normal_model_to_world * vec4(TBN * n, 1.0)).xyz;
	}

	vec3 diffuse_sample_colour = texture(diffuse_map, fs_in.texture_coord.xy).rgb;
	float rough_sample = texture(rough_map, fs_in.texture_coord.xy).r;
	vec3 diff_color = diffuse_sample_colour * diffuse_colour;

	/*
		NOTE: Phong
	*/
	vec3 color =
		  ambient_colour
    	+ diff_color * max(dot(N, L), 0.0)
		+ (specular_colour * rough_sample) * max(pow(dot(reflect(-L, N), V), shininess_value), 0.0);

	// vec3 R = reflect(-V, N);

	frag_color = vec4(color, 1.0);
	// frag_color = vec4(diffuse_sample_colour, 1.0);
	// frag_color = vec4(1.0, 0.0, 1.0, 1.0);
}
