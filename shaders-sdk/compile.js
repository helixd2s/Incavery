var exec = require('child_process').exec;
//var cmd = 'prince -v builds/pdf/book.html -o builds/pdf/book.pdf';

let IO = (error, stdout, stderr) => {
    
    console.log(stdout);
    console.error(stderr);
};

let compileShader = (filename, output, definition = "")=>{
    if (!output) { output = filename; };
    let execString = `glslangValidator ${filename} --target-env spirv1.5 --client vulkan100 ${definition} -o ${output}.spv`;
    console.log(execString);
    exec(execString, IO);
};

compileShader("rasterization.frag", "opaque.frag", "-DOPAQUE");
compileShader("rasterization.geom", "opaque.geom", "-DOPAQUE");
compileShader("rasterization.vert", "opaque.vert", "-DOPAQUE");

compileShader("rasterization.frag", "translucent.frag");
compileShader("rasterization.geom", "translucent.geom");
compileShader("rasterization.vert", "translucent.vert");

compileShader("rayTracing.comp", "rayTracing.comp");

compileShader("render.frag", "render.frag");
compileShader("render.vert", "render.vert");
