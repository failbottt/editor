#version 450 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 snakeColor;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in float a_TexIndex;

uniform mat4 projection;

out vec4 vs_color;
out vec2 v_TexCoord;
out float v_TexIndex;

void main(void)
{
  gl_Position = projection * position;
  vs_color = snakeColor; 
  v_TexIndex = a_TexIndex;
  v_TexCoord = a_TexCoord;
}
