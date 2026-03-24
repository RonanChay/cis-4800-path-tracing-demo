#include <glad/glad.h> // This must be first 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Camera.h"
#include "Lighting.h"
#include "Shader.h"
#include "Model.h"

using namespace glm;

#define PI 3.1415926543f

// Function Prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main(void) {
    srand(time(0));

	/*********************************
        Setup Window and libraries
    *********************************/
	GLFWwindow* window;
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for Mac
	window = glfwCreateWindow(640*2, 480*2, "What if there was a skytrain in Guelph for some insane reason?", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

	fprintf(stdout, "Status: Using OpenGL %s\n", glGetString(GL_VERSION));

	/********************************** 
        Create + initialize Models
    **********************************/

    GLfloat cube_data[] = { 
        // position xyzh,       normal xyzw,            color rgba
		-1.0f,-1.0f,-1.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f, 1.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f, 1.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
												
		1.0f, 1.0f,-1.0f,1.0,	0.0f,0.0f,-1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f,-1.0f,1.0,	0.0f,0.0f,-1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f,-1.0f,1.0,	0.0f,0.0f,-1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
												
		1.0f,-1.0f, 1.0f,1.0,	0.0f,-1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f,-1.0f,1.0,	0.0f,-1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f,-1.0f,-1.0f,1.0,	0.0f,-1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
												
		1.0f, 1.0f,-1.0f,1.0,	0.0f,0.0f,-1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f,-1.0f,-1.0f,1.0,	0.0f,0.0f,-1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f,-1.0f,1.0,	0.0f,0.0f,-1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
												
		-1.0f,-1.0f,-1.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f, 1.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f,-1.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
												
		1.0f,-1.0f, 1.0f,1.0,	0.0f,-1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f, 1.0f,1.0,	0.0f,-1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f,-1.0f,1.0,	0.0f,-1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,

		-1.0f, 1.0f, 1.0f,1.0,	0.0f,0.0f,1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f,-1.0f, 1.0f,1.0,	0.0f,0.0f,1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f,-1.0f, 1.0f,1.0,	0.0f,0.0f,1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
											   
		1.0f, 1.0f, 1.0f,1.0,	1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f,-1.0f,-1.0f,1.0,	1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f, 1.0f,-1.0f,1.0,	1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
											   
		1.0f,-1.0f,-1.0f,1.0,	1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f, 1.0f, 1.0f,1.0,	1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f,-1.0f, 1.0f,1.0,	1.0f,0.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
											   
		1.0f, 1.0f, 1.0f,1.0,	0.0f,1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f, 1.0f,-1.0f,1.0,	0.0f,1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f,-1.0f,1.0,	0.0f,1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
											   
		1.0f, 1.0f, 1.0f,1.0,	0.0f,1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f,-1.0f,1.0,	0.0f,1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f, 1.0f,1.0,	0.0f,1.0f,0.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
											   
		1.0f, 1.0f, 1.0f,1.0,	0.0f,0.0f,1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		-1.0f, 1.0f, 1.0f,1.0,	0.0f,0.0f,1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f,
		1.0f,-1.0f, 1.0f,1.0,	0.0f,0.0f,1.0f,0.0f,	1.0f,1.0f,1.0f,1.0f
	};
    
    /********************************** 
        Create + initialize Camera 
    **********************************/
    float CAM_SENS = 200.0;
    float MOVEMENT_SPEED = 20.0;
    Camera camera(window, CAM_SENS, MOVEMENT_SPEED);

    /*******************************************
        Create + initialize point lights
    *******************************************/
    GlobalLighting globalLighting(glm::vec3(0.7f));

	/********************************** 
        Create shader program 
    **********************************/
    Shader shader_phong("src/shaders/phong_shading.glsl");
    Shader shader_basic("src/shaders/basic.glsl");
    shader_phong.Bind();

    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /********************************** 
            Delta time 
    **********************************/
    clock_t oldTimeSinceStart = clock();
	
	/***********************************
     *      Render Loop
     **********************************/
	while (!glfwWindowShouldClose(window))
	{
        clock_t timeSinceStart = clock();
        float deltaTime = ((float)(timeSinceStart - oldTimeSinceStart)) / CLOCKS_PER_SEC;
        oldTimeSinceStart = timeSinceStart;

		/*********************************************
         *              Process Inputs
         * Poll for and process events (keyboard,
         * mouse, joystick, window resizing)
         *********************************************/
        
        glfwPollEvents();
        glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
        // Lock cursor to window
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

        // --- Camera movement ---
        camera.update(deltaTime);
        camera.sendToShader(&shader_basic);
        camera.sendToShader(&shader_phong);

		/********************************************
         *            Update game state
         * This is where you would move the triangle, 
         * change the camera position, update based 
         * on time, etc.
         ********************************************/
        double cur_time = glfwGetTime();


		/***********************************
         *      Render scene
         ***********************************/
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
