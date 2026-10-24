/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
 

#include "SDL.h"

#ifdef HAVE_OPENGL

#include "SDL_opengl.h"

static SDL_bool shaders_supported;
static int current_shader = 0;

enum
{
    SHADER_COLOR,
    SHADER_TEXTURE,
    SHADER_TEXCOORDS,
    NUM_SHADERS
};

typedef struct
{
    GLhandleARB program;
    GLhandleARB vert_shader;
    GLhandleARB frag_shader;
    const char *vert_source;
    const char *frag_source;
} ShaderData;

static ShaderData shaders[NUM_SHADERS] = {

     
    { 0, 0, 0,
       
      "varying vec4 v_color;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "    v_color = gl_Color;\n"
      "}",
       
      "varying vec4 v_color;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_FragColor = v_color;\n"
      "}" },

     
    { 0, 0, 0,
       
      "varying vec4 v_color;\n"
      "varying vec2 v_texCoord;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "    v_color = gl_Color;\n"
      "    v_texCoord = vec2(gl_MultiTexCoord0);\n"
      "}",
       
      "varying vec4 v_color;\n"
      "varying vec2 v_texCoord;\n"
      "uniform sampler2D tex0;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_FragColor = texture2D(tex0, v_texCoord) * v_color;\n"
      "}" },

     
    { 0, 0, 0,
       
      "varying vec2 v_texCoord;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "    v_texCoord = vec2(gl_MultiTexCoord0);\n"
      "}",
       
      "varying vec2 v_texCoord;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    vec4 color;\n"
      "    vec2 delta;\n"
      "    float dist;\n"
      "\n"
      "    delta = vec2(0.5, 0.5) - v_texCoord;\n"
      "    dist = dot(delta, delta);\n"
      "\n"
      "    color.r = v_texCoord.x;\n"
      "    color.g = v_texCoord.x * v_texCoord.y;\n"
      "    color.b = v_texCoord.y;\n"
      "    color.a = 1.0 - (dist * 4.0);\n"
      "    gl_FragColor = color;\n"
      "}" },
};

static PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
static PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
static PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
static PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
static PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
static PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
static PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
static PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
static PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
static PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
static PFNGLUNIFORM1IARBPROC glUniform1iARB;
static PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;

static SDL_bool CompileShader(GLhandleARB shader, const char *source)
{
    GLint status = 0;

    glShaderSourceARB(shader, 1, &source, NULL);
    glCompileShaderARB(shader);
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
    if (status == 0) {
        GLint length = 0;
        char *info;

        glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        info = (char *)SDL_malloc((size_t)length + 1);
        if (info == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Out of memory!");
        } else {
            glGetInfoLogARB(shader, length, NULL, info);
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile shader:\n%s\n%s", source, info);
            SDL_free(info);
        }
        return SDL_FALSE;
    } else {
        return SDL_TRUE;
    }
}

