add_rules("mode.debug", "mode.release")

add_requires("glfw")
add_requires("glm", "vulkan-hpp", "vulkan-memory-allocator", "stb", {system = false})

includes("third_party/")

target("learn_vulkan", function()
    set_kind("binary")
    set_languages("c17", "c++23")
    add_files("src/**.cpp")
    add_packages("glfw", "glm", "vulkan-hpp", "vulkan-memory-allocator", "stb")
    add_deps("imgui_vulkan_glfw")
    add_defines("GLFW_INCLUDE_VULKAN")
    add_defines("VK_NO_PROTOTYPES")
    add_defines("VULKAN_HPP_NO_CONSTRUCTORS")
    add_defines("VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1")
    add_defines("VMA_STATIC_VULKAN_FUNCTIONS=0","VMA_DYNAMIC_VULKAN_FUNCTIONS=1")
    on_load(function(target)
        if (is_mode("debug")) then
            target:add("defines", "_DEBUG")
        end
    end)
end)
