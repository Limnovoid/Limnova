#include <Limnova.h>
#include <Core/EntryPoint.h>

#include "Play2DLayer.h"
#include "TestLayer.h"

#include <chrono> // TEMPORARY delta time

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\PlayApp\\assets"


namespace Limnova
{

    class LIMNOVA_API PlayApp : public Application
    {
    public:
        PlayApp()
        {
            //PushLayer(new TestLayer());
            //
            PushLayer(new Play2DLayer());
        }

        ~PlayApp()
        {
        }
    };


    Application* CreateApplication()
    {
        return new PlayApp();
    }

}
