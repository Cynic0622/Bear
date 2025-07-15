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

file(GLOB BEAR_PLATFORM_SOURCES
    "src/Platform/*.cpp"
    "src/Platform/*.h"
)


set(BEAR_ALL_SOURCES
    ${BEAR_SOURCES}
    ${BEAR_EVENT_SOURCES}
    ${BEAR_PLATFORM_SOURCES}
)

# 创建Bear动态库
add_library(Bear SHARED ${BEAR_ALL_SOURCES})

# 设置VS中的源文件分组
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Bear" PREFIX "Bear" FILES ${BEAR_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Bear/Events" PREFIX "Bear/Events" FILES ${BEAR_EVENT_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform" PREFIX "Platform/Windows" FILES ${BEAR_PLATFORM_SOURCES})

# 链接GLFW
target_link_libraries(Bear PUBLIC glfw)
target_link_libraries(Bear PUBLIC spdlog)

# 设置包含目录
target_include_directories(Bear
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # 公共接口
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Bear
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw/include
)

# DLL导出设置
target_compile_definitions(Bear 
    PRIVATE
        BEAR_ENABLE_ASSERTS
        # BEAR_BUILD_DLL
        BEAR_API_EXPORTS
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
