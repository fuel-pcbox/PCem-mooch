#include <stdint.h>
#define BITMAP WINDOWS_BITMAP
#include "glad/glad.h"
#undef BITMAP
#include "resources.h"
#include "win-ogl.h"
#include "video.h"

void fatal(const char *format, ...);
void pclog(const char *format, ...);

void device_force_redraw();

void ogl_blit_memtoscreen(int x, int y, int y1, int y2, int w, int h);
void ogl_blit_memtoscreen_8(int x, int y, int w, int h);

static HWND ogl_hwnd;
static HDC ogl_hdc;
static HGLRC ogl_hrc;

static GLuint ogl_tex;
static GLuint ogl_vao;
static GLuint ogl_vbos[2];

static char* ogl_vs ="#version 330 core\nlayout(location = 0) in vec3 vert;\nlayout(location = 1) in vec2 vertuv;\nout vec2 uv;void main()\n{\n\tgl_Position = vec4(vert,1.0)\n;\tuv = vertuv;}";
static GLuint ogl_vsid;

static char* ogl_fs ="#version 330 core\nin vec2 texcoords;\nout vec4 color;\nuniform sampler2D tex;\nvoid main()\n{\n\tcolor = texture(tex, texcoords)\n;}";
static GLuint ogl_fsid;

static GLuint ogl_prog;

static PALETTE cgapal=
{
        {0,0,0},{0,42,0},{42,0,0},{42,21,0},
        {0,0,0},{0,42,42},{42,0,42},{42,42,42},
        {0,0,0},{21,63,21},{63,21,21},{63,63,21},
        {0,0,0},{21,63,63},{63,21,63},{63,63,63},

        {0,0,0},{0,0,42},{0,42,0},{0,42,42},
        {42,0,0},{42,0,42},{42,21,00},{42,42,42},
        {21,21,21},{21,21,63},{21,63,21},{21,63,63},
        {63,21,21},{63,21,63},{63,63,21},{63,63,63},

        {0,0,0},{0,21,0},{0,0,42},{0,42,42},
        {42,0,21},{21,10,21},{42,0,42},{42,0,63},
        {21,21,21},{21,63,21},{42,21,42},{21,63,63},
        {63,0,0},{42,42,0},{63,21,42},{41,41,41},
        
        {0,0,0},{0,42,42},{42,0,0},{42,42,42},
        {0,0,0},{0,42,42},{42,0,0},{42,42,42},
        {0,0,0},{0,63,63},{63,0,0},{63,63,63},
        {0,0,0},{0,63,63},{63,0,0},{63,63,63},
};

static uint32_t pal_lookup[256];

static GLfloat ogl_verts[] =
{
    -1,-1,0,
    1,-1,0,
    -1,1,0,
    1,1,0
};

static GLfloat ogl_texcoords[] =
{
    -1,-1,
    1,-1,
    -1,1,
    1,1
};

void ogl_init(HWND h)
{
        int c;
        HRESULT hr;
        
        for (c = 0; c < 256; c++)
            pal_lookup[c] = makecol(cgapal[c].r << 2, cgapal[c].g << 2, cgapal[c].b << 2);

        ogl_hwnd = h;
        
        ogl_hdc = GetDC(ogl_hwnd);
        
        PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
            PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
            32,                        //Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            0,                        //Number of bits for the depthbuffer
            0,                        //Number of bits for the stencilbuffer
            0,                        //Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };
        
        SetPixelFormat(ogl_hdc, ChoosePixelFormat(ogl_hdc, &pfd), &pfd);
        
        ogl_hrc = wglCreateContext(ogl_hdc);
        
        wglMakeCurrent(ogl_hdc, ogl_hrc);
	
	gladLoadGL();
        
        glGenTextures(1,&ogl_tex);
        glBindTexture(GL_TEXTURE_2D, ogl_tex);
        
        glGenVertexArrays(1, &ogl_vao);
        
        glGenBuffers(2, &ogl_vbos);
        glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ogl_verts), ogl_verts, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ogl_texcoords), ogl_texcoords, GL_STATIC_DRAW);
	
	ogl_vsid = glCreateShader(GL_VERTEX_SHADER);
	ogl_fsid = glCreateShader(GL_FRAGMENT_SHADER);
	
	glShaderSource(ogl_vsid, 1, &ogl_vs, NULL);
	glCompileShader(ogl_vsid);
	
	glShaderSource(ogl_fsid, 1, &ogl_fs, NULL);
	glCompileShader(ogl_fsid);
	
	ogl_prog = glCreateProgram();
	glAttachShader(ogl_prog, ogl_vsid);
	glAttachShader(ogl_prog, ogl_fsid);
	glLinkProgram(ogl_prog);
	
	glUseProgram(ogl_prog);
	
	video_blit_memtoscreen = ogl_blit_memtoscreen;
        video_blit_memtoscreen_8 = ogl_blit_memtoscreen_8;
}

