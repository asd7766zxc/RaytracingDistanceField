#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "Player.hpp"
#include "RayTracer.hpp"
#include "PhotonTracer.hpp"
#include "PolygonSplatting.hpp"
#include "DistanceFieldGenerator.hpp"
#include "DFMesh.hpp"
#include "MeshBuilder.hpp"
#include "ModelLoader.hpp"
#include "GameObject.hpp"
#include "PngWriter.hpp"

struct BatchConfig {
	std::string scene = "teapot";
	int width = 800, height = 800;
	std::string camera;
	int photons = 160000;
	int wavelengths = 3;
	std::string splat = "point";
	int frames = 30;
	int seed = 1;
	std::string output = "batch.png";
	std::string timing_csv = "batch_timing.csv";
	vec3 ior = vec3(1.5f, 1.4f, 1.3f);
};

inline std::string batchTrim(const std::string& s) {
	size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return "";
	size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}

inline bool loadBatchConfig(const char* path, BatchConfig& c) {
	std::ifstream in(path);
	if (!in) return false;
	std::map<std::string, std::string> kv;
	std::string line;
	while (std::getline(in, line)) {
		line = batchTrim(line);
		size_t eq = line.find('=');
		if (line.empty() || line[0] == '#' || eq == std::string::npos) continue;
		kv[batchTrim(line.substr(0, eq))] = batchTrim(line.substr(eq + 1));
	}
	auto get = [&](const char* k, auto& v) {
		auto it = kv.find(k);
		if (it == kv.end()) return;
		if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) v = it->second;
		else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) v = std::stoi(it->second);
		else v = std::stof(it->second);
	};
	get("scene", c.scene);
	get("width", c.width);
	get("height", c.height);
	get("camera", c.camera);
	get("photons", c.photons);
	get("wavelengths", c.wavelengths);
	get("splat", c.splat);
	get("frames", c.frames);
	get("seed", c.seed);
	get("output", c.output);
	get("timing_csv", c.timing_csv);
	get("ior_r", c.ior.x);
	get("ior_g", c.ior.y);
	get("ior_b", c.ior.z);
	return true;
}