static SDL_bool LinkProgram(ShaderData *data)
{
    GLint status = 0;

    glAttachObjectARB(data->program, data->vert_shader);
    glAttachObjectARB(data->program, data->frag_shader);
    glLinkProgramARB(data->program);

    glGetObjectParameterivARB(data->program, GL_OBJECT_LINK_STATUS_ARB, &status);
    if (status == 0) {
        GLint length = 0;
        char *info;

        glGetObjectParameterivARB(data->program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        info = (char *)SDL_malloc((size_t)length + 1);
        if (info == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Out of memory!");
        } else {
            glGetInfoLogARB(data->program, length, NULL, info);
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to link program:\n%s", info);
            SDL_free(info);
        }
        return SDL_FALSE;
    } else {
        return SDL_TRUE;
    }
}

static SDL_bool CompileShaderProgram(ShaderData *data)
{
    const int num_tmus_bound = 4;
    int i;
    GLint location;

    glGetError();

     
    data->program = glCreateProgramObjectARB();

     
    data->vert_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    if (!CompileShader(data->vert_shader, data->vert_source)) {
        return SDL_FALSE;
    }

     
    data->frag_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    if (!CompileShader(data->frag_shader, data->frag_source)) {
        return SDL_FALSE;
    }

     
    if (!LinkProgram(data)) {
        return SDL_FALSE;
    }

     
    glUseProgramObjectARB(data->program);
    for (i = 0; i < num_tmus_bound; ++i) {
        char tex_name[5];
        (void)SDL_snprintf(tex_name, SDL_arraysize(tex_name), "tex%d", i);
        location = glGetUniformLocationARB(data->program, tex_name);
        if (location >= 0) {
            glUniform1iARB(location, i);
        }
    }
    glUseProgramObjectARB(0);

    return (glGetError() == GL_NO_ERROR) ? SDL_TRUE : SDL_FALSE;
}

static void DestroyShaderProgram(ShaderData *data)
{
    if (shaders_supported) {
        glDeleteObjectARB(data->vert_shader);
        glDeleteObjectARB(data->frag_shader);
        glDeleteObjectARB(data->program);
    }
}

static SDL_bool InitShaders()
{
    int i;

     
    shaders_supported = SDL_FALSE;
    if (SDL_GL_ExtensionSupported("GL_ARB_shader_objects") &&
        SDL_GL_ExtensionSupported("GL_ARB_shading_language_100") &&
        SDL_GL_ExtensionSupported("GL_ARB_vertex_shader") &&
        SDL_GL_ExtensionSupported("GL_ARB_fragment_shader")) {
        glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)SDL_GL_GetProcAddress("glAttachObjectARB");
        glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)SDL_GL_GetProcAddress("glCompileShaderARB");
        glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glCreateProgramObjectARB");
        glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)SDL_GL_GetProcAddress("glCreateShaderObjectARB");
        glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)SDL_GL_GetProcAddress("glDeleteObjectARB");
        glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)SDL_GL_GetProcAddress("glGetInfoLogARB");
        glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)SDL_GL_GetProcAddress("glGetObjectParameterivARB");
        glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)SDL_GL_GetProcAddress("glGetUniformLocationARB");
        glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)SDL_GL_GetProcAddress("glLinkProgramARB");
        glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)SDL_GL_GetProcAddress("glShaderSourceARB");
        glUniform1iARB = (PFNGLUNIFORM1IARBPROC)SDL_GL_GetProcAddress("glUniform1iARB");
        glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glUseProgramObjectARB");
        if (glAttachObjectARB &&
            glCompileShaderARB &&
            glCreateProgramObjectARB &&
            glCreateShaderObjectARB &&
            glDeleteObjectARB &&
            glGetInfoLogARB &&
            glGetObjectParameterivARB &&
            glGetUniformLocationARB &&
            glLinkProgramARB &&
            glShaderSourceARB &&
            glUniform1iARB &&
            glUseProgramObjectARB) {
            shaders_supported = SDL_TRUE;
        }
    }

    if (!shaders_supported) {
        return SDL_FALSE;
    }

     
    for (i = 0; i < NUM_SHADERS; ++i) {
        if (!CompileShaderProgram(&shaders[i])) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to compile shader!\n");
            return SDL_FALSE;
        }
    }

     
    return SDL_TRUE;
}

static void QuitShaders()
{
    int i;

    for (i = 0; i < NUM_SHADERS; ++i) {
        DestroyShaderProgram(&shaders[i]);
    }
}

 
static int
power_of_two(int input)
{
    int value = 1;

    while (value < input) {
        value <<= 1;
    }
    return value;
}

