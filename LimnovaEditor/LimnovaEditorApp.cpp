#include <Limnova.h>
#include <Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Limnova
{

    class LimnovaEditorApp : public Application
    {
    public:
        LimnovaEditorApp()
            : Application("Limnova Editor")
        {
            PushLayer(new EditorLayer());
        }

        ~LimnovaEditorApp()
        {
        }
    };


    Application* CreateApplication()
    {
        return new LimnovaEditorApp();
    }

}
