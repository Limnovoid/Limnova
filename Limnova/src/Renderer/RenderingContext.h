#pragma once


namespace Limnova
{

    class RenderingContext
    {
    public:
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void SwapBuffers() = 0;
    };

}