GLuint
SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord)
{
    GLuint texture;
    int w, h;
    SDL_Surface *image;
    SDL_Rect area;
    SDL_BlendMode saved_mode;

     
    w = power_of_two(surface->w);
    h = power_of_two(surface->h);
    texcoord[0] = 0.0f;                     
    texcoord[1] = 0.0f;                     
    texcoord[2] = (GLfloat)surface->w / w;  
    texcoord[3] = (GLfloat)surface->h / h;  

    image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN      
                                 0x000000FF,
                                 0x0000FF00, 0x00FF0000, 0xFF000000
#else
                                 0xFF000000,
                                 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
    );
    if (image == NULL) {
        return 0;
    }

     
    SDL_GetSurfaceBlendMode(surface, &saved_mode);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

     
    area.x = 0;
    area.y = 0;
    area.w = surface->w;
    area.h = surface->h;
    SDL_BlitSurface(surface, &area, image, &area);

     
    SDL_SetSurfaceBlendMode(surface, saved_mode);

     
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
    SDL_FreeSurface(image);  

    return texture;
}

 
void InitGL(int Width, int Height)  
{
    GLdouble aspect;

    glViewport(0, 0, Width, Height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  
    glClearDepth(1.0);                     
    glDepthFunc(GL_LESS);                  
    glEnable(GL_DEPTH_TEST);               
    glShadeModel(GL_SMOOTH);               

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();  

    aspect = (GLdouble)Width / Height;
    glOrtho(-3.0, 3.0, -3.0 / aspect, 3.0 / aspect, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
}

 
void DrawGLScene(SDL_Window *window, GLuint texture, GLfloat *texcoord)
{
     
    enum
    {
        MINX,
        MINY,
        MAXX,
        MAXY
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    glLoadIdentity();                                    

    glTranslatef(-1.5f, 0.0f, 0.0f);  

     
    glBegin(GL_POLYGON);             
    glColor3f(1.0f, 0.0f, 0.0f);     
    glVertex3f(0.0f, 1.0f, 0.0f);    
    glColor3f(0.0f, 1.0f, 0.0f);     
    glVertex3f(1.0f, -1.0f, 0.0f);   
    glColor3f(0.0f, 0.0f, 1.0f);     
    glVertex3f(-1.0f, -1.0f, 0.0f);  
    glEnd();                         

    glTranslatef(3.0f, 0.0f, 0.0f);  

     
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

     
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1.0f, 1.0f, 1.0f);
    if (shaders_supported) {
        glUseProgramObjectARB(shaders[current_shader].program);
    }

    glBegin(GL_QUADS);  
    glTexCoord2f(texcoord[MINX], texcoord[MINY]);
    glVertex3f(-1.0f, 1.0f, 0.0f);  
    glTexCoord2f(texcoord[MAXX], texcoord[MINY]);
    glVertex3f(1.0f, 1.0f, 0.0f);  
    glTexCoord2f(texcoord[MAXX], texcoord[MAXY]);
    glVertex3f(1.0f, -1.0f, 0.0f);  
    glTexCoord2f(texcoord[MINX], texcoord[MAXY]);
    glVertex3f(-1.0f, -1.0f, 0.0f);  
    glEnd();                         

    if (shaders_supported) {
        glUseProgramObjectARB(0);
    }
    glDisable(GL_TEXTURE_2D);

     
    SDL_GL_SwapWindow(window);
}

int main(int argc, char **argv)
{
    int done;
    SDL_Window *window;
    SDL_Surface *surface;
    GLuint texture;
    GLfloat texcoords[4];

     
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

     
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

     
    window = SDL_CreateWindow("Shader Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create OpenGL window: %s\n", SDL_GetError());
        SDL_Quit();
        exit(2);
    }

    if (!SDL_GL_CreateContext(window)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create OpenGL context: %s\n", SDL_GetError());
        SDL_Quit();
        exit(2);
    }

    surface = SDL_LoadBMP("icon.bmp");
    if (surface == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to load icon.bmp: %s\n", SDL_GetError());
        SDL_Quit();
        exit(3);
    }
    texture = SDL_GL_LoadTexture(surface, texcoords);
    SDL_FreeSurface(surface);

     
    InitGL(640, 480);
    if (InitShaders()) {
        SDL_Log("Shaders supported, press SPACE to cycle them.\n");
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shaders not supported!\n");
    }
    done = 0;
    while (!done) {
        DrawGLScene(window, texture, texcoords);

         
        {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    done = 1;
                }
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        current_shader = (current_shader + 1) % NUM_SHADERS;
                    }
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        done = 1;
                    }
                }
            }
        }
    }
    QuitShaders();
    SDL_Quit();
    return 1;
}

#else  

int main(int argc, char *argv[])
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No OpenGL support on this system\n");
    return 1;
}

#endif  

 
