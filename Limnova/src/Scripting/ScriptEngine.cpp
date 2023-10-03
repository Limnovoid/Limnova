#include "ScriptEngine.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"


namespace Limnova
{

    void ScriptEngine::Init()
    {
        InitMono();
    }

    void ScriptEngine::Shutdown()
    {
    }

    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path(LV_DIR"/thirdparty/mono/lib");
    }

}
