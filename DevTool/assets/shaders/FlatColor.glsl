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

out vec3 v_Position;

void main()
{
    v_Position = a_Position;
    gl_Position = u_Camera.ViewProj * u_Transform * vec4(a_Position, 1.0);
}


#type fragment
#version 450

uniform vec3 u_Color;

layout(location = 0) out vec4 o_Color;

in vec3 v_Position;

void main()
{
    o_Color = vec4(u_Color, 1.0);
}
