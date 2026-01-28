add_requires("vulkan-headers", "glfw")

target("stb", function () 
    set_kind("object")
    add_includedirs("./", {public= true})
    add_files("./*.c")
end)