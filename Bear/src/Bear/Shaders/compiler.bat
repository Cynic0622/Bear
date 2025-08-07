@echo off
set "VULKAN_SDK_PATH=%VULKAN_SDK%"
if not defined VULKAN_SDK_PATH (
    echo VULKAN_SDK environment variable is not set. Please install Vulkan SDK.
    pause
    exit /b 1
)

set "GLSLC_PATH=%VULKAN_SDK_PATH%\Bin\glslc.exe"
if not exist "%GLSLC_PATH%" (
    echo glslc.exe not found at %GLSLC_PATH%
    pause
    exit /b 1
)

echo Compiling shaders...

"%GLSLC_PATH%" tri.vert -o tri.vert.spv
"%GLSLC_PATH%" tri.frag -o tri.frag.spv

echo Done.
pause