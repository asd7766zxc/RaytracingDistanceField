
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

#include <chrono>
#include "GameObject.hpp"
#include "DistanceFieldGenerator.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "ModelLoader.hpp"


shared_ptr<Player> player;
int window_width = 800, window_height = 800;
void renderQuad();
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

	/*glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const void* userParam) {
		fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
			type, severity, message);
	}, 0);*/

#pragma endregion

	glViewport(0, 0, window_width, window_height);
	player->camera->windowResize(window_width, window_height);
	player->updateView();

	shared_ptr<ModelLoader> teapot_nolid_raw = make_shared<ModelLoader>("utah_teapot_nolid.obj");
	shared_ptr<Mesh> teapot_nolid = make_shared<Mesh>(teapot_nolid_raw->vertices, teapot_nolid_raw->vertex_size * 8 * 4, teapot_nolid_raw->vertex_size);
	auto cube = MeshBuilder::Cube();
	
	ShaderProgram RTdisplay = ShaderProgram({
			Shader("raytrace_display.vert", GL_VERTEX_SHADER),
			Shader("raytrace_display.frag", GL_FRAGMENT_SHADER)
		});

	ShaderProgram df_visual = ShaderProgram({
			Shader("df_visualize.vert", GL_VERTEX_SHADER),
			Shader("df_visualize.frag", GL_FRAGMENT_SHADER)
		});
	GameObject obj;
	Voxelizer voxelizer;
	obj.mesh = cube;
	obj.scale = vec3(2);

	DefaultRenderer renderer;
	Visualization visualizer;
	
	player->rx = -0.738000691;
	player->yx = -0.0299994592;
	player->position = vec3(-0.512054026, 3.84784102, 5.60283041);
	//aabb culling_box = aabb(vec3(2.8, 2, -.5), vec3(3.5, 2.6, .5));
	aabb culling_box = aabb(vec3(-1),vec3(1));
	VoxelData data = voxelizer.buildVoxelData(culling_box,128);

	DistanceFieldData df_data = DistanceFieldData(cube->primitive_aabb, 10);

	DistanceFieldGenerator df_generator;

	df_generator.generateDistanceField(df_data, cube);

	float delta_stamp = 0.0f;
	
	auto start = std::chrono::steady_clock::now();
	
	voxelizer.Voxelize(data, mat4::identity(), cube);
	
	auto end = std::chrono::steady_clock::now();
	
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	std::cout << duration.count() << "microsecs" << '\n';
	while (!glfwWindowShouldClose(window)) {
		glViewport(0, 0, window_width, window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		float delta = glfwGetTime() - delta_stamp;
		delta_stamp = glfwGetTime();

		obj.orientation = obj.orientation.rotate(vec3(0, 1, 0) * 0.1f);
		obj.orientation.normalize();
		player->updateCameraPos(window, delta);
		renderer.draw(player->camera, vec4(1), obj.getModelMatrix(), obj.mesh);
		//visualizer.drawVoxel(player->camera,data);
		//visualizer.drawAABB(player->camera, culling_box);
		
		//glDispatchCompute(window_width, window_height, 1);
		//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


		//RTdisplay.use();
		//renderQuad();

		visualizer.drawDF(df_data,std::abs(std::cos(glfwGetTime())));
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

