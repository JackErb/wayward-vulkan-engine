set(CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.9)

# TODO: Look up policies
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0076 NEW)

# Set C++ Version
set(CMAKE_CXX_STANDARD 17)

project(WaywardVK)

set(ENV{VK_ICD_FILENAMES} $ENV{VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)
set(ENV{VK_LAYER_PATH} $ENV{VULKAN_SDK}/share/explicit_layer.d)

set(SOURCE_FILES main.cc app.h app.cc wvk_window.h wvk_window.cc wvk_device.h wvk_device.cc wvk_helper.h
                 wvk_pipeline.h wvk_pipeline.cc wvk_swapchain.h wvk_swapchain.cc resource_path.h resource_path.cc
                 wvk_image.h wvk_image.cc wvk_model.h wvk_model.cc glm.h wvk_buffer.h  wvk_skeleton.h wvk_skeleton.cc
                 wvk_sampler.h wvk_sampler.cc
                 anim/skeleton.h anim/skeleton.cc anim/accessor_parser.cc)
file(GLOB_RECURSE RES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

if (APPLE)
    # Create executable
    add_executable(WaywardVK MACOSX_BUNDLE ${SOURCE_FILES} ${RES_SOURCES})
    set_source_files_properties(${RES_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION resources/)

    set(SDL_LIBS "/Library/Frameworks/SDL2.framework"
                 "/Library/Frameworks/SDL2_image.framework")
    set(GLFW_LIBS "/usr/local/lib/libglfw.3.3.dylib")
    set(VULKAN_LIBS "/usr/local/lib/libvulkan.1.2.176.dylib" "/usr/local/lib/libvulkan.1.dylib")
endif()

set(PROJECT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../")

set(IMGUI "${PROJECT_SRC}lib/imgui/")
add_library(imgui STATIC "${IMGUI}imgui.cpp" "${IMGUI}imgui_demo.cpp" "${IMGUI}imgui_draw.cpp" "${IMGUI}imgui_tables.cpp"
                         "${IMGUI}imgui_impl_glfw.cpp" "${IMGUI}imgui_impl_vulkan.cpp")
target_link_libraries(imgui PRIVATE ${GLFW_LIBS} ${VULKAN_LIBS})
target_include_directories(imgui PRIVATE /usr/local/include ${IMGUI})

add_library(tinygltf STATIC "${PROJECT_SRC}inc/tiny_gltf.h" "${PROJECT_SRC}inc/stb_image.h" "${PROJECT_SRC}inc/stb_image_write.h"
                            "${PROJECT_SRC}lib/tiny_gltf_impl.cc")

# Header includes
target_include_directories(WaywardVK PRIVATE /usr/local/include ${PROJECT_SRC}lib/imgui)

# Optimization
set(OPTIMIZATION_FLAG "-O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OPTIMIZATION_FLAG}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${OPTIMIZATION_FLAG}")

# add subdirectories
add_subdirectory(game)

# include directories
target_include_directories(WaywardVK PRIVATE /usr/local/include ${PROJECT_SRC}lib/imgui ${PROJECT_SRC}inc)
target_include_directories(WaywardGame PRIVATE /usr/local/include ${PROJECT_SRC}inc)

set(GAME_LIBRARIES WaywardGame tinygltf)
target_link_libraries(WaywardVK PRIVATE ${GAME_LIBRARIES}
                                         ${GLFW_LIBS}
                                         ${VULKAN_LIBS}
                                         )
