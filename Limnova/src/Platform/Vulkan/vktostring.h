#pragma once

#include <volk.h>


namespace LimnovaVk
{
    std::ostream& operator<<(std::ostream& os, const VkResult&);
    std::string to_string(VkPhysicalDeviceType);
    std::string to_string(VkDebugUtilsMessageSeverityFlagBitsEXT);

    std::string queue_flags(VkQueueFlags);
    std::string message_type_flags(VkDebugUtilsMessageTypeFlagsEXT);
    std::string memory_heap_flags(VkMemoryHeapFlags);
    std::string memory_property_flags(VkMemoryPropertyFlags);

    std::string driver_version(std::uint32_t aVendorId, std::uint32_t aDriverVersion);
}
