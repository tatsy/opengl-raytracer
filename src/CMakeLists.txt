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
# GLRT library
# ----------------------------------------------------------------------------------------------------------------------
file(GLOB CORE_FILES
     "core/*.cpp"
     "core/*.hpp"
     "core/*.c"
     "core/*.h")

file(GLOB VULKAN_FILES
     "vulkan/*.cpp"
     "vulkan/*.hpp"
     "vulkan/*.c"
     "vulkan/*.h")

file(GLOB EXTERNAL_FILES
     "ext/glad/*.c"
     "ext/glad/*.h"
     "ext/stb/stb_image.h"
     "ext/stb/stb_image_write.h"
     "ext/imgui/*.cpp"
     "ext/imgui/*.h"
     "ext/imgui/examples/imgui_impl_opengl3.cpp"
     "ext/imgui/examples/imgui_opengl3.h"
     "ext/imgui/examples/imgui_impl_glfw.cpp"
     "ext/imgui/examples/imgui_glfw.h"
     "ext/tinyxml2/tinyxml2.h"
     "ext/tinyxml2/tinyxml2.cpp"
     "ext/json11/json11.hpp"
     "ext/json11/json11.cpp"
     "ext/tinyply/source/tinyply.h"
     "ext/tinyply/source/tinyply.cpp"
     "ext/tinyobjloader/tiny_obj_loader.h"
     "ext/tinyobjloader/tiny_obj_loader.cc")

message(STATUS ${EXTERNAL_FILES})

file(GLOB SHADER_FILES
     "${CMAKE_CURRENT_LIST_DIR}/shaders/*.vert"
     "${CMAKE_CURRENT_LIST_DIR}/shaders/*.geom"
     "${CMAKE_CURRENT_LIST_DIR}/shaders/*.frag")

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${GLRT_INCLUDE_DIR} ${GLM_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS})

if (WITH_VULKAN)
    add_definitions("-DGLFW_INCLUDE_VULKAN")
    include_directories(${VULKAN_INCLUDE_DIRS})
endif()

add_library(${GLRT_LIBRARY} SHARED ${CORE_FILES} ${VULKAN_FILES} ${EXTERNAL_FILES} ${SHADER_FILES})

source_group("Shader Files" FILES ${SHADER_FILES})
source_group("External Files" FILES ${EXTERNAL_FILES})
source_group("core" FILES ${CORE_FILES})
source_group("vulkan" FILES ${VULKAN_FILES})

target_link_libraries(${GLRT_LIBRARY} ${OPENGL_LIBRARIES} ${GLFW3_LIBRARIES} ${CXX_FS_LIBRARY})

if (WITH_VULKAN)
    target_link_libraries(${GLRT_LIBRARY} ${VULKAN_LIBRARIES})
endif()

# ----------------------------------------------------------------------------------------------------------------------
# GLRT main
# ----------------------------------------------------------------------------------------------------------------------
add_executable(${GLRT_MAIN_BINARY} main.cpp)
target_link_libraries(${GLRT_MAIN_BINARY} ${GLRT_LIBRARY})

# ----------------------------------------------------------------------------------------------------------------------
# Move ImGui font files
# ----------------------------------------------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin)
configure_file(${CMAKE_CURRENT_LIST_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COPYONLY)

# ----------------------------------------------------------------------------------------------------------------------
# Copy shader programs
# ----------------------------------------------------------------------------------------------------------------------
set(OUTPUT_SHADERS "")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

foreach (SHADER IN LISTS SHADER_FILES)
    get_filename_component(BASE_NAME ${SHADER} NAME)
    set(OUTPUT_SHADERS ${OUTPUT_SHADERS} ${SHADER_OUTPUT_DIR}/${BASE_NAME})
endforeach()

add_custom_command(OUTPUT ${OUTPUT_SHADERS}
                   COMMAND ${CMAKE_COMMAND}
                   ARGS -E remove_directory "${SHADER_OUTPUT_DIR}"
                   COMMAND ${CMAKE_COMMAND}
                   ARGS -E make_directory  "${SHADER_OUTPUT_DIR}"
                   COMMAND ${CMAKE_COMMAND}
                   ARGS -E copy ${SHADER_FILES} "${CMAKE_BINARY_DIR}/shaders"
                   DEPENDS ${SHADER_FILES})

add_custom_target(COPY_SHADER_FILES ALL
                  SOURCES ${OUTPUT_SHADERS})

add_dependencies(${GLRT_LIBRARY} COPY_SHADER_FILES)

if (WITH_VULKAN)
    set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

    foreach (SHADER IN LISTS SHADER_FILES)
        get_filename_component(BASE_NAME ${SHADER} NAME)
        set(OUTPUT_SHADER ${SHADER_OUTPUT_DIR}/${BASE_NAME}.spv)

        add_custom_command(OUTPUT ${OUTPUT_SHADER}
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E remove "${OUTPUT_SHADER}"
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E make_directory  "${SHADER_OUTPUT_DIR}"
                           COMMAND glslangValidator
                           ARGS -i "${SHADER}" -V -o "${OUTPUT_SHADER}"
                           DEPENDS ${SHADER})

        set(CUSTOM_TARGET_NAME GLSLANG_${EXPNAME}_${BASE_NAME})
        add_custom_target(${CUSTOM_TARGET_NAME} ALL SOURCES ${OUTPUT_SHADER})
        add_dependencies(${GLRT_LIBRARY} ${CUSTOM_TARGET_NAME})
        set_target_properties(${CUSTOM_TARGET_NAME} PROPERTIES FOLDER "GLSLang")
    endforeach()
endif()

# ----------------------------------------------------------------------------------------------------------------------
# Debug settings for MSVC
# ----------------------------------------------------------------------------------------------------------------------
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    set_property(TARGET ${GLRT_MAIN_BINARY} APPEND PROPERTY LINK_FLAGS "/DEBUG /PROFILE")
endif()