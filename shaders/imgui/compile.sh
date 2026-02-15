#!/usr/bin/sh
slangc patch.slang -profile spirv_1_3 -entry vsMain -stage vertex -o vert.spv -O3
slangc patch.slang -profile spirv_1_3 -entry fsMain -stage fragment -o frag.spv -O3

# dxc  patch.hlsl -T ps_6_7  -spirv -Fo frag.spv -E fsMain -fspv-entrypoint-name=main -O3
# dxc  patch.hlsl -T vs_6_7  -spirv -Fo vert.spv -E vsMain -fspv-entrypoint-name=main -O3