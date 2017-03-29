echo %1
echo %2
%VK_SDK_PATH%/Bin32/glslangValidator.exe -V %1/cube1.vert -o %2/vert1.spv
%VK_SDK_PATH%/Bin32/glslangValidator.exe -V %1/cube1.frag -o %2/frag1.spv
pause