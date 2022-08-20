#version 450 core

in vec4 vs_color;
in vec2 v_TexCoord;
in float v_TexIndex;

out vec4 color;

uniform sampler2D u_Textures[1];

void main(void)
{
  int index = int(v_TexIndex);
  color = texture(u_Textures[index], v_TexCoord);
}
