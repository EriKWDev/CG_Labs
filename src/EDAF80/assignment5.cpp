#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 4", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}


typedef struct sphere_t {
	float radius;
	glm::vec3 point;
} sphere_t;


bool sphere_v_sphere_check(sphere_t a, sphere_t b) {
	auto distance_squared =  glm::length2(a.point - b.point);

	auto r_tot = a.radius + b.radius;
	auto r_squared = r_tot * r_tot;

	return distance_squared < r_squared;
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(-40.0f, 14.0f, 6.0f));
	mCamera.mWorld.LookAt(glm::vec3(0.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h
	auto camera_position = mCamera.mWorld.GetTranslation();

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	//
	// Todo: Insert the creation of other shader programs.
	//       (Check how it was done in assignment 3.)
	//
	GLuint erik_water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Erik Water",
	                                         { { ShaderType::vertex, "EDAF80/erik_water.vert" },
	                                           { ShaderType::fragment, "EDAF80/erik_water.frag" } },
	                                         erik_water_shader);
	if (erik_water_shader == 0u) {
		LogError("Failed to load erik_water shader");
		return;
	}

	GLuint erik_phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Erik Phong",
	                                         { { ShaderType::vertex, "EDAF80/erik_phong.vert" },
	                                           { ShaderType::fragment, "EDAF80/erik_phong.frag" } },
	                                         erik_phong_shader);
	if (erik_phong_shader == 0u)
		LogError("Failed to load erik_phong shader");
	
	GLuint erik_skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Erik Skybox",
	                                         { { ShaderType::vertex, "EDAF80/erik_skybox.vert" },
	                                           { ShaderType::fragment, "EDAF80/erik_skybox.frag" } },
	                                          erik_skybox_shader);
	if (erik_skybox_shader == 0u)
		LogError("Failed to load erik_skybox shader");

	float elapsed_time_s = 0.0f;
	// GLuint cubemap = bonobo::loadTextureCubeMap(
	// 	config::resources_path("cubemaps/light_blue_space/right.png"),
	// 	config::resources_path("cubemaps/light_blue_space/left.png"),
	// 	config::resources_path("cubemaps/light_blue_space/top.png"),
	// 	config::resources_path("cubemaps/light_blue_space/bot.png"),
	// 	config::resources_path("cubemaps/light_blue_space/front.png"),
	// 	config::resources_path("cubemaps/light_blue_space/back.png"));
	GLuint cubemap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/red_space/bkg2_right1.png"),
		config::resources_path("cubemaps/red_space/bkg2_left2.png"),
		config::resources_path("cubemaps/red_space/bkg2_top3.png"),
		config::resources_path("cubemaps/red_space/bkg2_bottom4.png"),
		config::resources_path("cubemaps/red_space/bkg2_front5.png"),
		config::resources_path("cubemaps/red_space/bkg2_back6.png"));


	GLuint diffuse_map = bonobo::loadTexture2D(config::resources_path("textures/cobblestone_floor_08_diff_2k.jpg"));
	GLuint rough_map = bonobo::loadTexture2D(config::resources_path("textures/cobblestone_floor_08_rough_2k.jpg"));
	
	GLuint normal_map = bonobo::loadTexture2D(config::resources_path("textures/waves.png"));
	// GLuint normal_map = bonobo::loadTexture2D(config::resources_path("textures/lava_normal.png"));
	// GLuint normal_map = bonobo::loadTexture2D(config::resources_path("textures/lava_normal2.png"));


	//
	// Todo: Load your geometry
	//

	// auto const shape = parametric_shapes::createQuadTess(100.0, 100.0, 500, 500);
	auto const shape = parametric_shapes::createCircleRing(130.0, 200.0, 100, 10);
	if (shape.vao == 0u)
		return;

	auto const space_ship_mesh = parametric_shapes::createSphere(10.0, 10, 10);
	if (space_ship_mesh.vao == 0u)
		return;

	auto skybox_shape = parametric_shapes::createSphere(420.0f, 100u, 100u);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_skybox_uniforms = [&light_position, &camera_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
	};
	Node skybox;
	skybox.set_geometry(skybox_shape);
	skybox.set_program(&erik_skybox_shader, set_skybox_uniforms);
	skybox.add_texture("skybox", cubemap, GL_TEXTURE_CUBE_MAP);

	auto ambient = glm::vec3(30.0f, 0.0f, 54.0f) / 255.0f;
	auto diffuse = glm::vec3(194.0f, 100.0f, 43.0f) / 255.0f;

	auto use_normal_mapping = true;
	auto const set_uniforms = [&use_normal_mapping, &light_position, &elapsed_time_s, &camera_position, &ambient, &diffuse](GLuint program) {
		glUniform1f(glGetUniformLocation(program, "t"), elapsed_time_s);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
	};

	auto const phong_set_uniforms = [&use_normal_mapping, &light_position, &camera_position](GLuint program){
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
	};
	
	auto water_node = Node();
	water_node.set_geometry(shape);
	water_node.set_program(&erik_water_shader, set_uniforms);
	water_node.add_texture("normal_map", normal_map, GL_TEXTURE_2D);
	water_node.add_texture("sky_box", cubemap, GL_TEXTURE_CUBE_MAP);
	// the_node.get_transform().RotateX(3.141592f);


	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	demo_material.diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	demo_material.shininess = 10.0f;
	
	auto space_ship_node = Node();
	space_ship_node.set_geometry(space_ship_mesh);
	space_ship_node.set_material_constants(demo_material);
	space_ship_node.set_program(&erik_phong_shader, phong_set_uniforms);
	space_ship_node.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);
	space_ship_node.add_texture("normal_map", normal_map, GL_TEXTURE_2D);
	space_ship_node.add_texture("diffuse_map", diffuse_map, GL_TEXTURE_2D);
	space_ship_node.add_texture("rough_map", rough_map, GL_TEXTURE_2D);

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::high_resolution_clock::now();

	bool pause_animation = false;
	bool use_orbit_camera = false;
	auto cull_mode = bonobo::cull_mode_t::disabled;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	glm::vec3 origin = glm::vec3(0.0f, 0.0f, 0.0f);
	float radius = 80.0f;
	float speed = 0.7f;
	float t = 0.0;

	changeCullMode(cull_mode);

	glm::vec2 ship_velocity = glm::vec2(0.0, 0.0);
	glm::vec2 ship_screen_pos = glm::vec2(0.0, 0.0);
	glm::vec3 new_ship_look_pos = glm::vec3(0.0, 0.0, 0.0);

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		if (!pause_animation) {
			elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
		}

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);

		auto dt = std::chrono::duration<float>(deltaTimeUs).count();

		if (!inputHandler.IsKeyboardCapturedByUI()) {
			glm::vec3 movement = glm::vec3(0.0, 0.0, 0.0);
			if ((inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED)) movement.x += 1.0f;
			if ((inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED)) movement.x -= 1.0f;
			if ((inputHandler.GetKeycodeState(GLFW_KEY_Q) & PRESSED)) movement.y -= 1.0f;
			if ((inputHandler.GetKeycodeState(GLFW_KEY_E) & PRESSED)) movement.y += 1.0f;

			ship_velocity.x += movement.x;
			ship_velocity.y += movement.y;
		}

		ship_velocity *= 0.98;

		ship_screen_pos += ship_velocity * dt * 20.0f;
		ship_screen_pos.x = std::clamp(ship_screen_pos.x, 0.0f, 300.0f);
		ship_screen_pos.y = std::clamp(ship_screen_pos.y, -10.0f, 30.0f);

		t += speed * dt;
		auto dir = glm::vec3(cos(t), 0.0, sin(t));
		auto p = origin + dir * radius + glm::vec3(0.0, 20.0, 0.0);
		glm::vec3 new_ship_pos = p + dir * ship_screen_pos.x + glm::vec3(0.0f, ship_screen_pos.y, 0.0f);

		float t = dt * 5.0;
		new_ship_look_pos = new_ship_look_pos * (1.0f - t) + new_ship_pos * t;

		auto dir2 = glm::cross(glm::vec3(0.0, 1.0, 0.0), (-dir));
		mCamera.mWorld.SetTranslate(p - dir2 * 130.0f + dir * ship_screen_pos.x * 0.3f);
		mCamera.mWorld.LookAt(new_ship_look_pos);

		auto& ship_transform = space_ship_node.get_transform();
		ship_transform.SetTranslate(new_ship_pos);

		// mCamera.mWorld.SetTranslate(glm::vec3(0.0, 30.0, 0.0));

		camera_position = mCamera.mWorld.GetTranslation();

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F1) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		//
		// Todo: If you need to handle inputs, you can do it here
		//
		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);

		if (!shader_reload_failed) {
			//
			// Todo: Render all your geometry here.
			//
			// mCamera.mWorld.SetTranslate(p);
			skybox.render(mCamera.GetWorldToClipMatrix());
			water_node.render(mCamera.GetWorldToClipMatrix());

			space_ship_node.render(mCamera.GetWorldToClipMatrix());
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//


		bool opened = ImGui::Begin("Scene Control", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::Separator();
			ImGui::Checkbox("Show basis", &show_basis);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
