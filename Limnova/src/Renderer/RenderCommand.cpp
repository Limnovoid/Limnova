#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"


namespace Limnova
{

    RendererAPI* RenderCommand::s_RendererApi = new OpenGLRendererAPI;

}
