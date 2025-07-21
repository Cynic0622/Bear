set(VULKAN_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB CORE_SOURCES
    "${VULKAN_DIR}/Core/*.cpp"
    "${VULKAN_DIR}/Core/*.h"
)

file(GLOB RESOURCES_SOURCES
    "${VULKAN_DIR}/Resources/*.cpp"
    "${VULKAN_DIR}/Resources/*.h"
)

file(GLOB PIPELINE_SOURCES
    "${VULKAN_DIR}/Pipeline/*.cpp"
    "${VULKAN_DIR}/Pipeline/*.h"
)

file(GLOB PRESENTATION_SOURCES
    "${VULKAN_DIR}/Presentation/*.cpp"
    "${VULKAN_DIR}/Presentation/*.h"
)

file(GLOB COMMAND_SOURCES
    "${VULKAN_DIR}/Command/*.cpp"
    "${VULKAN_DIR}/Command/*.h"
)

file(GLOB SYNC_SOURCES
    "${VULKAN_DIR}/Sync/*.cpp"
    "${VULKAN_DIR}/Sync/*.h"
)

file(GLOB INTEGRATION_SOURCES
    "${VULKAN_DIR}/Integration/*.cpp"
    "${VULKAN_DIR}/Integration/*.h"
)

set(VULKAN_SOURCES
    ${CORE_SOURCES}
    ${RESOURCES_SOURCES}
    ${PIPELINE_SOURCES}
    ${PRESENTATION_SOURCES}
    ${COMMAND_SOURCES}
    ${SYNC_SOURCES}
    ${INTEGRATION_SOURCES}
)

add_library(Vulkan STATIC ${VULKAN_SOURCES})

target_include_directories(Vulkan PUBLIC
    "${VULKAN_DIR}/Core"
    "${VULKAN_DIR}/Resources"
    "${VULKAN_DIR}/Pipeline"
    "${VULKAN_DIR}/Presentation"
    "${VULKAN_DIR}/Command"
    "${VULKAN_DIR}/Sync"
    "${VULKAN_DIR}/Integration"
)

source_group(TREE "${VULKAN_DIR}/Core" PREFIX "Vulkan/Core" FILES ${CORE_SOURCES})
source_group(TREE "${VULKAN_DIR}/Resources" PREFIX "Vulkan/Resources" FILES ${RESOURCES_SOURCES})
source_group(TREE "${VULKAN_DIR}/Pipeline" PREFIX "Vulkan/Pipeline" FILES ${PIPELINE_SOURCES})
source_group(TREE "${VULKAN_DIR}/Presentation" PREFIX "Vulkan/Presentation" FILES ${PRESENTATION_SOURCES})
source_group(TREE "${VULKAN_DIR}/Command" PREFIX "Vulkan/Command" FILES ${COMMAND_SOURCES})
source_group(TREE "${VULKAN_DIR}/Sync" PREFIX "Vulkan/Sync" FILES ${SYNC_SOURCES})
source_group(TREE "${VULKAN_DIR}/Integration" PREFIX "Vulkan/Integration" FILES ${INTEGRATION_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(Vulkan PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Vulkan PRIVATE ${Vulkan_LIBRARIES})