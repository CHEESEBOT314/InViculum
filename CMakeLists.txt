cmake_minimum_required(VERSION 3.15)
project(InViculum)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

### GLFW SETUP ###
###============================================###
set(GLFW_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL          OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/glfw)
###============================================###

find_package(Vulkan REQUIRED)
find_package(PNG REQUIRED)

set(APP_NAME "InViculum")

if (DEBUG)
    add_definitions(-DDEBUG_MODE)
endif()

set(SOURCES src/main/start.cxx

        src/main/glfw_wrapper.cxx
        src/main/game.cxx
        src/main/vulkan_wrapper.cxx

        src/main/modules/directional_light.cxx
        src/main/modules/single_point_light.cxx
        src/main/modules/multi_point_light.cxx

        src/main/render/render_manager.cxx

        src/main/resource/resource_manager.cxx

        src/main/vml/mat2.cxx
        src/main/vml/mat3.cxx
        src/main/vml/mat4.cxx
        src/main/vml/quaternion.cxx
        src/main/vml/transform.cxx
        src/main/vml/vec2.cxx
        src/main/vml/vec3.cxx
        src/main/vml/vec4.cxx)

if (WIN32)
    set(PLATFORM_SOURCES src/main/platform/windows.cxx)
    set(RESOURCE_DIR ${PROJECT_SOURCE_DIR}/bin/resources)

    add_executable(${APP_NAME} ${SOURCES} ${PLATFORM_SOURCES})
endif()

add_custom_command(TARGET ${APP_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/resources ${RESOURCE_DIR})

target_link_libraries(${APP_NAME} glfw Vulkan::Vulkan ${PNG_LIBRARIES})
target_include_directories(${APP_NAME} PRIVATE src/include glfw/include Vulkan::Vulkan ${PNG_INCLUDE_DIRS})