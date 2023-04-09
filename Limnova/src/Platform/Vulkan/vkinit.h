#pragma once

#include <Volk/volk.h>
#include <GLFW/glfw3.h>


namespace LimnovaVk
{

    std::unordered_set< std::string > get_instance_layers();
    std::unordered_set< std::string > get_instance_extensions();
    VkInstance create_instance(
        std::vector< char const* > const& enabledLayers,
        std::vector< char const* > const& enabledInstanceExtensions,
        bool enableDebugUtils
    );
    VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance);
    void enumerate_devices(VkInstance);
    VkPhysicalDevice select_device(VkInstance, VkSurfaceKHR);
    float score_device(VkPhysicalDevice, VkSurfaceKHR);
    std::optional<std::uint32_t> find_queue_family(VkPhysicalDevice, VkQueueFlags, VkSurfaceKHR = VK_NULL_HANDLE);
    VkDevice create_device(VkPhysicalDevice, std::vector<std::uint32_t> const& queueFamilies, std::vector<char const*> const& enabledDeviceExtensions);
    std::unordered_set<std::string> get_device_extensions(VkPhysicalDevice);
    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> create_swapchain(VkPhysicalDevice, VkSurfaceKHR, VkDevice, GLFWwindow*, std::vector<std::uint32_t> const& aQueueFamilyIndices = {}, VkSwapchainKHR = VK_NULL_HANDLE);
    std::vector<VkSurfaceFormatKHR> get_surface_formats(VkPhysicalDevice, VkSurfaceKHR);
    std::unordered_set<VkPresentModeKHR> get_present_modes(VkPhysicalDevice, VkSurfaceKHR);
    void get_swapchain_images(VkDevice, VkSwapchainKHR, std::vector<VkImage>&);
    void create_swapchain_image_views(VkDevice, VkFormat, std::vector<VkImage> const&, std::vector<VkImageView>&);
    VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice);

}
