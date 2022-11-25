#type vertex
#version 450

layout (std140) uniform CameraUniform
{
    mat4 ViewProj;
    vec4 Position;
    vec4 AimDirection;
} u_Camera;

uniform mat4 u_Transform;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = u_Camera.ViewProj * u_Transform * vec4(a_Position, 1.0);
}


#type fragment
#version 450

uniform sampler2D u_Texture;

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

void main()
{
    o_Color = texture(u_Texture, v_TexCoord);
}
