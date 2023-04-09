#pragma once

#include "Renderer/RenderingContext.h"

#include <Volk/volk.h>
#include <GLFW/glfw3.h>


namespace Limnova
{

	class VulkanContext : public RenderingContext
    {
    public:
        VulkanContext(GLFWwindow* windowHandle);

        void Init() override;
        void Shutdown() override;
        void SwapBuffers() override;
    private:
        GLFWwindow* m_WindowHandle;
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        std::uint32_t m_GraphicsFamilyIndex = 0;
        std::uint32_t m_PresentFamilyIndex = 0;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_SwapchainFormat;
        VkExtent2D m_SwapchainExtent;
        std::vector<VkImage> m_SwapImages;
        std::vector<VkImageView> m_SwapViews;
        VkSampleCountFlagBits m_MsaaSamples;
    };

}
