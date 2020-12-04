::call glslangValidator render.vert --target-env spirv1.5 --client vulkan100 -o render.vert.spv
::call glslangValidator render.frag --target-env spirv1.5 --client vulkan100 -o render.frag.spv
call node ./compile.js
pause
