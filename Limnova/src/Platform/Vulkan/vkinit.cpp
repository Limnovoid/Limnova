#include "vkinit.h"

#include "vktostring.h"


namespace LimnovaVk
{

    std::unordered_set< std::string > get_instance_layers()
    {
        std::uint32_t numLayers = 0;
        {
            auto const res = vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to enumerate layers: vkEnumaterInstanceLayerProperties() returned {0}", res);
        }

        std::vector< VkLayerProperties > layers(numLayers);
        {
            auto const res = vkEnumerateInstanceLayerProperties(&numLayers, layers.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get layer properties: vkEnumaterInstanceLayerProperties() returned {0}", res);
        }

        std::unordered_set< std::string > results;
        for (auto const& layer : layers)
        {
            results.insert(layer.layerName);
        }
        return results;
    }


    std::unordered_set< std::string > get_instance_extensions()
    {
        std::uint32_t numExtensions = 0;
        {
            auto const res = vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to enumerate extensions: vkEnumerateInstanceExtensionProperties() returned {0}", res);
        }
        std::vector< VkExtensionProperties > extensions(numExtensions);
        {
            auto const res = vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensions.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get extension properties: vkEnumerateInstanceExtensionProperties() returned {0}", res);
        }

        std::unordered_set<std::string> res;
        for (auto const& extension : extensions)
        {
            res.insert(extension.extensionName);
        }
        return res;
    }


    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
        VkDebugUtilsMessengerCallbackDataEXT const* data, void* /*aUserPtr*/)
    {
        LV_CORE_ERROR("Vulkan error: {0} ({1}): {2} ({3})\n{4}",
            to_string(severity), message_type_flags(type),
            data->pMessageIdName, data->messageIdNumber, data->pMessage
        );
        return VK_FALSE;
    }


    VkInstance create_instance(
        std::vector< char const* > const& enabledLayers,
        std::vector< char const* > const& enabledExtensions,
        bool enableDebugUtils
    )
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Limnova Engine";
        appInfo.applicationVersion = 2022;
        appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledLayerCount = (std::uint32_t)enabledLayers.size();
        instanceInfo.ppEnabledLayerNames = enabledLayers.data();
        instanceInfo.enabledExtensionCount = (std::uint32_t)enabledExtensions.size();
        instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

        // If we wish to receive validation information on instance creation,
        // we need to provide additional information to vkCreateInstance(). This
        // is done by linking an instance of VkDebugUtilsMessengerCreateInfoEXT
        // into the pNext chain of VkInstanceCreateInfo.
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        if (enableDebugUtils)
        {
            debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugInfo.pfnUserCallback = &vulkan_debug_callback;
            debugInfo.pUserData = nullptr;

            debugInfo.pNext = instanceInfo.pNext;
            instanceInfo.pNext = &debugInfo;
        }

