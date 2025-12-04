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

"%GLSLC_PATH%" pbr.vert -o pbrVert.spv
"%GLSLC_PATH%" pbr.frag -o pbrFrag.spv
"%GLSLC_PATH%" preZ.frag -o preZfrag.spv
"%GLSLC_PATH%" oitBlend.vert -o oitBlendVert.spv
"%GLSLC_PATH%" oitBlend.frag -o oitBlendFrag.spv
"%GLSLC_PATH%" oitFrag.frag -o oitFrag.spv
"%GLSLC_PATH%" oitVert.vert -o oitVert.spv

dxc.exe -T ps_6_6 -E main -spirv ntcPS.hlsl -Fo ntcPS.spv -I "..\..\extern\rtxntc\include" -I "..\Renderer\Common"  -I "..\..\extern\rtxntc\src"  -I "..\..\extern\rtxtf" -enable-16bit-types -O3 -fspv-target-env=vulkan1.2
dxc.exe -T vs_6_6 -E main -spirv ntcVS.hlsl -Fo ntcVS.spv -I "..\..\extern\rtxntc\include" -enable-16bit-types -O3 -fspv-target-env=vulkan1.2 -fvk-use-gl-layout
dxc.exe -T vs_6_6 -E main -spirv preZ.hlsl -Fo preZvert.spv -O3 -fspv-target-env=vulkan1.2 -fvk-use-dx-layout
echo Done.
pause