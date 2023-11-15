#include "AssetBrowserPanel.h"


namespace Limnova
{

    extern const std::filesystem::path s_AssetDirectoryPath = "C:\\Programming\\source\\Limnova\\LimnovaEditor\\Assets";


    AssetBrowserPanel::AssetBrowserPanel()
        : m_CurrentDirectoryPath(s_AssetDirectoryPath)
    {
    }


    void AssetBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Asset Browser");

        if (m_CurrentDirectoryPath != s_AssetDirectoryPath)
        {
            if (ImGui::Button("<")) {
                m_CurrentDirectoryPath = m_CurrentDirectoryPath.parent_path();
            }
            ImGui::SameLine();
            std::string directoryString = m_CurrentDirectoryPath.filename().string();
            ImGui::Text("%s", directoryString.c_str());
        }

        if (ImGui::Button("+")) {
            ImGui::OpenPopup("CreateFolderPopup");
        }
        if (ImGui::BeginPopup("CreateFolderPopup")) {
            char newDirectoryName[64] = "New folder";
            if (ImGui::InputText("New folder name:", newDirectoryName, 64, ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::filesystem::path newDirectoryRelativePath = std::string(newDirectoryName);
                std::filesystem::create_directory(m_CurrentDirectoryPath / newDirectoryRelativePath);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::PushID("CurrentDirectoryBrowser");
        for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectoryPath))
        {
            auto const& path = directoryEntry.path();
            auto assetRelativePath = std::filesystem::relative(path, s_AssetDirectoryPath);
            std::string filenameString = path.filename().string();

            bool isDirectory = directoryEntry.is_directory();

            if (!isDirectory) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); }
            bool entryOpened = ImGui::Button(filenameString.c_str());
            if (!isDirectory) { ImGui::PopStyleColor(); }

            if (ImGui::BeginDragDropSource())
            {
                const wchar_t* itemPath = assetRelativePath.c_str();
                ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
                ImGui::EndDragDropSource();
            }

            if (entryOpened) {
                if (isDirectory) {
                    m_CurrentDirectoryPath = path;
                    break;
                }
                else {
                    // TODO : use file
                }
            }

            if (ImGui::BeginPopupContextItem(filenameString.c_str())) {
                ImGui::Text("TODO : modal 'Are you sure?' or undo button");
                /*if (isDirectory) {
                    if (ImGui::MenuItem("Delete folder")) {
                        std::filesystem::remove(path);
                    }
                }
                else {
                    if (ImGui::MenuItem("Delete file")) {
                        std::filesystem::remove(path);
                    }
                }*/
                ImGui::EndPopup();
            }
        }
        ImGui::PopID();

        ImGui::End();
    }

}