        VkInstance instance = VK_NULL_HANDLE;
        auto const res = vkCreateInstance(&instanceInfo, nullptr, &instance);
        LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to create Vulkan instance: vkCreateInstance() returned {0}", res);
        return instance;
    }


    VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance instance)
    {
        LV_CORE_ASSERT(VK_NULL_HANDLE != instance, "Vulkan instance is null!");

        // Set up debug messaging for the rest of the application.
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT

        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        debugInfo.pfnUserCallback = &vulkan_debug_callback;
        debugInfo.pUserData = nullptr;

        VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
        auto const res = vkCreateDebugUtilsMessengerEXT(instance, &debugInfo, nullptr, &messenger);
        LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to set up debug messenger: vkCreateDebugUtilsMessengerEXT() returned {0}", res);
        return messenger;
    }


    void enumerate_devices(VkInstance instance)
    {
        LV_CORE_ASSERT(VK_NULL_HANDLE != instance, "Vulkan instance is null!");

        // Call vkEnumeratePhysicalDevices once to get the number of physical devices.
        std::uint32_t numDevices = 0;
        {
            auto const res = vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Enable to get physical device count: vkEnumeratePhysicalDevices() returned {0}", res);
        }
        // Call vkEnumeratePhysicalDevices again to get the list of devices.
        std::vector< VkPhysicalDevice > devices(numDevices, VK_NULL_HANDLE);
        {
            auto const res = vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get physical device list: vkEnumeratePhysicalDevices() returned {0}", res);
        }
        // Print all device information.
        LV_CORE_INFO("Found {0} devices:", devices.size());
        for (auto const device : devices)
        {
            // Query Vulkan for device properties.
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            auto const versionMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
            auto const versionMinor = VK_API_VERSION_MINOR(properties.apiVersion);
            auto const versionPatch = VK_API_VERSION_PATCH(properties.apiVersion);

            // Print device properties.
            LV_CORE_INFO("- {0} (Vulkan: {1}.{2}.{3}, Driver: {4})",
                properties.deviceName,
                versionMajor, versionMinor, versionPatch,
                driver_version(properties.vendorID, properties.driverVersion)
            );
            LV_CORE_INFO("  - Type: {0}", to_string(properties.deviceType));

            // Query Vulkan for device features.
            // Note: vkGetPhysicalDeviceFeatures2 is only available on devices with API version 1.1 or
            // above - calling it on 'older' devices causes an error.
            VkPhysicalDeviceFeatures2 features{};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            vkGetPhysicalDeviceFeatures2(device, &features);

            // Print device features - see VkPhysicalDeviceFeatures for full list of features.
            LV_CORE_INFO("  - Anisotropic filtering: {0}",
                features.features.samplerAnisotropy ? "true" : "false");

            // Query Vulkan for device supported queue families.
            std::uint32_t numQueues = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueues, nullptr);
            std::vector< VkQueueFamilyProperties > families(numQueues);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueues, families.data());

            // Print device supported queue families.
            for (auto const& family : families)
            {
                LV_CORE_INFO("  - Queue family: {0} ({1} queues)",
                    queue_flags(family.queueFlags), family.queueCount
                );
            }

            // Query Vulkan for device memory properties.
            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(device, &memory);

            // Print memory properties.
            LV_CORE_INFO("  - {0} heaps", memory.memoryHeapCount);
            for (std::uint32_t i = 0; i < memory.memoryHeapCount; ++i)
            {
                LV_CORE_INFO("    - heap {0}: {1} MBytes, {2}",
                    i, memory.memoryHeaps[i].size / 1024 / 1024,
                    memory_heap_flags(memory.memoryHeaps[i].flags)
                );
            }

            LV_CORE_INFO("  - {0} memory types", memory.memoryTypeCount);
            for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i)
            {
                LV_CORE_INFO("    - type {0}: from heap {1}, {2}",
                    i, memory.memoryTypes[i].heapIndex,
                    memory_property_flags(memory.memoryTypes[i].propertyFlags)
                );
            }
        }
    }


    VkPhysicalDevice select_device(VkInstance instance, VkSurfaceKHR surface)
    {
        LV_CORE_ASSERT(VK_NULL_HANDLE != instance, "Vulkan instance is null!");

        std::uint32_t numDevices = 0;
        {
            auto const res = vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get physical device count: vkEnumeratePhysicalDevices() returned {0}", res);
        }
        std::vector< VkPhysicalDevice > devices(numDevices, VK_NULL_HANDLE);
        {
            auto const res = vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get physical device list: vkEnumeratePhysicalDevices() returned {0}", res);
        }
        float bestScore = -1.f;
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        for (auto const device : devices)
        {
            auto const score = score_device(device, surface);
            if (score > bestScore)
            {
                bestScore = score;
                bestDevice = device;
            }
        }
        return bestDevice;
    }


    float score_device(VkPhysicalDevice physicalDev, VkSurfaceKHR surface)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDev, &props);

        auto const versMajor = VK_API_VERSION_MAJOR(props.apiVersion);
        auto const versMinor = VK_API_VERSION_MINOR(props.apiVersion);

        // Only consider devices with Vulkan 1.1 or later.
        if (versMajor < 1 || (versMajor == 1 && versMinor < 1))
        {
            LV_CORE_INFO("Discarding device {0}: Vulkan version too old", props.deviceName);
            return -1.f;
        }
        // Check that the device supports the VK_KHR_swapchain extension.
        auto const exts = get_device_extensions(physicalDev);
        if (!exts.count(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            LV_CORE_INFO("Discarding device {0}: extension {1} missing", props.deviceName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            return -1.f;
        }
        // Ensure there is a queue family that can present to the given surface.
        if (!find_queue_family(physicalDev, 0, surface))
        {
            LV_CORE_INFO("Discarding device {0}: cannot present to surface", props.deviceName);
            return -1.f;
        }
        // Ensure there is also a queue family that supports graphics commands.
        if (!find_queue_family(physicalDev, VK_QUEUE_GRAPHICS_BIT))
        {
            LV_CORE_INFO("Info: Discarding device {0}: no graphics queue family", props.deviceName);
            return -1.f;
        }

        // Discrete GPU > Integrated GPU > others
        float score = 0.f;
        if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == props.deviceType)
        {
            score += 500.f;
        }
        else if (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == props.deviceType)
        {
            score += 100.f;
        }
        return score;
    }


    std::optional<std::uint32_t> find_queue_family(VkPhysicalDevice physicalDev, VkQueueFlags queueFlags, VkSurfaceKHR surface)
    {
        LV_CORE_ASSERT(VK_NULL_HANDLE != physicalDev, "Physical device is null!");

        std::uint32_t numQueues = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDev, &numQueues, nullptr);
        std::vector<VkQueueFamilyProperties> families(numQueues);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDev, &numQueues, families.data());

        for (std::uint32_t i = 0; i < numQueues; ++i)
        {
            auto const& family = families[i];
            if (queueFlags == (queueFlags & family.queueFlags))
            {
                if (VK_NULL_HANDLE == surface)
                {
                    return i;
                }
                VkBool32 supported = VK_FALSE;
                auto const res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, i, surface, &supported);
                if (VK_SUCCESS == res && supported)
                {
                    return i;
                }
            }
        }
        return {};
    }


    VkDevice create_device(VkPhysicalDevice physicalDev, std::vector<std::uint32_t> const& queueFamilies, std::vector<char const*> const& enabledDeviceExtensions)
    {
        LV_CORE_ASSERT(VK_NULL_HANDLE != physicalDev, "Physical device is null!");
        LV_CORE_ASSERT(!queueFamilies.empty(), "No queues requested!");

        float queuePriorities[1] = { 1.f };
        std::vector<VkDeviceQueueCreateInfo> queueInfos(queueFamilies.size());
        for (std::size_t i = 0; i < queueFamilies.size(); ++i)
        {
            auto& queueInfo = queueInfos[i];
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilies[i];
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = queuePriorities;
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        VkPhysicalDeviceFeatures supportedFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDev, &supportedFeatures);
        deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
        deviceFeatures.sampleRateShading = supportedFeatures.sampleRateShading;

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = (std::uint32_t)queueInfos.size();
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.enabledExtensionCount = (std::uint32_t)enabledDeviceExtensions.size();
        deviceInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
        deviceInfo.pEnabledFeatures = &deviceFeatures;

        VkDevice device = VK_NULL_HANDLE;
        auto const res = vkCreateDevice(physicalDev, &deviceInfo, nullptr, &device);
        LV_CORE_ASSERT(VK_SUCCESS == res, "Failed to create logical device: vkCreateDevice() returned {0}", res);
        return device;
    }


    std::unordered_set<std::string> get_device_extensions(VkPhysicalDevice physicalDev)
    {
        std::uint32_t extensionCount = 0;
        {
            auto const res = vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &extensionCount, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get device extension count: "
                "vkEnumerateDeviceExtensionProperties() returned {0}", res
            );
        }

        std::vector<VkExtensionProperties> extensions(extensionCount);
        {
            auto const res = vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &extensionCount, extensions.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get device extensions: "
                "vkEnumerateDeviceExtensionProperties() returned {0}", res
            );
        }

        std::unordered_set<std::string> ret;
        for (auto const& ext : extensions)
        {
            ret.emplace(ext.extensionName);
        }
        return ret;
    }


    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> create_swapchain(VkPhysicalDevice physicalDev, VkSurfaceKHR surface, VkDevice device, GLFWwindow* windowHandle, std::vector<std::uint32_t> const& queueFamilyIndices, VkSwapchainKHR oldSwapchain)
    {
        auto const formats = get_surface_formats(physicalDev, surface);
        auto const modes = get_present_modes(physicalDev, surface);
        LV_CORE_ASSERT(!formats.empty(), "Could not get surface formats!");

        // Pick the surface format.
        // If there is an 8-bit RGB(A) SRGB format available, pick that. There are
        // two main variations possible here: RGBA and BGRA. If neither is
        // available, pick the first one that the driver gives us.
        //
        // See http://vulkan.gpuinfo.org/listsurfaceformats.php for a list of
        // formats and statistics about where they're supported.
        VkSurfaceFormatKHR format{};
        for (auto const fmt : formats)
        {
            if (VK_FORMAT_R8G8B8A8_SRGB == fmt.format &&
                VK_COLORSPACE_SRGB_NONLINEAR_KHR == fmt.colorSpace)
            {
                format = fmt;
                break;
            }

            if (VK_FORMAT_B8G8R8A8_SRGB == fmt.format &&
                VK_COLORSPACE_SRGB_NONLINEAR_KHR == fmt.colorSpace)
            {
                format = fmt;
                break;
            }
        }

        // Pick a presentation mode.
        // Use FIFO_RELAXED if available, otherwise fall back to plain FIFO (which is guaranteed
        // to be available).
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        if (modes.count(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
            presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;

        // Pick an image count.
        // We follow the Vulkan tutorial's recommendation of using one greater than the device's
        // minimum capability, i.e., " minImageCount+1 ".
        VkSurfaceCapabilitiesKHR caps;
        auto const res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDev, surface, &caps);
        LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get surface capabilities: vkGetPhysicalDeviceSurfaceCapabilitiesKHR() returned {0}", res);
        std::uint_fast32_t imageCount = 2;
        if (imageCount < caps.minImageCount + 1)
        {
            imageCount = caps.minImageCount + 1;
        }
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        {
            imageCount = caps.maxImageCount;
        }
        // Figure out swap extent.
        // If the Vulkan driver does not have this information, it sets the extent to the maximum
        // limit of the uint32_t type, in which case we query GLFW instead.
        VkExtent2D extent = caps.currentExtent;
        if (0xFFFFFFFFF == extent.width)
        {
            int width, height;
            glfwGetFramebufferSize(windowHandle, &width, &height);

            // Note: we must ensure that the extent is within the range defined by
            // [ minImageExtent, maxImageExtent ].
            auto const& min = caps.minImageExtent;
            auto const& max = caps.maxImageExtent;

            extent.width = std::clamp(std::uint32_t(width), min.width, max.width);
            extent.height = std::clamp(std::uint32_t(height), min.height, max.height);
        }

        // Finally, create the swap chain.
        VkSwapchainCreateInfoKHR chainInfo{};
        chainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        chainInfo.surface = surface;
        chainInfo.minImageCount = imageCount;
        chainInfo.imageFormat = format.format;
        chainInfo.imageColorSpace = format.colorSpace;
        chainInfo.imageExtent = extent;
        chainInfo.imageArrayLayers = 1;
        chainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        chainInfo.preTransform = caps.currentTransform;
        chainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        chainInfo.presentMode = presentMode;
        chainInfo.clipped = VK_TRUE;
        chainInfo.oldSwapchain = oldSwapchain;

        if (queueFamilyIndices.size() <= 1)
        {
            chainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else
        {
            // Multiple queues may access this resource. There are two
            // alternative here. SHARING_MODE_CONCURRENT allows access from
            // multiple queues without transferring ownership. EXCLUSIVE would
            // require explicit ownership transfers, which we're avoiding for
            // now. EXCLUSIVE may result in better performance than CONCURRENT.
            chainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            chainInfo.queueFamilyIndexCount = std::uint32_t(queueFamilyIndices.size());
            chainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }
        VkSwapchainKHR chain = VK_NULL_HANDLE;
        {
            auto const res = vkCreateSwapchainKHR(device, &chainInfo, nullptr, &chain);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to create swap chain: vkCreateSwapchainKHR() returned {0}", res);
        }
        return { chain, format.format, extent };
    }


    std::vector<VkSurfaceFormatKHR> get_surface_formats(VkPhysicalDevice physicalDev, VkSurfaceKHR surface)
    {
        uint32_t numFormats;
        {
            auto const res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, surface, &numFormats, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Could not get surface format: vkGetPhysicalDeviceSurfaceFormatsKHR() returned {0}", res);
        }
        std::vector< VkSurfaceFormatKHR > formats(numFormats);
        {
            auto const res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, surface, &numFormats, formats.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get surface formats: vkGetPhysicalDeviceSurfaceFormatsKHR() returned {0}", res);
        }
        return formats;
    }


    std::unordered_set<VkPresentModeKHR> get_present_modes(VkPhysicalDevice physicalDev, VkSurfaceKHR surface)
    {
        uint32_t numModes;
        {
            auto const res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, surface, &numModes, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get present mode count: vkGetPhysicalDeviceSurfacePresentModesKHR() returned {0}", res);
        }
        std::vector<VkPresentModeKHR> presentModes(numModes);
        {
            auto const res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, surface, &numModes, presentModes.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get present modes: vkGetPhysicalDeviceSurfacePresentModesKHR() returned {0}", res);
        }
        std::unordered_set<VkPresentModeKHR> results;
        for (auto const& presentMode : presentModes)
        {
            results.insert(presentMode);
        }
        return results;
    }


    void get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& images)
    {
        LV_CORE_ASSERT(0 == images.size(), "Image vector already populated!");
        std::uint32_t numImages = 0;
        {
            auto const res = vkGetSwapchainImagesKHR(device, swapchain, &numImages, nullptr);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get swapchain image count: vkGetSwapchainImagesKHR() returned {0}", res);
        }
        images.resize(numImages);
        {
            auto const res = vkGetSwapchainImagesKHR(device, swapchain, &numImages, images.data());
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to get swapchain images: vkGetSwapchainImagesKHR() returned {0}", res);
        }
    }


    void create_swapchain_image_views(VkDevice device, VkFormat swapchainFormat, std::vector<VkImage> const& images, std::vector<VkImageView>& views)
    {
        LV_CORE_ASSERT(0 == views.size(), "View vector already populated!");
        for (std::size_t i = 0; i < images.size(); ++i)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainFormat;
            viewInfo.components = VkComponentMapping{
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            };
            viewInfo.subresourceRange = VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 1
            };

            VkImageView view = VK_NULL_HANDLE;
            auto const res = vkCreateImageView(device, &viewInfo, nullptr, &view);
            LV_CORE_ASSERT(VK_SUCCESS == res, "Unable to create image view for swap chain image {0}: vkCreateImageView() returned {1}", i, res);

            views.emplace_back(view);
        }
        LV_CORE_ASSERT(views.size() == images.size(), "Different number of images ({0}) and image views ({1})!", images.size(), views.size());
    }


    VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts =
            physicalDeviceProperties.limits.framebufferColorSampleCounts &
            physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

}
