#version 410

uniform vec3 light_position;
uniform vec3 camera_position;

uniform vec3 diffuse_colour;
uniform vec3 ambient_colour;
uniform vec3 specular_colour;

uniform float shininess_value;

uniform samplerCube cubemap;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 L = normalize(light_position - fs_in.vertex);
	vec3 V = normalize(camera_position - fs_in.vertex);

	vec3 R = reflect(-V, fs_in.normal);
	// frag_color = vec4(1.0) * clamp(dot(fs_in.normal, L), 0.0, 1.0);
	frag_color = texture(cubemap, R);
	// frag_color = vec4(1.0, 1.0, 0.0, 1.0);
}
