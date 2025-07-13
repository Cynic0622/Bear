# ==================== Bear/CMakeLists.txt ====================
# Bear 引擎库项目

# 按类别收集源文件
file(GLOB_RECURSE BEAR_SRC_SOURCES
    "src/*.cpp"
    "src/*.h"
)

# 创建Bear动态库
add_library(Bear SHARED ${BEAR_SRC_SOURCES})

# 设置VS中的源文件分组
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${BEAR_SRC_SOURCES})

# 设置包含目录
target_include_directories(Bear
    PUBLIC
        src  # 公共接口
        extern/spdlog/include
)

# DLL导出设置
target_compile_definitions(Bear 
    PRIVATE 
        BEAR_BUILD_DLL
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
    
    target_link_libraries(Bear PRIVATE
        kernel32 user32 gdi32 winspool comdlg32 
        advapi32 shell32 ole32 oleaut32 uuid 
        odbc32 odbccp32
    )
endif()
