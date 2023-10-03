#pragma once


namespace Limnova
{

    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
    private:
        static void InitMono();
    };

}
