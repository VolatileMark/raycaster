# Raycaster

A simple raycaster (made with [raylib](https://github.com/raysan5/raylib)) I made to learn about compute shaders.  
This project provides both a CPU and GPU based renderers examples.  

The [CPU based renderer](src/cpu.c) renders the scene using the CPU. It's slow and has to render at a lower resolution. Build it using `cmake <path to project> -DUSE_COMPUTE_SHADERS=OFF`.  
The [GPU based renderer](src/gpu.c) instead uses [compute](shaders/wall.glsl) and [fragment](shaders/frag.glsl) shaders to render the scene at a much higher resolution and framerate. Build it using `cmake <path to project> -DUSE_COMPUTE_SHADERS=ON`. **Note**: the executable has to be run from the root directory of the project, otherwise it won't find the shader files.