void ogl_resize(int x, int y)
{
        HRESULT hr;

        ogl_reset();
}

void ogl_reset()
{
    wglMakeCurrent(ogl_hdc, NULL);
    wglDeleteContext(ogl_hrc);
    
    PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
            PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
            32,                        //Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            0,                        //Number of bits for the depthbuffer
            0,                        //Number of bits for the stencilbuffer
            0,                        //Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };
        
        SetPixelFormat(ogl_hdc, ChoosePixelFormat(ogl_hdc, &pfd), &pfd);
        
        ogl_hrc = wglCreateContext(ogl_hdc);
        
        wglMakeCurrent(ogl_hdc, ogl_hrc);
        
        glGenTextures(1,&ogl_tex);
        glBindTexture(GL_TEXTURE_2D, ogl_tex);
        
        glGenVertexArrays(1, &ogl_vao);
        
        glGenBuffers(2, &ogl_vbos);
        glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ogl_verts), ogl_verts, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ogl_texcoords), ogl_texcoords, GL_STATIC_DRAW);
	
	ogl_vsid = glCreateShader(GL_VERTEX_SHADER);
	ogl_fsid = glCreateShader(GL_FRAGMENT_SHADER);
	
	glShaderSource(ogl_vsid, 1, &ogl_vs, NULL);
	glCompileShader(ogl_vsid);
	
	glShaderSource(ogl_fsid, 1, &ogl_fs, NULL);
	glCompileShader(ogl_fsid);
	
	ogl_prog = glCreateProgram();
	glAttachShader(ogl_prog, ogl_vsid);
	glAttachShader(ogl_prog, ogl_fsid);
	glLinkProgram(ogl_prog);

	glUseProgram(ogl_prog);
	
    device_force_redraw();
}

void ogl_close()
{
    wglMakeCurrent(ogl_hdc, NULL);
    wglDeleteContext(ogl_hrc);
}

void ogl_blit_memtoscreen(int x, int y, int y1, int y2, int w, int h)
{
   int xx, yy;
   uint32_t *p;

   if((w < 0) || (w > 2048) || (h < 0) || (h > 2048))
      return;
      
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer32->w, buffer32->h, 0, GL_RGBA, GL_UNSIGNED_INT, buffer32->dat);
   
   glEnableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[1]);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
   
   glEnableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[2]);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
   
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}

void ogl_blit_memtoscreen_8(int x, int y, int w, int h)
{
   int xx, yy;
   uint32_t *p;

   if((w < 0) || (w > 2048) || (h < 0) || (h > 2048))
      return;

   for (yy = 0; yy < h; yy++)
   {
      if ((y + yy) >= 0 && (y + yy) < buffer8->h)
      {
         p = buffer32->dat + yy * buffer32->w * 4;
         for (xx = 0; xx < w; xx++)
         {
            p[xx] = pal_lookup[buffer8->line[y + yy][x + xx]];
            /* If brown circuity is disabled, double the green component. */
            if ((buffer8->line[y + yy][x + xx] == 0x16) && !cga_brown)  p[xx] += (p[xx] & 0xff00);
         }
      }
   }
   
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer32->w, buffer32->h, 0, GL_RGBA, GL_UNSIGNED_INT, buffer32->dat);
   
   glEnableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[1]);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
   
   glEnableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, ogl_vbos[2]);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
   
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}