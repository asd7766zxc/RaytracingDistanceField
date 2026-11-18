
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Mesh/Mesh.hpp"
#include "ShaderPipelines/ShaderProgram.hpp"
#include "Player.hpp"
#include "Mesh/MeshBuilder.hpp"
#include "Visualization/DefaultRenderer.hpp"
#include "Voxelization/Voxelizer.hpp"
#include "Visualization/Visualization.hpp"

#include <chrono>
#include "GameObject.hpp"
#include "DistanceField/DistanceFieldGenerator.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Mesh/ModelLoader.hpp"
#include "Raytracing/RayTracer.hpp"
#include "DistanceField/DFMesh.hpp"
#include "Raytracing/PhotonTracer.hpp"
#include "Raytracing/PolygonSplatting.hpp"

#include <filesystem>

shared_ptr<Player> player;
bool use_splat = false;
bool use_movingobject = false;
bool start_render = false;
int window_width = 800, window_height = 800;
int LIGHTGRID = 1000;
int LIGHTRAY_COUNT = 16000000;
int STOPA,STOPB,STOPC,STOPD;
float intensity = 60000;
bool point_mode = false;
bool nowindow = false;
vec3 spread = vec3(0.5, 0.02, 0.0);
shared_ptr<RayTracer> raytracer;
quat initial_orientation;
vec3 lightPos;
vec3 OBJPos;
vec3 refracSplat;
vec3 refracPoint;

std::string objfilename;

mat4 perframe_trans;
mat4 perframe_light;

int objResolution;
float objScale;

bool usePrism = false;
int objMaterial = 0;

void loadSceneSettings(std::string path){
	std::ifstream in(path);
	if (!in) return;
	std::string tmp;
	tmp.resize(100);
	std::string line;
	int x,y,z,w;
	float xf,yf,zf,wf;
	while (std::getline(in, line)) {
		if(sscanf_s(line.c_str(), "width=%d,height=%d", &x,&y) == 2) window_width = x, window_height = y;
		else if (sscanf_s(line.c_str(), "OBJ=%100s", &tmp[0]) == 1) objfilename = tmp;
		else if (sscanf_s(line.c_str(), "material=%d", &x) == 1) objMaterial = x;
		else if (sscanf_s(line.c_str(), "usePrism=%d", &x) == 1) usePrism = x;
		else if (sscanf_s(line.c_str(), "dfres=%d", &x) == 1) objResolution = x;
		else if (sscanf_s(line.c_str(), "objscale=%f", &xf) == 1) objScale = xf;
		else if (sscanf_s(line.c_str(), "DEFAULT_RAY_MODE=%d", &x) == 1) use_splat = x;
		else if (sscanf_s(line.c_str(), "LIGHTRAY_COUNT=%d", &x) == 1) LIGHTRAY_COUNT = x;
		else if (sscanf_s(line.c_str(), "LIGHTGRID=%d", &x) == 1) LIGHTGRID = x;
		else if (sscanf_s(line.c_str(), "MOVING_OBJECT_RENDER=%d", &x) == 1) use_movingobject = x;
		else if (sscanf_s(line.c_str(), "STOPA=%d,STOPB=%d,STOPC=%d,STOPD=%d", &x, &y, &z, &w) == 4) STOPA = x, STOPB = y, STOPC = z, STOPD = w;
		else if (sscanf_s(line.c_str(), "INTENSITY=%f", &xf) == 1) intensity = xf;
		else if (sscanf_s(line.c_str(), "pos=%f,%f,%f", &xf,&yf,&zf) == 3) OBJPos = vec3(xf,yf,zf);
		else if (sscanf_s(line.c_str(), "lightpos=%f,%f,%f", &xf,&yf,&zf) == 3) lightPos = vec3(xf,yf,zf);
		else if (sscanf_s(line.c_str(), "ori=%f,%f,%f,%f", &xf,&yf,&zf,&wf) == 4) initial_orientation = quat(xf,yf,zf,wf);
		else if (sscanf_s(line.c_str(), "point_mode=%d",&x) == 1) point_mode = x;
		else if (sscanf_s(line.c_str(), "spread=%f,%f", &xf,&yf) == 2) spread.x = xf, spread.y = yf;
		else if (sscanf_s(line.c_str(), "nowindow=%d", &x) == 1) nowindow = x;
		else if (sscanf_s(line.c_str(), "IORPOINT=%f,%f", &xf,&yf) == 1) refracPoint = vec3(xf,yf,0.0);
		else if (sscanf_s(line.c_str(), "IORPOLY=%f,%f,%f", &xf,&yf,&zf) == 3) refracSplat = vec3(xf,yf,zf);;

	}
}
void window_resize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	player->camera->windowResize(width, height);

	window_width = width;
	window_height = height;

	raytracer->update_window(width, height);
}

