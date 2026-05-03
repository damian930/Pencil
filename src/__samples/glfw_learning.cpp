#include "core/core_include.h"
#include "core/core_include.cpp"

#include "__third_party/glfw/include/glfw3.h"
#if DEBUG_MODE 
#pragma comment (lib, "__third_party/glfw/debug_bins/glfw3.lib")
#elif RELEASE_MODE
#pragma comment (lib, "__third_party/glfw/bins/glfw3.lib")
#endif

#pragma comment (lib, "user32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "shell32.lib")

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error [%d]: %s\n", error, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  (void)scancode;
  (void)mods;

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  (void)window;
  // glViewport(0, 0, width, height);
}

int main(void)
{
  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
  {
    fprintf(stderr, "Failed to initialise GLFW\n");
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  // glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Sample Window", NULL, NULL);
  if (!window)
  {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // glClearColor(0.1f, 0.2f, 0.25f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
