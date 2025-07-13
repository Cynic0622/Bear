# ==================== 根目录 CMakeLists.txt ====================
cmake_minimum_required(VERSION 3.16)

# 设置项目名称和语言
project(Bear LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
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

# 添加子项目
add_subdirectory(Bear)
add_subdirectory(Sandbox)

# 设置启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)