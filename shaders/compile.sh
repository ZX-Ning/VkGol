#!/usr/bin/sh
slangc shader.slang -profile spirv_1_3 -matrix-layout-row-major -o shader.spv -O3 
slangc gol.slang -profile spirv_1_3 -stage compute -entry update -o gol.spv  -O3