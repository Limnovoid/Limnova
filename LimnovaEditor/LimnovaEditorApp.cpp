#include <Limnova.h>
#include <Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Limnova
{

    class LimnovaEditorApp : public Application
    {
    public:
        LimnovaEditorApp(ApplicationCommandLineArgs args)
            : Application("Limnova Editor", args)
        {
            PushLayer(new EditorLayer());
        }

        ~LimnovaEditorApp()
        {
        }
    };


    Application* CreateApplication(ApplicationCommandLineArgs args)
    {
        return new LimnovaEditorApp(args);
    }

}
