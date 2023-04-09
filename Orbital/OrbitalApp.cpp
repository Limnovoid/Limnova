#include <Limnova.h>
#include <Core/EntryPoint.h>

#include "Orbital2D.h"
#include "OrbitalLayer.h"

#include <chrono> // TEMPORARY delta time

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\Orbital\\assets"


namespace Limnova
{

    class LIMNOVA_API OrbitalApp : public Application
    {
    public:
        OrbitalApp()
        {
            //PushLayer(new Orbital2D());
            PushLayer(new OrbitalLayer());
        }

        ~OrbitalApp()
        {
        }
    };


    LV::Application* CreateApplication(ApplicationCommandLineArgs /*args*/)
    {
        return new OrbitalApp();
    }

}
