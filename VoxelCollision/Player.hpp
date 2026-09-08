#pragma once
#include "Helpers.hpp"
#include "Vec.hpp"
#include "Camera.hpp"
#include <GLFW/glfw3.h>

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
		
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			exit(0);
		}
	}
	void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
		static double last_x = xpos, last_y = ypos;
		double dx = xpos - last_x, dy = ypos - last_y;
		last_x = xpos, last_y = ypos;
		if (camera_control) mouseMove(dx, -dy);
	}
};