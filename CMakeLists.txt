cmake_minimum_required(VERSION 3.10)
project(GraphicsDemo)

set(CMAKE_CXX_STANDARD 17)

# --- CHANGED: VENDORING GLFW ---
# Instead of looking for it on the system, we build it from the folder we provided.
# This prevents the "Could not find package" error for students.
add_subdirectory(libs/glfw) 

# --- YOUR PROJECT ---
add_executable(demo src/main.cpp src/Shader.cpp src/Model.cpp src/Camera.cpp src/Lighting.cpp src/glad.c)

# --- INCLUDE DIRECTORIES ---
target_include_directories(demo PRIVATE include libs/glfw/include libs/glm)

# Tell the compiler where to find your headers (include/) AND 
# where to find GLFW's headers (libs/glfw/include)
target_include_directories(demo PRIVATE include libs/glfw/include)

# --- CHANGED: LINKING ---
# We link against the 'glfw' target that we just built in the line above.
target_link_libraries(demo PRIVATE glfw)

# Platform specifics (Windows/Mac/Linux)
if(WIN32)
    target_link_libraries(demo
 PRIVATE opengl32)
elseif(APPLE)
    target_link_libraries(demo
 PRIVATE "-framework OpenGL" "-framework IOKit" "-framework Cocoa" "-framework CoreVideo")
else()
    target_link_libraries(demo
 PRIVATE GL dl)
endif()
