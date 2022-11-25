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
layout(location = 1) in vec4 a_Color;

out vec3 v_Position;
out vec4 v_Color;

void main()
{
    v_Position = a_Position;
    v_Color = a_Color;
    gl_Position = u_Camera.ViewProj * u_Transform * vec4(a_Position, 1.0);
}


#type fragment
#version 450

layout(location = 0) out vec4 o_Color;

in vec3 v_Position;
in vec4 v_Color;

void main()
{
    o_Color = vec4(v_Position * 0.5 + 0.5, 1.0);
    o_Color = v_Color;
}
