#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>

int main() {
	if( glfwVulkanSupported() )	{
		printf( "vulkan is available, at least for compute\n" );
	} else {
		printf( "vulkan not available\n" );
	}

	// TODO extensions tedium here

	{ // create window and surface
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		GLFWwindow* window = glfwCreateWindow( 640, 480, "vulkan hw", NULL, NULL );
		VkSurfaceKHR surface;
		VkResult err = glfwCreateWindowSurface( instance, window, NULL, &surface );
		if ( err ) {
		   printf( "ERROR: surface creation failed\n" );
		}
	}

	// TODO destroy window etc.

	return 0;
}