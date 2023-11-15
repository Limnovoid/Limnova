#pragma once

extern "C" {
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoType MonoType;
}


namespace Limnova
{

    class Entity;

    class ScriptLibrary
    {
    public:
        static void RegisterComponentTypes(MonoImage* coreAssemblyImage);
        static void RegisterInternalCalls();

        static std::unordered_map<MonoType*, std::function<bool(Entity)>>& GetEntityHasComponentFuncs();
    private:
        typedef std::unordered_map<MonoType*, std::function<bool(Entity)>> EntityHasComponentFuncMap;
        static EntityHasComponentFuncMap s_EntityHasComponentFuncs;
    };

}
