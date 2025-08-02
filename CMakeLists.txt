# ==================== 根目录 CMakeLists.txt ====================
cmake_minimum_required(VERSION 3.16)

# 设置项目名称和语言
project(Bear LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 全局设置UTF-8编码（用于MSVC）
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Windows特定设置
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_SYSTEM_PROCESSOR "x64")
    else()
        set(CMAKE_SYSTEM_PROCESSOR "x86")
    endif()
    set(CMAKE_SYSTEM_VERSION 10.0)
endif()

# ==================== 全局输出目录设置 ====================
# 统一的输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 为不同配置设置子目录
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

# 添加文件夹支持
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 设置GLFW构建选项（在添加子目录之前）
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# 设置VMA构建选项
set(VMA_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_SAMPLE_SHADERS OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_REPLAY OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(VMA_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

# 添加子项目
add_subdirectory(Bear)
add_subdirectory(Sandbox)
add_subdirectory(Bear/extern/glfw)
add_subdirectory(Bear/extern/spdlog)
add_subdirectory(Bear/extern/vma)
add_subdirectory(Bear/extern/glm)

find_package(Vulkan REQUIRED)
# 创建ImGui库.lib
add_library(imgui STATIC

    Bear/extern/imgui/imgui.cpp
    Bear/extern/imgui/imgui_draw.cpp
    Bear/extern/imgui/imgui_tables.cpp
    Bear/extern/imgui/imgui_widgets.cpp
    Bear/extern/imgui/imgui_demo.cpp

    Bear/extern/imgui/backends/imgui_impl_glfw.cpp
    Bear/extern/imgui/backends/imgui_impl_opengl3.cpp
    Bear/extern/imgui/backends/imgui_impl_vulkan.cpp
)

# 设置包含目录
target_include_directories(imgui PUBLIC
    Bear/extern/imgui
    Bear/extern/imgui/backends
)

# 为ImGui链接必要的依赖
target_link_libraries(imgui PRIVATE
    glfw
    Vulkan::Vulkan
)

# 组织外部依赖到文件夹中
if (TARGET glfw)
    set_target_properties(glfw PROPERTIES FOLDER "External")
endif()
if(TARGET update_mappings)
    set_target_properties(update_mappings PROPERTIES FOLDER "External")
endif()
if(TARGET uninstall)
    set_target_properties(uninstall PROPERTIES FOLDER "External")
endif()

if(TARGET spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER "External")
endif()

if (TARGET imgui)
    set_target_properties(imgui PROPERTIES FOLDER "External")
endif()

if (TARGET glm)
    set_target_properties(glm PROPERTIES FOLDER "External")
endif()

# 设置启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)