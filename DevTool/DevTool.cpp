#include <Limnova.h>

#include <imgui/imgui.h>


class LIMNOVA_API DevLayer : public Limnova::Layer
{
public:
    DevLayer()
        : Layer("DevLayer")
    {
    }
    DevLayer(ImGuiContext* p_ImGuiCtx, ImGuiMemAllocFunc p_Alloc, ImGuiMemFreeFunc p_Free, void* p_Data)
        : Layer("DevLayer")
    {
        ImGui::SetCurrentContext(p_ImGuiCtx);
        ImGui::SetAllocatorFunctions(p_Alloc, p_Free, p_Data);
    }

    void OnUpdate() override
    {
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Test");
        ImGui::Text("Hello from DevTool!");
        ImGui::End();
    }

    void OnEvent(Limnova::Event& event) override
    {
    }
};


class LIMNOVA_API DevApp : public Limnova::Application
{
public:
    DevApp()
    {
        //void* p_ImGuiDllCtx = GetImGuiContext();
        //ImGuiMemAllocFunc alloc;
        //ImGuiMemFreeFunc free;
        //void* p_Data;
        //GetAllocatorFunctions((void*)&alloc, (void*)&free, &p_Data);
        //PushLayer(new DevLayer((ImGuiContext*)p_ImGuiDllCtx, alloc, free, p_Data));
        PushLayer(new DevLayer());
    }

    ~DevApp()
    {
    }
};


Limnova::Application* Limnova::CreateApplication()
{
    return new DevApp();
}
