#version 410

uniform vec3 light_position;
uniform vec3 camera_position;

uniform vec3 ambient;
uniform vec3 diffuse;

uniform sampler2D normal_map;
uniform samplerCube sky_box;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform bool use_normal_mapping;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 tangents_TBN;
	vec3 binormals_TBN;
	vec3 normals_TBN;

	vec2 coord0;
	vec2 coord1;
	vec2 coord2;
} fs_in;

out vec4 frag_color;

void main()
{
	vec4 color_deep = vec4(ambient, 1.0);
	vec4 color_shallow = vec4(diffuse, 1.0);
	vec3 V = normalize(camera_position - fs_in.vertex);

	/*
		NOTE: Calculate normals
	*/ 
	vec3 N = normalize(fs_in.normal);
	if (use_normal_mapping) {
		vec3 tangents_norm  = normalize(fs_in.tangents_TBN);
		vec3 binormals_norm = normalize(fs_in.binormals_TBN);
		vec3 normals_norm   = normalize(fs_in.normals_TBN);
		mat3 tbn_norm = mat3(tangents_norm, binormals_norm, normals_norm);

		vec4 bump = normalize((texture(normal_map, fs_in.coord0) * 2 - 1
							 + texture(normal_map, fs_in.coord1) * 2 - 1
							 + texture(normal_map, fs_in.coord2) * 2 - 1));

		N = normalize((normal_model_to_world * vec4(tbn_norm * bump.xyz, 1.0)).xyz);
	}

	float facing = 1.0 - max(dot(V, N), 0.0);
	vec3 R = reflect(-V, N);

	float R0 = 0.02037;
	float fresnel = R0 + (1.0 - R0) * pow(1.0 - dot(V, N), 5.0);

	float ETA = 1.0 / 1.33;
	vec3 refraction = refract(-V, N, ETA);

	vec4 col_reflection = texture(sky_box, R);
	vec4 col_refraction = texture(sky_box, refraction);
	vec4 col_water = mix(color_deep, color_shallow, facing);

	frag_color = col_water
	 + (col_reflection * fresnel)
	 + (col_refraction * (1.0 - fresnel));

	/*
		NOTE: Debugs
	*/
	// frag_color = col_reflection * (fresnel + 0.5);
	// frag_color = col_refraction * (1.0 - fresnel);

	// frag_color = vec4(vec3(fresnel), 1.0);
	// frag_color = vec4(vec3(dot(N, V)), 1.0);
	// frag_color = vec4(R, 1.0);

	// frag_color = vec4(V, 1.0);
	// frag_color = vec4(camera_position, 1.0);
	// frag_color = vec4(N, 1.0);
	// frag_color = vec4(vec3(fs_in.coord0, 0.0), 1.0);
}
