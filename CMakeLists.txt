cmake_minimum_required(VERSION 3.26)

set(PROJ_NAME "3D_fractals") # <- change the project name here
set(EXE_NAME ${PROJ_NAME})

project(${PROJ_NAME})

# Add source files
file(GLOB_RECURSE SOURCES 
    ${PROJ_NAME}/src/*.cpp
)

# Add executable
add_executable(${EXE_NAME} ${SOURCES})

# -- libs --

# Opengl_startup_project
set(SOURCE_OPENGL ${PROJ_NAME}/vendor/OpenGL/OpenGL_startup_project)
add_subdirectory(${PROJ_NAME}/vendor/OpenGL)
target_include_directories(${EXE_NAME} PRIVATE ${SOURCE_OPENGL})
target_include_directories(${EXE_NAME} PRIVATE ${SOURCE_OPENGL}/source)
target_link_libraries(${EXE_NAME} OpenGL_startup_project)

# glfw
target_link_libraries(${EXE_NAME} glfw)

# glad
target_include_directories(${EXE_NAME} PRIVATE ${SOURCE_OPENGL}/vendor/glad/include)

# glm
target_include_directories(${EXE_NAME} PRIVATE ${SOURCE_OPENGL}/vendor/glm)

# stb
target_include_directories(${EXE_NAME} PRIVATE ${SOURCE_OPENGL}/vendor/stb)

# imgui
target_include_directories(${EXE_NAME} PRIVATE ${SOURCE_OPENGL}/vendor/imgui)

# freetype
target_link_libraries(${EXE_NAME} freetype)

# copy assets to executable folder
add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/${PROJ_NAME}/assets ${CMAKE_BINARY_DIR}/assets)