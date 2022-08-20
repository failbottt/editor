void checkCompileErrors(GLuint shader, char *type)
{
  GLint success;
  GLchar infoLog[1024];
  if(strcmp(type, "PROGRAM") != 0)
  {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      printf("ERROR::SHADER_COMPILATION_ERROR of type %s: error: %s\n", type, infoLog);
      printf("----------------------------------------\n");
    }
  }
  else
  {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if(!success)
    {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      printf("ERROR::PROGRAM_LINKING of type %s: error: %s\n", type, infoLog);
      printf("----------------------------------------\n");
    }
  }
}

