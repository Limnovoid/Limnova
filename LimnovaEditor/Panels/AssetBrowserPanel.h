#pragma once

#include <Limnova.h>
#include <filesystem>


namespace Limnova
{

    class AssetBrowserPanel
    {
    public:
        AssetBrowserPanel();

        void OnImGuiRender();
    private:
        std::filesystem::path m_CurrentDirectoryPath;
    };

}
