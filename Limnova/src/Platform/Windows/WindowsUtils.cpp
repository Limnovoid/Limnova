#include <Utils/PlatformUtils.h>

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Core/Application.h>


namespace Limnova
{

    OPENFILENAMEA* GetOpenFileNameAInfo(const char* filter)
    {
        static OPENFILENAMEA ofn;
        static CHAR szFile[256] = { 0 };

        ZeroMemory(&ofn, sizeof(OPENFILENAME));

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        return &ofn;
    }


    std::string FileDialogs::OpenFile(const char* filter)
    {
        auto* pOfn = GetOpenFileNameAInfo(filter);
        if (GetOpenFileNameA(pOfn) == TRUE) {
            return pOfn->lpstrFile;
        }
        return { "" };
    }


    std::string FileDialogs::SaveFile(const char* filter)
    {
        auto* pOfn = GetOpenFileNameAInfo(filter);
        if (GetSaveFileNameA(pOfn) == TRUE) {
            return pOfn->lpstrFile;
        }
        return { "" };
    }

}
