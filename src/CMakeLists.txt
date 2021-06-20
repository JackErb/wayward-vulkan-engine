set(CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.9)

# TODO: Look up policies
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0076 NEW)

# Set C++ Version
set(CMAKE_CXX_STANDARD 17)

project(DinoHerder)

set(ENV{VK_ICD_FILENAMES} $ENV{VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)
set(ENV{VK_LAYER_PATH} $ENV{VULKAN_SDK}/share/explicit_layer.d)

message(STATUS $ENV{VK_ICD_FILENAMES})
message(STATUS $ENV{VK_LAYER_PATH})

set(SOURCE_FILES main.cc app.h app.cc shaders.cc resources.cc)
file(GLOB_RECURSE RES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

if (APPLE)
    # Create executable
    add_executable(DinoHerder MACOSX_BUNDLE ${SOURCE_FILES} ${RES_SOURCES})
    set_source_files_properties(${RES_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION resources/)
	
    set(SDL_LIBS "/Library/Frameworks/SDL2.framework"
                 "/Library/Frameworks/SDL2_image.framework")
    set(GLFW_LIBS "/usr/local/lib/libglfw.3.3.dylib")
    set(VULKAN_LIBS "/usr/local/lib/libvulkan.1.2.176.dylib" "/usr/local/lib/libvulkan.1.dylib")
endif()

# Header includes
target_include_directories(DinoHerder PRIVATE /usr/local/include)

# Optimization
set(OPTIMIZATION_FLAG "-O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OPTIMIZATION_FLAG}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${OPTIMIZATION_FLAG}")

# add subdirectories

# include directories
target_include_directories(DinoHerder PRIVATE ../inc)

set(GAME_LIBRARIES "")
target_link_libraries(DinoHerder PRIVATE ${GAME_LIBRARIES}
                                         ${GLFW_LIBS}
                                         ${VULKAN_LIBS}
                                         )
