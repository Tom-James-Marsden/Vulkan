#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cstdlib>
#include <iostream>

int main() {
	uint32_t version = 0;
	vkEnumerateInstanceVersion(&version);

	std::cout << "Vulkan version: "
		<< VK_VERSION_MAJOR(version) << "."
		<< VK_VERSION_MINOR(version) << "."
		<< VK_VERSION_PATCH(version) << std::endl;




	// init glfw
	if (glfwInit() != GLFW_TRUE) {
		std::cerr << "Failed to initialize GLFW.\n";
		return EXIT_FAILURE;
	}

	// window hints
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);


	// create window
	GLFWwindow* window = glfwCreateWindow(800, 600, "Vox", nullptr, nullptr);


	if (window == nullptr) {
		std::cerr << "Failed to create GLFW window.\n";
		glfwTerminate();
		return EXIT_FAILURE;
	}
	// glm size ( for doing fancy stuff )
	glm::vec2 viewport_size(800.0f, 600.0f);
	(void)viewport_size;

	// main loop
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		glfwPollEvents();
	}


	glm::vec3 color(1.0f, 0.0f, 0.0f);

	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}