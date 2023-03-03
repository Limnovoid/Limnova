#include <Limnova.h>
#include <Core/EntryPoint.h>

#include "Orbital2D.h"

#include <chrono> // TEMPORARY delta time

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\Orbital\\assets"


namespace LV = Limnova;

class LIMNOVA_API OrbitalApp : public Limnova::Application
{
public:
    OrbitalApp()
    {
        PushLayer(new Orbital2D());
    }

    ~OrbitalApp()
    {
    }
};


Limnova::Application* Limnova::CreateApplication()
{
    return new OrbitalApp();
}
