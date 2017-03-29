echo %1
echo %2
%VK_SDK_PATH%/Bin32/glslangValidator.exe -V %1/cube.vert -o %2/vert.spv
%VK_SDK_PATH%/Bin32/glslangValidator.exe -V %1/cube.frag -o %2/frag.spv
pause