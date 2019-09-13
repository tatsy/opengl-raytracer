include(FindPackageHandleStandardArgs)

set(VULKAN_DIR "" CACHE PATH "")
message(STATUS ${VULKAN_DIR})

find_path(VULKAN_INCLUDE_DIR
          NAMES vulkan/vulkan.h
          PATHS
          /usr/include
          /usr/local/include
          ${VULKAN_DIR}
          ${VULKAN_DIR}/include)

find_library(VULKAN_LIBRARY
             NAMES vulkan vulkan-1
             PATHS
             /usr/lib
             /usr/local/lib
             ${VULKAN_DIR}
             ${VULKAN_DIR}/lib)

find_package_handle_standard_args(
    Vulkan
    DEFALUT_MSG
    VULKAN_INCLUDE_DIR
    VULKAN_LIBRARY
)

if (VULKAN_FOUND)
    message(STATUS "Vulkan include: ${VULKAN_INCLUDE_DIR}")
    message(STATUS "Vulkan library: ${VULKAN_LIBRARY}")
    set(VULKAN_INCLUDE_DIRS ${VULKAN_INCLUDE_DIR} CACHE PATH "")
    set(VULKAN_LIBRARIES ${VULKAN_LIBRARY} CACHE FILEPATH "")
    mark_as_advanced(VULKAN_INCLUDE_DIR VULKAN_LIBRARY)
endif()
