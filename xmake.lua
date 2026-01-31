add_rules("mode.debug", "mode.release")
set_policy("build.warning", true)
set_warnings("all", "extra")

if (is_plat("linux")) then
    add_requires("glfw", {configs = {wayland = true}})
else
    add_requires("glfw", {system = false})
end

add_requires("glm", "vulkan-hpp", "vulkan-memory-allocator", {system = false})

includes("third_party/")

target("learn_vulkan", function()
    set_kind("binary")
    set_languages("c17", "c++23")
    add_files("src/**.cpp")
    add_packages("glfw", "glm", "vulkan-hpp", "vulkan-memory-allocator")
    add_deps("imgui_vulkan_glfw", "stb")
    add_defines("GLFW_INCLUDE_VULKAN")
    add_defines("VK_NO_PROTOTYPES")
    add_defines("VULKAN_HPP_NO_CONSTRUCTORS")
    add_defines("VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1")
    add_defines("VMA_STATIC_VULKAN_FUNCTIONS=0","VMA_DYNAMIC_VULKAN_FUNCTIONS=1")
end)
