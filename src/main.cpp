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
    int windowWidth = 640;
    int windowHeight = 480;
	window = glfwCreateWindow(windowWidth, windowHeight, "Tracing the path to some balls", NULL, NULL);
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
    ModelLighting basic_lighting(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        0.0f
    );

    /********************************** 
        Create + initialize Camera 
    **********************************/
    float CAM_SENS = 200.0;
    float MOVEMENT_SPEED = 200.0;
    Camera camera(window, CAM_SENS, MOVEMENT_SPEED);

    /*******************************************
        Initialize Path Tracing
    *******************************************/
    GLfloat fullscreen_quad[] = { 
        // position xyzh,       normal xyzw,            color rgba
		-1.0f,-1.0f,0.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	0.0f,0.0f,0.0f,1.0f,
		 1.0f,-1.0f,0.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	0.0f,0.0f,0.0f,1.0f,
		-1.0f, 1.0f,0.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	0.0f,0.0f,0.0f,1.0f,
         1.0f, 1.0f,0.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	0.0f,0.0f,0.0f,1.0f,
		-1.0f, 1.0f,0.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	0.0f,0.0f,0.0f,1.0f,
		 1.0f,-1.0f,0.0f,1.0,	-1.0f,0.0f,0.0f,0.0f,	0.0f,0.0f,0.0f,1.0f,
    };

    Model path_trace_quad(0, &basic_lighting);
    path_trace_quad.init(fullscreen_quad, sizeof(fullscreen_quad)/sizeof(float), "fullscreen_quad");

    GLuint fbo, texture[2];
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    for (int i = 0; i < 2; i++) {
        glGenTextures(1, &texture[i]);
        glBindTexture(GL_TEXTURE_2D, texture[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB32F,
            windowWidth, windowHeight, 0, GL_RGB,
            GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    int writeBuffer = 0;
    int frames = 0;
    int samples_per_pixel = 32;
    int max_bounces = 8;

    int rendering_mode = 2;     // 1: no blending  |  2: temporal accumulation
    int move_objects = 1;       // 1: no moving    |  2: yes moving

	/********************************** 
        Create shader program 
    **********************************/
    Shader shader_path_tracer("src/shaders/path_tracer.glsl");
    Shader shader_tone_mapper("src/shaders/tonemapper.glsl");
    shader_path_tracer.Bind();

    glfwSwapInterval(1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    /********************************** 
            Delta time 
    **********************************/
    clock_t oldTimeSinceStart = clock();
	
	/***********************************
     *      Render Loop
     **********************************/
    int key_pressed[4] = {0, 0, 0, 0};
	while (!glfwWindowShouldClose(window)) {
        clock_t timeSinceStart = clock();
        float deltaTime = ((float)(timeSinceStart - oldTimeSinceStart)) / CLOCKS_PER_SEC;
        oldTimeSinceStart = timeSinceStart;

        // Poll for window size changes
        // Rebuild FBO and reset frames if window size changed
        int prevWidth = windowWidth;
        int prevHeight = windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        if (prevWidth != windowWidth || prevHeight != windowHeight) {
            frames = 0;
            glDeleteTextures(2, texture);
            for (int i = 0; i < 2; i++) {
                glGenFramebuffers(1, &fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);

                glGenTextures(1, &texture[i]);
                glBindTexture(GL_TEXTURE_2D, texture[i]);
                glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_RGB32F,
                    windowWidth, windowHeight, 0, GL_RGB,
                    GL_FLOAT, NULL
                );
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
        }

		/*********************************************
         *              Process Inputs
         * Poll for and process events (keyboard,
         * mouse, joystick, window resizing)
         *********************************************/
        
        glfwPollEvents();
        glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

        // Change SAMPLES_PER_PIXEL
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            if (key_pressed[0] == 0) {
                key_pressed[0] = 1;
                samples_per_pixel *= 2;
                printf("Samples per pixel: %d\n", samples_per_pixel);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE) {
            key_pressed[0] = 0;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            if (key_pressed[1] == 0) {
                key_pressed[1] = 1;
                samples_per_pixel /= 2;
                printf("Samples per pixel: %d\n", samples_per_pixel);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)  {
            key_pressed[1] = 0;
        }

        // Change MAX_BOUNCES
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            if (key_pressed[2] == 0) {
                key_pressed[2] = 1;
                max_bounces += 1;
                printf("Max Bounces: %d\n", max_bounces);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE)  {
            key_pressed[2] = 0;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            if (key_pressed[3] == 0) {
                key_pressed[3] = 1;
                max_bounces -= 1;
                printf("Max Bounces: %d\n", max_bounces);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)  {
            key_pressed[3] = 0;
        }

        // Change rendering_mode
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            printf(">>> Disabled Temporal Accumulation\n");
            rendering_mode = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            printf(">>> Enabled Temporal Accumulation\n");
            rendering_mode = 2;
        }

        // Move objects
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            printf(">>> Objects get jiggy with it\n");
            move_objects = 2;
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            printf(">>> Objects stop getting jiggy with it\n");
            move_objects = 1;
        }

        // Reset frames
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            printf(">>> Frame machine broke\n");
            frames = 0;
        }
        
        // --- Camera movement ---
        // static glm::vec3 lastCameraPos = camera.getPosition();
        // camera.update(deltaTime);
        // if (camera.getPosition() != lastCameraPos) {
        //     frames = 0;
        //     lastCameraPos = camera.getPosition();
        // }
        camera.sendToShader(&shader_path_tracer);
        camera.sendToShader(&shader_tone_mapper);
        shader_path_tracer.SetUniform3f("u_cameraPosition", camera.getPosition());

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
        if (rendering_mode == 1) {
            // No temporal accumulation
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            
            shader_path_tracer.Bind();
            shader_path_tracer.SetUniform1i("u_rendering_mode", rendering_mode);
            shader_path_tracer.SetUniform1i("u_move_objects", move_objects);
            shader_path_tracer.SetUniform1f("u_time", cur_time);
            shader_path_tracer.SetUniform1i("u_max_bounces", max_bounces);
            shader_path_tracer.SetUniform1i("u_samples_per_pixel", samples_per_pixel);
            shader_path_tracer.SetUniform2f("u_resolution", glm::vec2(windowWidth, windowHeight));
            
            path_trace_quad.draw(&shader_path_tracer);
        }
        else {
            // Temporal accumulation
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, 
                GL_COLOR_ATTACHMENT0, 
                GL_TEXTURE_2D, 
                texture[writeBuffer], 
                0
            );
            glClear(GL_COLOR_BUFFER_BIT);
            
            shader_path_tracer.Bind();
            shader_path_tracer.SetUniform1i("u_rendering_mode", rendering_mode);
            shader_path_tracer.SetUniform1i("u_move_objects", move_objects);
            shader_path_tracer.SetUniform1i("u_frames", frames);
            shader_path_tracer.SetUniform1f("u_time", cur_time);
            shader_path_tracer.SetUniform1i("u_max_bounces", max_bounces);
            shader_path_tracer.SetUniform1i("u_samples_per_pixel", samples_per_pixel);
            shader_path_tracer.SetUniform2f("u_resolution", glm::vec2(windowWidth, windowHeight));
    
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture[1-writeBuffer]);
            shader_path_tracer.SetUniform1i("u_accumulatedTexture", 0);
            path_trace_quad.draw(&shader_path_tracer);
    
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, windowWidth, windowHeight);
            glClear(GL_COLOR_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture[writeBuffer]);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(
                0, 0, windowWidth, windowHeight, 
                0, 0, windowWidth, windowHeight, 
                GL_COLOR_BUFFER_BIT, GL_LINEAR
            );
            frames++;
            writeBuffer = 1-writeBuffer;
        }

		glfwSwapBuffers(window);
	}
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(2, texture);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
