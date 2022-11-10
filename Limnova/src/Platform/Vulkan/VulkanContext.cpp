#include "VulkanContext.h"

#include "vkinit.h"
namespace Vk = LimnovaVk;


namespace Limnova
{

    VulkanContext::VulkanContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        LV_CORE_ASSERT(m_WindowHandle, "Window handle is null!");
    }

    void VulkanContext::Init()
    {        
        // Use Volk to load the initial parts of the Vulkan API that are required
        // to create a Vulkan instance.
        auto const res = volkInitialize();
        LV_CORE_ASSERT(res == VK_SUCCESS, "Unable to load Vulkan API: Volk returned error {0}", res);

        // Use vkEnumerateInstanceVersion() to tell us the version of the
        // Vulkan loader.
        std::uint32_t loaderVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        if (vkEnumerateInstanceVersion)
        {
            auto const res = vkEnumerateInstanceVersion(&loaderVersion);
            LV_CORE_ASSERT(res == VK_SUCCESS, "vkEnumerateInstanceVersion() returned {0}", res);            
        }
        LV_CORE_INFO("Vulkan loader version: {0}.{1}.{2} (variant {3})", VK_API_VERSION_MAJOR(loaderVersion), VK_API_VERSION_MINOR(loaderVersion), VK_API_VERSION_PATCH(loaderVersion), VK_API_VERSION_VARIANT(loaderVersion));


        // Check instance layers and extensions.
        auto const supportedLayers = Vk::get_instance_layers();
        auto const supportedExtensions = Vk::get_instance_extensions();
        std::vector< char const* > enabledLayers, enabledExtensions;
        bool enableDebugUtils = false;

        // GLFW may require a number of instance extensions
        std::uint32_t reqExtCount = 0;
        char const** requiredExt = glfwGetRequiredInstanceExtensions(&reqExtCount);
        for (std::uint_fast32_t i = 0; i < reqExtCount; ++i)
        {
            if (!supportedExtensions.count(requiredExt[i]))
            {
                LV_CORE_ASSERT(false, "GLFW/Vulkan: required instance extension {0} not supported",
                    requiredExt[i]
                );
            }
            enabledExtensions.emplace_back(requiredExt[i]);
        }

#	if defined(LV_DEBUG)
        if (supportedLayers.count("VK_LAYER_KHRONOS_validation"))
        {
            enabledLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }

        if (supportedExtensions.count("VK_EXT_debug_utils"))
        {
            enableDebugUtils = true;
            enabledExtensions.emplace_back("VK_EXT_debug_utils");
        }
#	endif

        for (auto const& layer : enabledLayers)
        {
            LV_CORE_INFO("Enabling layer: {0}", layer);
        }
        for (auto const& extension : enabledExtensions)
        {
            LV_CORE_INFO("Enabling instance extension: {0}", extension);
        }

        // Create Vulkan instance
        m_Instance = Vk::create_instance(enabledLayers, enabledExtensions, enableDebugUtils);
        LV_CORE_ASSERT(VK_NULL_HANDLE != m_Instance, "Failed to create Vulkan instance!");

        // Instruct Volk to load the remainder of the Vulkan API.
        volkLoadInstance(m_Instance);

        // Setup debug messenger.
        if (enableDebugUtils)
        {
            m_DebugMessenger = Vk::create_debug_messenger(m_Instance);
        }

        // Create Vulkan surface
        {
            auto const res = glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to create VkSurfaceKHR: glfwCreateWindowSurface() returned {0}", res);
        }

        // Print Vulkan devices.
        Vk::enumerate_devices(m_Instance);

        // Select appropriate Vulkan device.
        m_PhysicalDevice = Vk::select_device(m_Instance, m_Surface);
        if (VK_NULL_HANDLE == m_PhysicalDevice)
        {
            vkDestroyInstance(m_Instance, nullptr);
            LV_CORE_ASSERT(false, "No suitable physical device found!");
        }
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
            LV_CORE_INFO("Selected device: {0}", props.deviceName);
        }

        // Enable required extensions.
        std::vector<char const*> enabledDevExensions;
        enabledDevExensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        for (auto const& ext : enabledDevExensions)
        {
            LV_CORE_INFO("Enabling device extension: {0}", ext);
        }

        // We need one or two queues:
        // - best case: one GRAPHICS queue that can present
        // - otherwise: one GRAPHICS queue and any queue that can present
        std::vector<std::uint32_t> queueFamilyIndices;
        if (auto const index = Vk::find_queue_family(m_PhysicalDevice, VK_QUEUE_GRAPHICS_BIT, m_Surface))
        {
            m_GraphicsFamilyIndex = *index;
            queueFamilyIndices.emplace_back(*index);
        }
        else
        {
            auto graphics = Vk::find_queue_family(m_PhysicalDevice, VK_QUEUE_GRAPHICS_BIT);
            auto present = Vk::find_queue_family(m_PhysicalDevice, 0, m_Surface);
            LV_CORE_ASSERT(graphics && present, "Missing suitable queue families!");

            m_GraphicsFamilyIndex = *graphics;
            m_PresentFamilyIndex = *present;

            queueFamilyIndices.emplace_back(*graphics);
            queueFamilyIndices.emplace_back(*present);
        }

        // Create a logical device from the graphics queue family.
        m_Device = Vk::create_device(m_PhysicalDevice, queueFamilyIndices, enabledDevExensions);
        if (VK_NULL_HANDLE == m_Device)
        {
            vkDestroyInstance(m_Instance, nullptr);
            LV_CORE_ASSERT(false, "Failed to create physical device!");
        }

        // Optionally: specialise Vulkan functions for this device.
        //volkLoadDevice(m_Device);

        // Retrieve the device's queues.
        vkGetDeviceQueue(m_Device, m_GraphicsFamilyIndex, 0, &m_GraphicsQueue);
        LV_CORE_ASSERT(VK_NULL_HANDLE != m_GraphicsQueue, "Failed to retrieve graphics queue!");

        if (queueFamilyIndices.size() >= 2)
        {
            vkGetDeviceQueue(m_Device, m_PresentFamilyIndex, 0, &m_PresentQueue);
        }
        else
        {
            m_PresentFamilyIndex = m_GraphicsFamilyIndex;
            m_PresentQueue = m_GraphicsQueue;
        }

        // Create swap chain
        std::tie(m_Swapchain, m_SwapchainFormat, m_SwapchainExtent) = Vk::create_swapchain(m_PhysicalDevice, m_Surface, m_Device, m_WindowHandle, queueFamilyIndices);
        
        // Get swap chain images & create associated image views
        Vk::get_swapchain_images(m_Device, m_Swapchain, m_SwapImages);
        Vk::create_swapchain_image_views(m_Device, m_SwapchainFormat, m_SwapImages, m_SwapViews);

        // Get maximum supported sample count for MSAA.
        m_MsaaSamples = Vk::get_max_usable_sample_count(m_PhysicalDevice);        
    }


    void VulkanContext::Shutdown()
    {
        // Vulkan cleanup
        for (auto const view : m_SwapViews)
        {
            if (VK_NULL_HANDLE != view)
                vkDestroyImageView(m_Device, view, nullptr);
        }
        if (VK_NULL_HANDLE != m_Swapchain)
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

        if (VK_NULL_HANDLE != m_Surface)
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

        if (VK_NULL_HANDLE != m_Device)
            vkDestroyDevice(m_Device, nullptr);

        if (VK_NULL_HANDLE != m_DebugMessenger)
            vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

        if (VK_NULL_HANDLE != m_Instance)
            vkDestroyInstance(m_Instance, nullptr);
    }


    void VulkanContext::SwapBuffers()
    {
    }

}
