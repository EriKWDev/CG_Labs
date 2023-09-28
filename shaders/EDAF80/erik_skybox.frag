#version 410

uniform vec3 light_position;
uniform vec3 camera_position;

uniform samplerCube skybox;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main() {
	vec4 skybox_sample = texture(skybox, fs_in.normal);

	frag_color = vec4(skybox_sample.rgb, 1.0);
}
