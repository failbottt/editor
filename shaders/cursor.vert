#version 460 core 

layout (location = 0) in vec3 aPos;

out vec4 color;

uniform mat4 projection;
uniform vec4 in_color;

void main(void)
{
    gl_Position = projection * vec4(aPos.x, aPos.y, aPos.z, 1.0);

    color = in_color;
}
