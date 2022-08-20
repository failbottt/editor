#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "glextloader.h"
#include "file.h"
#include "gl_compile_errors.h"

#include <ft2build.h>
#include FT_FREETYPE_H

// settings
#define LINE_HEIGHT 20
#define WIDTH   1024 
#define HEIGHT  768
float cursor_h;
float cursor_w;

void render_cursor();
void render_text(const char* text, float x, float y, float scale);
GLuint compile_shaders(void);

GLuint vertex_shader;
GLuint frag_shader;
GLuint font_vertex_shader;
GLuint font_frag_shader;
GLuint cursor_program;
GLuint font_program;

const char *cursor_vert_shader = "shaders/cursor.vert";
const char *texture_frag_shader = "shaders/cursor.frag";
const char *font_vert_shader_path = "shaders/font.vert";
const char *font_frag_shader_path = "shaders/font.frag";

unsigned int VAO, VBO;
unsigned int VBO2, VAO2;

typedef struct {
	float x;
	float y;
} VEC2;

/// Holds all state information relevant to a character as loaded using FreeType
typedef struct {
	int16_t TextureID; // ID handle of the glyph texture
	VEC2 Size;      // Size of glyph
	VEC2 Bearing;   // Offset from baseline to left/top of glyph
	int16_t Advance;   // Horizontal offset to advance to next glyph
} Character;

Character Characters[128];

