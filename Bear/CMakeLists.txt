# ==================== Bear/CMakeLists.txt ====================
# Bear 引擎库项目

# 按类别收集源文件
file(GLOB BEAR_SOURCES
    "src/Bear/*.cpp"
    "src/Bear/*.h"
)
file(GLOB BEAR_EVENT_SOURCES
    "src/Bear/Events/*.cpp"
    "src/Bear/Events/*.h"
)

file(GLOB BEAR_PLATFORM_WIN_SOURCES
    "src/Platform/Windows/*.cpp"
    "src/Platform/Windows/*.h"
)

file(GLOB BEAR_GUI_SOURCES
    "src/Bear/Gui/*.cpp"
    "src/Bear/Gui/*.h"
)

file(GLOB BEAR_BACKEND_OPENGL_SOURCES
    "src/Backend/OpenGL/*.cpp"
    "src/Backend/OpenGL/*.h"
)

set(VULKAN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan")
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

set(BEAR_BACKEND_VULKAN_SOURCES
    ${CORE_SOURCES}
    ${RESOURCES_SOURCES}
    ${PIPELINE_SOURCES}
    ${PRESENTATION_SOURCES}
    ${COMMAND_SOURCES}
    ${SYNC_SOURCES}
    ${INTEGRATION_SOURCES}
)

file(GLOB RENDERER_COMMON_SOURCES
    "src/Renderer/Common/*.cpp"
    "src/Renderer/Common/*.h"
)
# 着色器文件
file(GLOB BEAR_SHADERS_SOURCES
    "src/Bear/Shaders/*.vert"
    "src/Bear/Shaders/*.frag"
    "src/Bear/Shaders/*.comp"
)
file(GLOB RENERDER_RHI_SOURCES
    "src/Renderer/RHI/*.cpp"
    "src/Renderer/RHI/*.h"
)
file(GLOB RENDERER_SOURCES
    "src/Renderer/*.cpp"
    "src/Renderer/*.h"
)

file(GLOB BEAR_CORE_SOURCES
    "src/Core/*.cpp"
    "src/Core/*.h"
)

file(GLOB BEAR_SCENE_SOURCES
    "src/Scene/*.cpp"
    "src/Scene/*.h"
)
set(BEAR_ALL_SOURCES
    ${BEAR_SOURCES}
    ${BEAR_EVENT_SOURCES}
    ${BEAR_PLATFORM_WIN_SOURCES}
    ${BEAR_GUI_SOURCES}
    ${BEAR_BACKEND_OPENGL_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/src/glad.c
    ${BEAR_BACKEND_VULKAN_SOURCES}
    ${BEAR_SHADERS_SOURCES}
    ${RENDERER_COMMON_SOURCES}
    ${RENERDER_RHI_SOURCES}
    ${RENDERER_SOURCES}
    ${BEAR_CORE_SOURCES}
    ${BEAR_SCENE_SOURCES}
)

# set(GLAD_SOURCES
#     ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/src/glad.c)

# # 创建glad静态库
# # add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/src/glad.c)
# # target_include_directories(glad PUBLIC
# #     ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
# # )
# 确保glad.c被当作C文件编译
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/src/glad.c
    PROPERTIES 
    LANGUAGE C
    SKIP_PRECOMPILE_HEADERS ON
)

# 创建Bear动态库
add_library(Bear SHARED ${BEAR_ALL_SOURCES})



# target_include_directories(Vulkan PUBLIC
#     "${VULKAN_DIR}/Core"
#     "${VULKAN_DIR}/Resources"
#     "${VULKAN_DIR}/Pipeline"
#     "${VULKAN_DIR}/Presentation"
#     "${VULKAN_DIR}/Command"
#     "${VULKAN_DIR}/Sync"
#     "${VULKAN_DIR}/Integration"
# )

find_package(Vulkan REQUIRED)
target_include_directories(Bear PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Bear PRIVATE ${Vulkan_LIBRARIES})

# 设置VS中的源文件分组
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Bear" PREFIX "Bear" FILES ${BEAR_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Bear/Events" PREFIX "Bear/Events" FILES ${BEAR_EVENT_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows" PREFIX "Platform/Windows" FILES ${BEAR_PLATFORM_WIN_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Bear/Gui" PREFIX "Bear/Gui" FILES ${BEAR_GUI_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Backend/OpenGL" PREFIX "Renderer/Backend/OpenGL" FILES ${BEAR_BACKEND_OPENGL_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Bear/Shaders" PREFIX "Bear/Shaders" FILES ${BEAR_SHADERS_SOURCES})

source_group(TREE "${VULKAN_DIR}/Core" PREFIX "Renderer/Backend/Vulkan/Core" FILES ${CORE_SOURCES})
source_group(TREE "${VULKAN_DIR}/Resources" PREFIX "Renderer/Backend/Vulkan/Resources" FILES ${RESOURCES_SOURCES})
source_group(TREE "${VULKAN_DIR}/Pipeline" PREFIX "Renderer/Backend/Vulkan/Pipeline" FILES ${PIPELINE_SOURCES})
source_group(TREE "${VULKAN_DIR}/Presentation" PREFIX "Renderer/Backend/Vulkan/Presentation" FILES ${PRESENTATION_SOURCES})
source_group(TREE "${VULKAN_DIR}/Command" PREFIX "Renderer/Backend/Vulkan/Command" FILES ${COMMAND_SOURCES})
source_group(TREE "${VULKAN_DIR}/Sync" PREFIX "Renderer/Backend/Vulkan/Sync" FILES ${SYNC_SOURCES})
source_group(TREE "${VULKAN_DIR}/Integration" PREFIX "Renderer/Backend/Vulkan/Integration" FILES ${INTEGRATION_SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Common" PREFIX "Renderer/Common" FILES ${RENDERER_COMMON_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/RHI" PREFIX "Renderer/RHI" FILES ${RENERDER_RHI_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer" PREFIX "Renderer" FILES ${RENDERER_SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Core" PREFIX "Core" FILES ${BEAR_CORE_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Scene" PREFIX "Scene" FILES ${BEAR_SCENE_SOURCES})


target_link_libraries(Bear PUBLIC
    glfw
    spdlog
    imgui
    VulkanMemoryAllocator
    glm
    EnTT::EnTT
    libntc
)

# 设置包含目录
target_include_directories(Bear
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # 公共接口
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Bear
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Scene
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Command
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Integration
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Presentation
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Resources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Backend/Vulkan/Sync
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/vma/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/tinyobjloader
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/entt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/tinygltf
)

# DLL导出设置
target_compile_definitions(Bear 
    PRIVATE
        BEAR_ENABLE_ASSERTS
        # BEAR_BUILD_DLL
        BEAR_API_EXPORTS
        GLFW_INCLUDE_NONE  # 避免GLFW包含OpenGL头文件
    PUBLIC
        $<$<CONFIG:Debug>:BEAR_DEBUG>
        $<$<CONFIG:Release>:BEAR_RELEASE>
        $<$<CONFIG:Release>:NDEBUG>
)

# 编译选项
if(MSVC)
    target_compile_options(Bear PRIVATE /W4 /utf-8)
endif()

# Windows特定设置
if(WIN32)
    target_compile_definitions(Bear PRIVATE
        BEAR_PLATFORM_WINDOWS
        _CRT_SECURE_NO_WARNINGS
    )
    # --- 在这里添加预编译头文件设置 ---
    # 将 bearpch.h 指定为 Bear 目标的预编译头文件。
    # PRIVATE 表示这个 PCH 设置只对 Bear 目标内部生效，不会影响链接到它的其他目标（如 Sandbox）。
    target_precompile_headers(Bear PRIVATE src/Bear/bearpch.h)

    target_link_libraries(Bear PRIVATE
        kernel32 user32 gdi32 winspool comdlg32 
        advapi32 shell32 ole32 oleaut32 uuid 
        odbc32 odbccp32
    )
endif()
