# ==================== Sandbox/CMakeLists.txt ====================
# Sandbox 应用程序项目

# 收集源文件
file(GLOB SANDBOX_SOURCES
    "src/*.cpp"
    "src/*.h"
)

# 创建可执行文件
add_executable(Sandbox ${SANDBOX_SOURCES})

# 设置VS中的源文件分组
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SANDBOX_SOURCES})

# 设置项目属性
set_target_properties(Sandbox PROPERTIES
    # 控制台应用
    WIN32_EXECUTABLE FALSE
)

# 链接Bear库
target_link_libraries(Sandbox PRIVATE Bear)

# 设置包含目录（通过Bear库的PUBLIC接口自动获得）
# target_include_directories(Sandbox PRIVATE
#     ../Bear/src  # 不再需要，通过Bear的PUBLIC接口自动获得
# )

# 编译选项
if(MSVC)
    target_compile_options(Sandbox PRIVATE /W4 /utf-8)
endif()

# 编译定义
target_compile_definitions(Sandbox PRIVATE
    BEAR_PLATFORM_WINDOWS
    $<$<CONFIG:Debug>:BEAR_DEBUG>
    $<$<CONFIG:Release>:BEAR_RELEASE>
)

# 复制Bear.dll到Sandbox目录（Windows DLL依赖处理）
# if(WIN32)
#     add_custom_command(TARGET Sandbox POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:Bear>
#         $<TARGET_FILE_DIR:Sandbox>
#         COMMENT "Copying Bear.dll to Sandbox directory"
#     )
# endif()

# 设置调试工作目录
set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Sandbox>"
)