int main() {
    LoadGLExtensions(); 
    Display *display = XOpenDisplay(NULL);

    XVisualInfo *visual = glXChooseVisual(display, 0, (int[]){
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    });
    if (!visual) {
        fprintf(stderr, "Unable to choose visual\n");
        exit(1);
    }
    printf("Visual ID: %ld\n", visual->visualid);

    GLXContext gl_context = glXCreateContext(display, visual, 0, True);
    if (!gl_context) {
        fprintf(stderr, "Unable to create GL context\n");
        exit(1);
    }

    Window window = XCreateSimpleWindow(
        display,
        XDefaultRootWindow(display),    // parent
        0, 0,                           // x, y
        WIDTH, HEIGHT,                  // width, height
        0,                              // border width
        0x00000000,                     // border color
        0x00000000                      // background color
    );

    glXMakeCurrent(display, window, gl_context);

    XStoreName(display, window, "opengl");

    XSelectInput(display, window, KeyPressMask|KeyReleaseMask);

    XMapWindow(display, window);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const char *vert_shader_source = read_file(cursor_vert_shader);
    const char *frag_shader_source = read_file(texture_frag_shader);

    const char *font_vert_shader_source = read_file(font_vert_shader_path);
    const char *font_frag_shader_source = read_file(font_frag_shader_path);

    // compiles vert shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vert_shader_source, NULL);
    glCompileShader(vertex_shader);
    checkCompileErrors(vertex_shader, "VERTEX");

    font_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(font_vertex_shader, 1, &font_vert_shader_source, NULL);
    glCompileShader(font_vertex_shader);
    checkCompileErrors(font_vertex_shader, "FONT VERTEX");

    // compiles frag shader
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
    glCompileShader(frag_shader);
    checkCompileErrors(frag_shader, "FRAG");

    font_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(font_frag_shader, 1, &font_frag_shader_source, NULL);
    glCompileShader(font_frag_shader);
    checkCompileErrors(font_frag_shader, "FONTFRAG");

    // create program
    cursor_program = glCreateProgram();
    glAttachShader(cursor_program, vertex_shader);
    glAttachShader(cursor_program, frag_shader);
    glLinkProgram(cursor_program);

    font_program = glCreateProgram();
    glAttachShader(font_program, font_vertex_shader);
    glAttachShader(font_program, font_frag_shader);
    glLinkProgram(font_program);

    // delete shaders because they are part of the program now
    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);
    glDeleteShader(font_vertex_shader);
    glDeleteShader(font_frag_shader);
	// TODO: free files read into memory

  // 2D set coordinate space match height and width of screen rather than 0.0-1.0
  // ORTHO
    float pj[4][4] = {{0.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 0.0f},  
      {0.0f, 0.0f, 0.0f, 0.0f},                    \
      {0.0f, 0.0f, 0.0f, 0.0f}};

    float rl, tb, fn;
    float left = 0.0f;
    float right = (float)WIDTH;
    float bottom = 0.0f;
    float top = (float)HEIGHT;
    float nearZ = -1.0f;
    float farZ = 1.0f;

    rl = 1.0f / (right  - left);
    tb = 1.0f / (top    - bottom);
    fn =-1.0f / (farZ - nearZ);

    pj[0][0] = 2.0f * rl;
    pj[1][1] = 2.0f * tb;
    pj[2][2] = 2.0f * fn;
    pj[3][0] =-(right  + left)    * rl;
    pj[3][1] =-(top    + bottom)  * tb;
    pj[3][2] = (farZ + nearZ) * fn;
    pj[3][3] = 1.0f;
    // ORTHO END
    
    glUseProgram(cursor_program);
	glUniform4f(glGetUniformLocation(cursor_program, "in_color"), 1.0f, 1.0f, 1.0, 0.59f);
    glUniformMatrix4fv(glGetUniformLocation(cursor_program, "projection"), 1, GL_FALSE, (float *)pj);

	glUseProgram(font_program);
	glUniformMatrix4fv(glGetUniformLocation(font_program, "projection"), 1, GL_FALSE, (float *)pj);

	// FREETYPE
	// ----------------------
	//
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
	{
		printf("ERROR::FREETYPE: Could not init FreeType Library\n");
		return -1;
	}

	// load font as face
	FT_Face face;
	if (FT_New_Face(ft, "Hack-Regular.ttf", 0, &face)) {
		printf("ERROR::FREETYPE: Failed to load font at ./external/fonts/Hack-Regular.ttf\n");
		return -1;
	}

	FT_Set_Pixel_Sizes(face, 16, 16);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// make textures for ASCII characters 0-128
	for (uint8_t c = 0; c < 128; c++)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			printf("ERROR::FREETYTPE: Failed to load Glyph\n");
			continue;
		}

		unsigned int texture;
		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
				);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		VEC2 size = {face->glyph->bitmap.width, face->glyph->bitmap.rows};
		VEC2 bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
		Character character = {
			texture,
			size,
			bearing,
			(unsigned int)(face->glyph->advance.x)
		};

		Characters[c] = character;
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// set cursor dimensions
	Character ch = Characters[65];
	cursor_w = ch.Size.x * 1.0;
	cursor_h = ch.Size.y * 1.0 + 3;

	// configure VAO/VBO for texture quads
	// -----------------------------------
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	char *code = "Good news, everyone!";

    unsigned int indices[] = {  // note that we start from 0!
		// snake head
        0, 1, 3,  // first Triangle
        1, 2, 3,   // second Triangle
    };

	// object 1
	glGenVertexArrays(1, &VAO2);
	glGenBuffers(1, &VBO2);

    bool quit = false;
    while (!quit) {
        while (XPending(display) > 0) {
            XEvent event = {0};
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                KeySym key = XLookupKeysym(&event.xkey, 0);

                switch(key) {
                  case XK_Escape:
                    quit = 1;
                    break;
                }
            }
        }

        const GLfloat bgColor[] = {0.0f,0.0f,0.5f,1.0f};
        glClearBufferfv(GL_COLOR, 0, bgColor);

		render_text(
				code, 
				10.0f,
				(float)(HEIGHT) - LINE_HEIGHT,
				1.0f
				);

		render_cursor();

        glXSwapBuffers(display, window);
    }

    /* glDeleteVertexArrays(1, &vertex_array_object); */
    /* glDeleteProgram(program); */
    XCloseDisplay(display);
}

void render_text(const char *text, float x, float y, float scale)
{
	glUseProgram(font_program);

	// font color
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;

	glUniform3f(glGetUniformLocation(font_program, "textColor"), r, g, b);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);

	float xpos = 0.0f;
	float ypos = 0.0f;

	for (unsigned int c = 0; c != strlen(text); c++) 
	{
		Character ch = Characters[(int)text[c]];
		xpos = x + ch.Bearing.x * scale;
		ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },            
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }           
		};

		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		x += (ch.Advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void render_cursor() {
	glUseProgram(cursor_program);
	
	/* float x = (ed.cursor_x * left_pad) + left_pad; */
	float x = (0 * cursor_w) + 10; // 10 is the left padding

	/* printf("X POS: %f\n", x); */
	// 22 = cursor line position movement down and up
	
	float y = ((float)HEIGHT - LINE_HEIGHT) + (cursor_h - 2.0f);

	float cursor_pos[] = {
		x, y, 0.0f,
		x, y - cursor_h, 0.0f,
		x + cursor_w,  y - cursor_h, 0.0f,

		x + cursor_w,  y, 0.0f,
		x, y, 0.0f,
		0.0f,  0.0f, 0.0f,
	}; 

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cursor_pos), cursor_pos, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0); 
	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
}
