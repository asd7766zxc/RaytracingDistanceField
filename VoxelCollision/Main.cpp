
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "Player.hpp"
#include "MeshBuilder.hpp"
#include "DefaultRenderer.hpp"
#include "Voxelizer.hpp"
#include "Visualization.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "ModelLoader.hpp"

#include <chrono>

shared_ptr<Player> player;
int window_width = 1024, window_height = 1024;
void window_resize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	player->camera->windowResize(width, height);
	window_width = width;
	window_height = height;
}
BYTE tmp[128 * 128 * 128];
signed main() {
	player = make_shared<Player>();
#pragma region WindowInitialization
	GLFWwindow* window;

	if (!glfwInit()) return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(window_width, window_height, "Voxel", NULL, NULL);

	if (!window) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, window_resize);

	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		player->key_callback(window, key, scancode, action, mods);
	});
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
		player->cursor_pos_callback(window, xpos, ypos);
	});

	//glfwSetMouseButtonCallback(window, mouse_button_callback);
	//glfwSetScrollCallback(window, scroll_callback);
	glfwSwapInterval(1); // Make fps constants
#pragma endregion
#pragma region GladInitialization

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Fail to load glad\n";
		return 0;
	}
	std::cout << glGetString(GL_VERSION) << '\n';

#pragma endregion
#pragma region ImGuiSetup
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
#pragma endregion
#pragma region GlSetup
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#pragma endregion

	glViewport(0, 0, window_width, window_height);
	player->camera->windowResize(window_width, window_height);
	player->updateView();

	shared_ptr<ModelLoader> teapot_nolid_raw = make_shared<ModelLoader>("utah_teapot_nolid.obj");
	shared_ptr<Mesh> teapot_nolid = make_shared<Mesh>(teapot_nolid_raw->vertices, teapot_nolid_raw->vertex_size * 8 * 4, teapot_nolid_raw->vertex_size);
	auto cube = MeshBuilder::Cone(20);


	Voxelizer voxelizer;

	DefaultRenderer renderer;
	Visualization visualizer;
	
	player->rx = 0.374999821;
	player->yx = -0.0869998336;
	player->position = vec3(3.18540215, 2.11347151, 0.676904023);

	//aabb culling_box = aabb(vec3(2.8, 2, -.5), vec3(3.5, 2.6, .5));
	aabb culling_box = aabb(vec3(-1),vec3(1));
	VoxelData data = voxelizer.buildVoxelData(culling_box,128);
	float delta_stamp = 0.0f;
	auto start = std::chrono::steady_clock::now();
		voxelizer.Voxelize(data, mat4::identity(), cube);
	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	std::cout << duration.count() << "£gs" << '\n';
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		float delta = glfwGetTime() - delta_stamp;
		delta_stamp = glfwGetTime();

		player->updateCameraPos(window, delta);
		renderer.draw(player->camera, vec4(1), mat4::identity(), cube);
		visualizer.drawVoxel(player->camera,data);
		visualizer.drawAABB(player->camera, culling_box);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}