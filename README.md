# Hello Vulkan!

This is a small repo for learning Vulkan.

Tested on windows 10 and MoltenVK on macOS.

1. build shaders

`cd shaders`

`glslc triangle.vert -o ../build/bin/tri_v.spv`

`glslc triangle.frag -o ../build/bin/tri_f.spv`

2. build program

`cd build`

`cmake .. && cmake --build .`