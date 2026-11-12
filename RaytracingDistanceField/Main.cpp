
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
#include "RayTracer.hpp"
#include "DFMesh.hpp"
#include "PhotonTracer.hpp"
#include "PolygonSplatting.hpp"


shared_ptr<Player> player;
bool use_splat = true;
bool start_render = false;
int window_width = 800, window_height = 800;
shared_ptr<RayTracer> raytracer;
void window_resize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	player->camera->windowResize(width, height);

	window_width = width;
	window_height = height;

	raytracer->update_window(width, height);
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
	glfwSetWindowPos(window, 0, 30);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, window_resize);

	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		player->key_callback(window, key, scancode, action, mods);
		if (key == GLFW_KEY_C && action == GLFW_PRESS) {
			use_splat = !use_splat;
			std::cout << "caustics: " << (use_splat ? "polygon splatting" : "point splatting") << '\n';
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			start_render = !start_render;
		}
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
	

	ShaderProgram df_visual = ShaderProgram({
			Shader("df_visualize.vert", GL_VERTEX_SHADER),
			Shader("df_visualize.frag", GL_FRAGMENT_SHADER)
		});
	Voxelizer voxelizer;

	PhotonTracer photontracer(2048, 2048);
	PolygonSplatting polysplat(2048, 2048);
	const int splat_grid_size = 1024; // vertices per side -> 511^2 cells

	DefaultRenderer renderer;
	raytracer = make_shared<RayTracer>();
	
	//player->rx = -0.738000691;
	//player->yx = -0.0299994592;
	//player->position = vec3(-0.512054026, 3.84784102, 5.60283041);
	player->position = vec3(0, 0, 5);
	player->loadCamera("camera.txt"); 
	player->updateView();
	//aabb culling_box = aabb(vec3(2.8, 2, -.5), vec3(3.5, 2.6, .5));
	aabb culling_box = aabb(vec3(-1),vec3(1));
	float delta_stamp = 0.0f;
	
	shared_ptr<ModelLoader> teapot_nolid_raw = make_shared<ModelLoader>("utah_teapot_nolid.obj");
	shared_ptr<ModelLoader> rabbit_raw = make_shared<ModelLoader>("bunny.obj");
	shared_ptr<Mesh> teapot_nolid = make_shared<Mesh>(teapot_nolid_raw->vertices, teapot_nolid_raw->vertex_size * 8 * 4, teapot_nolid_raw->vertex_size);
	shared_ptr<Mesh> rabbit = make_shared<Mesh>(rabbit_raw->vertices, rabbit_raw->vertex_size * 8 * 4, rabbit_raw->vertex_size);

	DistanceFieldGenerator df_generator;
	//auto dfmesh = DFMesh(df_generator, teapot_nolid, mat4::identity(), 1, 128);
	//auto dfmesh_rabbit = DFMesh(df_generator, rabbit, mat4::identity(), 0.25, 128);
	auto box = DFMesh(df_generator, MeshBuilder::Sphere(10), mat4::identity(), 0.5, 10);
	vector<GameObject> objs;
	GameObject obj,obj1,obj3;
	obj.df_mesh = make_shared<DFMesh>(box);
	obj.position = vec3(-1, 1.5, 1.6);
	obj.type.x = 3;

	{
		auto rod = MeshBuilder::Rod(3);
		std::vector<float> v = rod->unstructured_vertices;
		for (int i = 0; i < rod->vertex_count; ++i) {
			v[i * 8 + 0] *= 2.2f;
			v[i * 8 + 1] *= 2.2f;
			v[i * 8 + 2] *= 1.2f;
		}
		for (int t = 0; t + 12 <= rod->vertex_count; t += 12)
			std::swap_ranges(v.begin() + (t + 6) * 8, v.begin() + (t + 7) * 8, v.begin() + (t + 7) * 8);
		auto prism = make_shared<DFMesh>();
		prism->updateMesh(v.data(), (int)v.size());
		const float pad = 0.3f;
		aabb b = prism->primitive_aabb;
		aabb padded(interval(b.x.min - pad, b.x.max + pad), interval(b.y.min - pad, b.y.max + pad), interval(b.z.min - pad, b.z.max + pad));
		prism->df_data = DistanceFieldData(padded, 128);
		df_generator.generateDistanceField(prism->df_data, make_shared<Mesh>(*prism));

		obj.df_mesh = prism;
	}

	//obj3.type.x = 3;
	//obj3.position = vec3(1.6, 1.5, 1.6);
	//obj3.df_mesh = make_shared<DFMesh>(dfmesh);
	
	//objs.push_back(obj3);
	//objs.push_back(obj1);
	//objs.push_back(obj3);
	objs.push_back(obj);


	
	raytracer->update_window(window_width, window_height);
	raytracer->setupScence();



	for (auto c : objs) c.prepareDFData();
		vector<DF_Object> df_datas;
	for (auto c : objs) df_datas.push_back(c.getDFOjbect());
	photontracer.LoadDistanceFieldDatas(df_datas);
	int total_time = 0;
	int frame_index = 0;
	while (!glfwWindowShouldClose(window)) {
		if (!start_render) {
			glfwPollEvents();
			continue;
		}
		++frame_index;
		glViewport(0, 0, window_width, window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		float delta = glfwGetTime() - delta_stamp;
		delta_stamp = glfwGetTime();

		player->updateCameraPos(window, delta);
		 obj.orientation = obj.orientation.rotate(vec3(0, 0.01, 0));
		df_datas[0].model = obj.getModelMatrix();

		auto start = std::chrono::steady_clock::now();
		if (use_splat) {
			polysplat.LoadDistanceFieldDatas(df_datas);
			polysplat.render(splat_grid_size, vec3(50000), vec3(0, 1, 0), vec3(-20), vec3(20));
		} else {
			photontracer.LoadDistanceFieldDatas(df_datas);
			photontracer.render(1000000, vec3(50000 * 1.2), vec3(0, 1, 0), vec3(-20), vec3(20));
		}

		raytracer->LoadDistanceFieldDatas(df_datas);
		raytracer->use_splat = use_splat;
		raytracer->render(player->camera, vec3(-20), vec3(20));
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		total_time += duration.count();
		//std::cout << duration.count() << " s" << '\n';
		glfwSetWindowTitle(window, (std::string("Raytracing Distance Field - ") + std::to_string(total_time / 1000.0f) + " ms" + " - Frame: " + std::to_string(frame_index) + " - Avg Frame time: " + std::to_string(((float)total_time) / frame_index / 1000.0f) + " ms" + " - Window Size: " + std::to_string(window_width) + "x" + std::to_string(window_height)).c_str());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}