void saveTexPPM(const std::string& path, GLuint tex) {
    int w, h;
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    std::vector<char> px(w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << ' ' << h << "\n255\n";
    f.write(px.data(), px.size());
}
BYTE tmp[128 * 128 * 128];
signed main() {
	loadSceneSettings(ASSET_DIR "scene.txt");
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
		if (key == GLFW_KEY_V && action == GLFW_PRESS) {
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
			Shader("visualization/df_visualize.vert", GL_VERTEX_SHADER),
			Shader("visualization/df_visualize.frag", GL_FRAGMENT_SHADER)
		});
	Voxelizer voxelizer;

	PhotonTracer photontracer(2048, 2048);
	PolygonSplatting polysplat(2048, 2048);
	const int splat_grid_size = LIGHTGRID; // vertices per side -> 511^2 cells

	DefaultRenderer renderer;
	raytracer = make_shared<RayTracer>();
	
	//player->rx = -0.738000691;
	//player->yx = -0.0299994592;
	//player->position = vec3(-0.512054026, 3.84784102, 5.60283041);
	player->position = vec3(0, 0, 5);
	player->loadCamera(ASSET_DIR "camera.txt"); 
	player->updateView();
	//aabb culling_box = aabb(vec3(2.8, 2, -.5), vec3(3.5, 2.6, .5));
	aabb culling_box = aabb(vec3(-1),vec3(1));
	float delta_stamp = 0.0f;
	
	shared_ptr<ModelLoader> teapot_nolid_raw = make_shared<ModelLoader>(ASSET_DIR "utah_teapot_nolid.obj");
	shared_ptr<ModelLoader> rabbit_raw = make_shared<ModelLoader>(ASSET_DIR "bunny.obj");
	shared_ptr<ModelLoader> wineglass_raw = make_shared<ModelLoader>(ASSET_DIR "another_wine.obj");
	shared_ptr<ModelLoader> text_raw = make_shared<ModelLoader>(ASSET_DIR "dispersion_text.obj");

	shared_ptr<ModelLoader> readed_raw = make_shared<ModelLoader>(ASSET_DIR + objfilename);

	
	shared_ptr<Mesh> teapot_nolid = make_shared<Mesh>(teapot_nolid_raw->vertices, teapot_nolid_raw->vertex_size * 8 * 4, teapot_nolid_raw->vertex_size);
	shared_ptr<Mesh> rabbit = make_shared<Mesh>(rabbit_raw->vertices, rabbit_raw->vertex_size * 8 * 4, rabbit_raw->vertex_size);
	shared_ptr<Mesh> wineglass = make_shared<Mesh>(wineglass_raw->vertices, wineglass_raw->vertex_size * 8 * 4, wineglass_raw->vertex_size);
	shared_ptr<Mesh> textmodel = make_shared<Mesh>(text_raw->vertices, text_raw->vertex_size * 8 * 4, text_raw->vertex_size);


	shared_ptr<Mesh> readed_model = make_shared<Mesh>(readed_raw->vertices, readed_raw->vertex_size * 8 * 4, readed_raw->vertex_size);


	DistanceFieldGenerator df_generator;
	auto dfmesh = DFMesh(df_generator, teapot_nolid, mat4::identity(), 1, 256);
	auto dfmesh_obj = DFMesh(df_generator, readed_model, mat4::identity(), objScale, objResolution);
	// auto dfmesh_rabbit = DFMesh(df_generator, rabbit, mat4::identity(), 0.25, 128);
	// auto dfmesh_text = DFMesh(df_generator, textmodel, mat4::identity(), 1.5, 256);
	// auto dfmesh_wineglass = DFMesh(df_generator, wineglass, mat4::identity(), 4, 128 * 3);
	auto box = DFMesh(df_generator, MeshBuilder::Cube(), mat4::identity(), 0.5, 128);
	vector<GameObject> objs;
	GameObject obj,obj1,obj3;
	obj.df_mesh = make_shared<DFMesh>(dfmesh_obj);
	obj.position = vec3(1, 0.8, 1.6); // prism
	obj.position = vec3(1, 0.5, 1.6); // prism 2
	obj.position = OBJPos;
	// obj.position = vec3(0, 0.5, 0); 
	// obj.position = vec3(0, 1.0, 1.6); // teapot
	// obj.position = vec3(0, 1.0, 1.6); // glass wine
	// obj.orientation = quat(0.70710678f, vec3(0, 0, -0.70710678f));
	// obj.orientation = quat(0.70710678f, vec3(0, -0.70710678f, 0));
	obj.orientation = initial_orientation;
	obj.type.x = objMaterial;
	
	if(usePrism){
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

	// obj.orientation = obj.orientation.rotate(vec3(0, pi / 2, 0));

	vec3 initial_light_pos = vec3(-2, 1.1, 1.6); // prism
	initial_light_pos = vec3(-2, 1.2, 1.6); // prism 2
	// initial_light_pos = vec3(-4, 1.8, 3.6); // prism 2
	// initial_light_pos = vec3(1.5,2,0);
	const int lightray_count = LIGHTRAY_COUNT;


	long long folder_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	std::string outputdir;
	if(nowindow){
		outputdir = OUTPUT_DIR + std::to_string(folder_timestamp) + "/";
		std::filesystem::create_directories(outputdir);
		std::error_code ec;
		std::filesystem::copy(ASSET_DIR "camera.txt",outputdir + "camera.txt", std::filesystem::copy_options::overwrite_existing,ec);
		std::filesystem::copy(ASSET_DIR "scene.txt",outputdir + "scene.txt", std::filesystem::copy_options::overwrite_existing,ec);
	}

	while(nowindow){
		++frame_index;
		glViewport(0, 0, window_width, window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		float delta = glfwGetTime() - delta_stamp;
		delta_stamp = glfwGetTime();

		vec3 light_pos = initial_light_pos; // + vec3(std::cos(glfwGetTime()),0,std::sin(glfwGetTime()));

		// player->updateCameraPos(window, delta);
		if(use_movingobject){
			// obj.orientation = obj.orientation.rotate(vec3(0, 0, 0.01));
			obj.position = (mat4::Ry(0.01) * vec4((obj.position - initial_light_pos),1.0f)).toVec3() + initial_light_pos;
		}
		df_datas[0] = obj.getDFOjbect(); 

		auto start = std::chrono::steady_clock::now();
		if (use_splat) {
			polysplat.LoadDistanceFieldDatas(df_datas);
			polysplat.render(refracSplat,splat_grid_size, vec3(intensity), light_pos, vec3(-20), vec3(20),point_mode,spread);
		} else {
			photontracer.LoadDistanceFieldDatas(df_datas);
			photontracer.render(refracPoint,lightray_count, vec3(intensity), light_pos, vec3(-20), vec3(20),point_mode,spread);
		}

		raytracer->LoadDistanceFieldDatas(df_datas);
		raytracer->use_splat = use_splat;
		raytracer->render(player->camera,vec3(intensity), light_pos, vec3(-20), vec3(20));
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		total_time += duration.count();
		std::string infos = (std::string("") + std::to_string(total_time / 1000.0f) + " ms" + " - Frame: " + std::to_string(frame_index) + " - Avg Frame time: " + std::to_string(((float)total_time) / frame_index / 1000.0f) + " ms" + " - Window Size: " + std::to_string(window_width) + "x" + std::to_string(window_height) + " " +\
         (use_splat ? ("Vertex Grid : " + std::to_string(splat_grid_size) + "^2") : (std::to_string(lightray_count) + " Light Rays"))).c_str();
		 
		if(frame_index >= STOPA){
			saveTexPPM((outputdir + "img.ppm").c_str(),raytracer->texture);
    		std::ofstream(outputdir + "info.txt", std::ios::binary) << infos;
			break;
		}
	}

	if(nowindow){
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwTerminate();
		return 0;
	}


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

		vec3 light_pos = initial_light_pos; // + vec3(std::cos(glfwGetTime()),0,std::sin(glfwGetTime()));

		player->updateCameraPos(window, delta);
		if(use_movingobject){
			// obj.orientation = obj.orientation.rotate(vec3(0, 0, 0.01));
			obj.position = (mat4::Ry(0.01) * vec4((obj.position - initial_light_pos),1.0f)).toVec3() + initial_light_pos;
		}
		df_datas[0] = obj.getDFOjbect(); 

		auto start = std::chrono::steady_clock::now();
		if (use_splat) {
			polysplat.LoadDistanceFieldDatas(df_datas);
			polysplat.render( refracSplat,splat_grid_size, vec3(intensity), light_pos, vec3(-20), vec3(20),point_mode,spread);
		} else {
			photontracer.LoadDistanceFieldDatas(df_datas);
			photontracer.render(refracPoint,lightray_count, vec3(intensity), light_pos, vec3(-20), vec3(20),point_mode,spread);
		}

		raytracer->LoadDistanceFieldDatas(df_datas);
		raytracer->use_splat = use_splat;
		raytracer->render(player->camera,vec3(intensity), light_pos, vec3(-20), vec3(20));
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		total_time += duration.count();
		std::string infos = (std::string("") + std::to_string(total_time / 1000.0f) + " ms" + " - Frame: " + std::to_string(frame_index) + " - Avg Frame time: " + std::to_string(((float)total_time) / frame_index / 1000.0f) + " ms" + " - Window Size: " + std::to_string(window_width) + "x" + std::to_string(window_height) + " " +\
         (use_splat ? ("Vertex Grid : " + std::to_string(splat_grid_size) + "^2") : (std::to_string(lightray_count) + " Light Rays"))).c_str();
		//std::cout << duration.count() << " s" << '\n';
		glfwSetWindowTitle(window, infos.c_str());

		glfwSwapBuffers(window);
		glfwPollEvents();

		if(frame_index == STOPA || frame_index == STOPB || frame_index == STOPC || frame_index == STOPD) start_render = false;
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}