inline int renderBatch(const BatchConfig& cfg) {
	bool quad = cfg.splat == "quad";
	if (!quad && cfg.splat != "point") { std::cerr << "splat must be point or quad\n"; return 1; }
	if (cfg.wavelengths != 1 && cfg.wavelengths != 3) { std::cerr << "wavelengths must be 1 or 3\n"; return 1; }
	if (cfg.scene != "teapot" && cfg.scene != "prism") { std::cerr << "scene must be teapot or prism\n"; return 1; }

	Player player;
	player.position = vec3(0, 0, 5);
	player.camera->windowResize(cfg.width, cfg.height);
	if (!cfg.camera.empty() && !player.loadCamera(cfg.camera.c_str())) {
		std::cerr << "cannot read camera " << cfg.camera << '\n';
		return 1;
	}
	player.updateView();

	PhotonTracer photontracer(2048, 2048);
	PolygonSplatting polysplat(2048, 2048);

	GLuint fbo = 0, color = 0;
	glGenTextures(1, &color);
	glBindTexture(GL_TEXTURE_2D, color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cfg.width, cfg.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
	bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!complete) { std::cerr << "output framebuffer incomplete\n"; return 1; }

	RayTracer raytracer;
	raytracer.update_window(cfg.width, cfg.height);

	DistanceFieldGenerator df_generator;
	GameObject obj;
	obj.position = vec3(-1, 1.5, 1.6);
	obj.type.x = 3;
	if (cfg.scene == "teapot") {
		auto raw = make_shared<ModelLoader>("utah_teapot_nolid.obj");
		auto mesh = make_shared<Mesh>(raw->vertices, raw->vertex_size * 8 * 4, raw->vertex_size);
		obj.df_mesh = make_shared<DFMesh>(df_generator, mesh, mat4::identity(), 1, 128);
	}
	else {
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
		obj.position = vec3(0.8f, 0.6f, 0.8f);
		obj.orientation = quat((float)std::cos(pi / 4), 0.0f, (float)std::sin(pi / 4), 0.0f);
	}
	obj.prepareDFData();
	std::vector<DF_Object> df_datas = { obj.getDFOjbect() };

	vec3 ior = cfg.wavelengths == 1 ? vec3(cfg.ior.y) : cfg.ior;
	photontracer.refract_indices = ior;
	polysplat.refract_indices = ior;
	raytracer.use_splat = quad;
	int grid = std::max(2, (int)std::lround(std::sqrt((double)cfg.photons)));

	GLuint queries[3];
	glGenQueries(3, queries);
	std::ofstream csv(cfg.timing_csv);
	if (!csv) { std::cerr << "cannot write " << cfg.timing_csv << '\n'; return 1; }
	csv << "frame,scene,splat,photons,wavelengths,photon_ms,splat_ms,raytrace_ms\n";

	for (int f = 0; f < cfg.frames; ++f) {
		photontracer.seed = (float)(cfg.seed * 4096 + f);
		photontracer.LoadDistanceFieldDatas(df_datas);
		polysplat.LoadDistanceFieldDatas(df_datas);
		raytracer.LoadDistanceFieldDatas(df_datas);

		glBeginQuery(GL_TIME_ELAPSED, queries[0]);
		if (!quad) photontracer.render(cfg.photons, vec3(50000), vec3(0, 1, 0), vec3(-20), vec3(20));
		glEndQuery(GL_TIME_ELAPSED);

		glBeginQuery(GL_TIME_ELAPSED, queries[1]);
		if (quad) polysplat.render(grid, vec3(50000), vec3(0, 1, 0), vec3(-20), vec3(20));
		glEndQuery(GL_TIME_ELAPSED);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, cfg.width, cfg.height);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBeginQuery(GL_TIME_ELAPSED, queries[2]);
		raytracer.render(player.camera, vec3(-20), vec3(20));
		glEndQuery(GL_TIME_ELAPSED);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		GLuint64 ns[3];
		for (int i = 0; i < 3; ++i) glGetQueryObjectui64v(queries[i], GL_QUERY_RESULT, &ns[i]);
		csv << f << ',' << cfg.scene << ',' << cfg.splat << ',' << cfg.photons << ',' << cfg.wavelengths << ','
			<< ns[0] / 1e6 << ',' << ns[1] / 1e6 << ',' << ns[2] / 1e6 << '\n';
	}

	std::vector<uint8_t> rgba((size_t)cfg.width * cfg.height * 4), rgb((size_t)cfg.width * cfg.height * 3);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, cfg.width, cfg.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	for (int y = 0; y < cfg.height; ++y)
		for (int x = 0; x < cfg.width; ++x)
			for (int c = 0; c < 3; ++c)
				rgb[((size_t)y * cfg.width + x) * 3 + c] = rgba[((size_t)(cfg.height - 1 - y) * cfg.width + x) * 4 + c];
	if (!png_writer::writeRGB(cfg.output, cfg.width, cfg.height, rgb.data())) {
		std::cerr << "cannot write " << cfg.output << '\n';
		return 1;
	}

	glDeleteQueries(3, queries);
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &color);
	std::cout << "wrote " << cfg.output << " and " << cfg.timing_csv << '\n';
	return 0;
}

inline int runBatch(const char* config_path) {
	BatchConfig cfg;
	if (!loadBatchConfig(config_path, cfg)) {
		std::cerr << "cannot read config " << config_path << '\n';
		return 1;
	}
	if (!glfwInit()) return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(cfg.width, cfg.height, "batch", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		glfwTerminate();
		return 1;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int ret = 0;
	try {
		ret = renderBatch(cfg);
	}
	catch (const std::exception& e) {
		std::cerr << "batch failed: " << e.what() << '\n';
		ret = 1;
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return ret;
}
