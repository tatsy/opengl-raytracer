# ----------------------------------------------------------------------------------------------------------------------
# Add all the subdirectories of "ext" as include directory
# ----------------------------------------------------------------------------------------------------------------------
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}/ext")
set(EXT_INCLUDE_DIRS "")
list(APPEND EXT_INCLUDE_DIRS
    "${EXT_DIR}"
    "${EXT_DIR}/imgui"
    "${EXT_DIR}/imgui/examples"
    "${EXT_DIR}/stb"
    "${EXT_DIR}/tinyobjloader"
    "${EXT_DIR}/tinyply/source"
)

foreach(D IN LISTS EXT_INCLUDE_DIRS)
    include_directories(${D})
    set(GLRT_INCLUDE_DIR "${GLPG_INCLUDE_DIR};${D}" CACHE INTERNAL "")
endforeach()

# ----------------------------------------------------------------------------------------------------------------------
# External components
# ----------------------------------------------------------------------------------------------------------------------
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
if (MSVC)
    add_definitions(-DIMGUI_API=__declspec\(dllexport\))
endif()

# ----------------------------------------------------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------------------------------------------------
set(BUILD_TARGET ${GLRT_MAIN_BINARY})

file(GLOB SOURCE_FILES "*.cpp" "*.hpp" "*.c" "*.h")
file(GLOB EXTERNAL_FILES
     "ext/**/*.hpp" "ext/**/*.h" "ext/**/*.cpp" "ext/**/*.c"
     "ext/**/examples/imgui_impl_opengl3.cpp" "ext/**/examples/imgui_opengl3.h"
     "ext/**/examples/imgui_impl_glfw.cpp" "ext/**/examples/imgui_glfw.h"
     "ext/**/tiny_obj_loader.cc" "ext/**/source/tinyply.cpp")

message(STATUS ${EXTERNAL_FILES})

file(GLOB SHADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/*.frag")

include_directories(${GLRT_INCLUDE_DIR} ${GLM_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS})
add_executable(${BUILD_TARGET} ${SOURCE_FILES} ${EXTERNAL_FILES} ${SHADER_FILES})

target_link_libraries(${BUILD_TARGET} ${OPENGL_LIBRARIES} ${GLFW3_LIBRARIES})

# ----------------------------------------------------------------------------------------------------------------------
# Move ImGui font files
# ----------------------------------------------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin)
configure_file(${CMAKE_CURRENT_LIST_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COPYONLY)

# ----------------------------------------------------------------------------------------------------------------------
# Debug settings for MSVC
# ----------------------------------------------------------------------------------------------------------------------
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    set_property(TARGET ${BUILD_TARGET} APPEND PROPERTY LINK_FLAGS "/DEBUG /PROFILE")
endif()