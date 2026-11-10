#pragma once
#include "Helpers.hpp"
#include "Vec.hpp"
#include "Camera.hpp"
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>

class Player {
public:
	float horizontal_sensitivity = 0.003;
	float vertical_sensitivity = 0.003;
	float player_movement_speed = 2.5f;

	float rx = 0 , yx = 0, rz = 0;
	shared_ptr<Camera> camera;
	vec3 position;
	Player() {
		camera = make_shared<Camera>();
	}
	void mouseMove(float dx, float dy) {
		rx += dy * vertical_sensitivity;
		yx += dx * horizontal_sensitivity;
		if (rx >= pi / 2) rx = pi / 2 - 0.1f;
		if (rx <= -pi / 2) rx = -pi / 2 + 0.1f;
		updateView();
	}
	void updateView() {
		camera->view = mat4::Rz(rz) * mat4::Rx(-rx) * mat4::Ry(yx) * mat4::trans(-position);
		camera->position = position;
	}
	void updateCameraPos(GLFWwindow* window, float delta) {
		float camera_speed = player_movement_speed * delta;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			position += camera_speed * -camera->view.transposed().z_axis();
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			position -= camera_speed * -camera->view.transposed().z_axis();
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			position += camera_speed * -camera->view.transposed().x_axis();
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			position -= camera_speed * -camera->view.transposed().x_axis();
		}
		updateView();
	}
	bool camera_control = false;
	
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_E && action == GLFW_PRESS) {
			camera_control = !camera_control;
			glfwSetInputMode(window, GLFW_CURSOR, camera_control ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		}
		
		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			saveCamera("camera.txt");
			glfwSetWindowShouldClose(window, GLFW_TRUE); 
		}

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			exit(0);
		}
	}

	void saveCamera(const char* path) {
		std::ostringstream ss;
		ss.precision(9);
		ss << "player->rx = " << rx << ";\n"
		   << "player->yx = " << yx << ";\n"
		   << "player->rz = " << rz << ";\n"
		   << "player->position = vec3(" << position.x << ", " << position.y << ", " << position.z << ");\n";
		std::ofstream out(path);
		out << ss.str();
		//std::cout << "camera saved to " << path << ":\n" << ss.str();
	}
	
	bool loadCamera(const char* path) {
		std::ifstream in(path);
		if (!in) return false;
		std::string line;
		float x, y, z;
		while (std::getline(in, line)) {
			if (sscanf_s(line.c_str(), "player->rx = %f", &x) == 1) rx = x;
			else if (sscanf_s(line.c_str(), "player->yx = %f", &x) == 1) yx = x;
			else if (sscanf_s(line.c_str(), "player->rz = %f", &x) == 1) rz = x;
			else if (sscanf_s(line.c_str(), "player->position = vec3(%f, %f, %f)", &x, &y, &z) == 3) position = vec3(x, y, z);
		}
		updateView();
		std::cout << "camera loaded from " << path << '\n';
		return true;
	}
	void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
		static double last_x = xpos, last_y = ypos;
		double dx = xpos - last_x, dy = ypos - last_y;
		last_x = xpos, last_y = ypos;
		if (camera_control) mouseMove(dx, -dy);
	}
};