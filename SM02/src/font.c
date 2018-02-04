#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <EGL/egl.h>
#include <GLES/gl.h>

#include "font.h"
#include "common.h"

#define MAXCHARS 128

#define HAVE_KERNING 1

struct font_t {
    unsigned int font_texture;
    float pt;
    float advance[MAXCHARS];
    float width[MAXCHARS];
    float height[MAXCHARS];
    float tex_x1[MAXCHARS];
    float tex_x2[MAXCHARS];
    float tex_y1[MAXCHARS];
    float tex_y2[MAXCHARS];
    float offset_x[MAXCHARS];
    float offset_y[MAXCHARS];
    int initialized;
#if HAVE_KERNING
    int use_kerning;
    FT_Face face;
#endif    
};

typedef struct font_t font_t;

typedef struct font_item_t {
    char fontname[32];
    font_t* fp;
    GLfloat *vertices;
    int vcap;
    GLfloat *texture_coords;
    int tcap;
    GLshort *indices;
    int icap;
    struct font_item_t* next;
} font_item_t;

static font_item_t* fonts = NULL;

static font_item_t* current_font = NULL;

static int initialized = 1;

#if HAVE_KERNING

int get_kerning(font_t* font, int prev, int current, int* kx) {
    
    FT_Vector vector;
    int glyph1 = FT_Get_Char_Index(font->face, prev);
    int glyph2 = FT_Get_Char_Index(font->face, current);
    int error = FT_Get_Kerning(font->face, glyph1, glyph2, ft_kerning_default, &vector);
    
    if (error) {
        put_error("FT_Get_Kerning() return error %d\n", error);
    }
    if (kx)
        *kx = (int)(vector.x) >> 6;
        
    return 0;
}

#endif

/* Utility function to load and ready the font for use by OpenGL */

font_t* loadFont(const char* path, float point_size, int dpi) {
    FT_Library library;
    FT_Face face;
    int c;
    int i, j;
    font_t* font;

    if (!initialized) {
        put_error("EGL has not been initialized\n");
        return NULL;
    }

    if (!path){
        put_error("Invalid path to font file\n");
        return NULL;
    }

    if (FT_Init_FreeType(&library)) {
        put_error("Error loading Freetype library\n");
        return NULL;
    }
    if (FT_New_Face(library, path,0,&face)) {
        put_error("Error loading font %s\n", path);
        return NULL;
    }

    if (FT_Set_Char_Size ( face, point_size * 64, point_size * 64, dpi, dpi)) {
        put_error("Error initializing character parameters\n");
        return NULL;
    }

    font = (font_t*) malloc(sizeof(font_t));
    font->initialized = 0;
    
    glGenTextures(1, &(font->font_texture));

    /*Let each glyph reside in 32x32 section of the font texture */
    int segment_size_x = 0;
    int segment_size_y = 0;
    int num_segments_x = 16;
    int num_segments_y = 16;

    FT_GlyphSlot slot;
    FT_Bitmap bmp;
    int glyph_width, glyph_height;

    /* First calculate the max width and height of a character in a passed font */
    for (c = 0; c < MAXCHARS; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            put_error("FT_Load_Char failed\n");
            free(font);
            return NULL;
        }

        slot = face->glyph;
        bmp = slot->bitmap;

        glyph_width = nextp2(bmp.width);
        glyph_height = nextp2(bmp.rows);

        if (glyph_width > segment_size_x) {
            segment_size_x = glyph_width;
        }

        if (glyph_height > segment_size_y) {
            segment_size_y = glyph_height;
        }
    }
    
    int font_tex_width = nextp2(num_segments_x * segment_size_x);
    int font_tex_height = nextp2(num_segments_y * segment_size_y);

    put_message("texture: %d x %d ", font_tex_width, font_tex_height);

    int bitmap_offset_x = 0, bitmap_offset_y = 0;

    GLubyte* font_texture_data = (GLubyte*) malloc(sizeof(GLubyte) * 2 * font_tex_width * font_tex_height);
    memset((void*)font_texture_data, 0, sizeof(GLubyte) * 2 * font_tex_width * font_tex_height);

    if (!font_texture_data) {
        put_error("Failed to allocate memory for font texture\n");
        free(font);
        return NULL;
    }

    /* Fill font texture bitmap with individual bmp data and record appropriate size,
       texture coordinates and offsets for every glyph */

    for (c = 0; c < MAXCHARS; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            put_error("FT_Load_Char failed\n");
            free(font);
            return NULL;
        }

        slot = face->glyph;
        bmp = slot->bitmap;

        glyph_width = nextp2(bmp.width);
        glyph_height = nextp2(bmp.rows);

        div_t temp = div(c, num_segments_x);

        bitmap_offset_x = segment_size_x * temp.rem;
        bitmap_offset_y = segment_size_y * temp.quot;

        for (j = 0; j < glyph_height; j++) {
            int yofs = (j + bitmap_offset_y) * font_tex_width;
            for (i = 0; i < glyph_width; i++) {
                int idx = 2 * (yofs + (bitmap_offset_x + i));
                font_texture_data[idx + 0] = font_texture_data[idx + 1] =
                    (i >= bmp.width || j >= bmp.rows) ? 0 : bmp.buffer[i + bmp.width * j];
            }
        }
        font->advance[c]  = (float)(slot->advance.x >> 6);
        font->tex_x1[c]   = (float)bitmap_offset_x / font_tex_width;
        font->tex_x2[c]   = (float)(bitmap_offset_x + bmp.width) / font_tex_width;
        font->tex_y1[c]   = (float)bitmap_offset_y / font_tex_height;
        font->tex_y2[c]   = (float)(bitmap_offset_y + bmp.rows) / font_tex_height;
        font->width[c]    = bmp.width;
        font->height[c]   = bmp.rows;
        font->offset_x[c] = (float)slot->bitmap_left;
        font->offset_y[c] = (float)((slot->metrics.horiBearingY - face->glyph->metrics.height) >> 6);
    }    
    glBindTexture(GL_TEXTURE_2D, font->font_texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, font_tex_width, font_tex_height, 0, GL_LUMINANCE_ALPHA , GL_UNSIGNED_BYTE, font_texture_data);

    int err = glGetError();

    free(font_texture_data);
    
#if HAVE_KERNING
    if (FT_HAS_KERNING(face)) {
        font->use_kerning = 1;        
    } else {
        font->use_kerning = 0;
        put_message("no kerning ");
    }
    font->face = face;
#else
    FT_Done_Face(face);
    FT_Done_FreeType(library);
#endif

    if (err != 0) {
        put_error("GL Error 0x%x", err);
        free(font);
        return NULL;
    }
    font->pt = point_size;
    font->initialized = 1;
    return font;
}

//#define EGL_TEST(foo) foo; err = glGetError(); if (err != 0) { fprintf(stderr, #foo " GL Error 0x%x", err);  font->initialized = 0; goto fail; }

#define MAXFONTS 32

int setFont(const char* fontname, float pointsize) {
    font_item_t* ptr = fonts, *prev = NULL;
    int cnt = 0;

    while (ptr != NULL && ++cnt < MAXFONTS) {
        if (ptr->fp->pt == pointsize && strcmp(ptr->fontname, fontname) == 0) {
            break;            
        }
        prev = ptr;
        ptr = ptr->next;
    }
    if (cnt == MAXFONTS) {
        put_error("oops! too many fonts\n");
        return -1;
    }
    if (ptr == NULL) {

        char* filename = (char*)safe_malloc(1024);
        strcpy(filename, FONT_DIR);
        strcat(filename, fontname);
        
        put_message("loading new font '%s':%.1f... ", filename, pointsize);        
        font_t* fp = loadFont(filename, pointsize, 102);
             
        if (fp && fp->initialized) {
            put_message("OK\n");
            ptr = (font_item_t*)safe_malloc(sizeof(font_item_t));
            memset(ptr, 0, sizeof(font_item_t));
            strcpy(ptr->fontname, fontname);
            ptr->fp = fp;
            ptr->next = NULL;
        }
        else {
            put_message("Failed\n");
            safe_free((void**)&fp);            
            return -1;
        }
    }
    //put_message("ptr:%p(%s:%.1f), pt:%.1f, cnt %d\n", ptr->fp, ptr->fontname, ptr->fp->pt, pointsize, cnt);
    if (ptr != fonts) {
        if (prev != NULL) {
            prev->next = ptr->next;
        }
        ptr->next = fonts;
        fonts = ptr;
    }
    current_font = ptr;
    return 0;
}

#define EGL_TEST(foo) do { foo; } while(0)

void* fast_malloc(void* p, int size, int* allocated) {
    if (p && *allocated >= size) return p;
    if (p) free(p);
    *allocated = size;
    return malloc(size);
}

enum {
    ALIGN_CENTER    = 0,
    ALIGN_TOP       = 1,
    ALIGN_BOTTOM    = 2,
    ALIGN_LEFT      = 4,
    ALIGN_RIGHT     = 8,    
    ALIGN_LEFTTOP       = ALIGN_LEFT|ALIGN_TOP,
    ALIGN_LEFTBOTTOM    = ALIGN_LEFT|ALIGN_BOTTOM, 
    ALIGN_RIGHTTOP      = ALIGN_RIGHT|ALIGN_TOP,
    ALIGN_RIGHTBOTTOM   = ALIGN_RIGHT|ALIGN_BOTTOM
};

void drawText(const char* msg, float x, float y, int align) {
    
}

void renderText(const char* msg, float x, float y, int msglen) {
    
    font_item_t* font = current_font;
    
    if (!font || !font->fp) {
        fprintf(stderr, "Font must not be null\n");
        return;
    }

    if (!font->fp->initialized) {
        fprintf(stderr, "Font has not been loaded\n");
        return;
    }

    if (!msg) {
        return;
    }
    
    int c, err;
    if (msglen < 0)
    	msglen = strlen(msg);

    float pen_x = 0.0f;

    int texture_enabled;
    glGetIntegerv(GL_TEXTURE_2D, &texture_enabled);
    if (!texture_enabled) {
        glEnable(GL_TEXTURE_2D);
    }

    int blend_enabled;
    glGetIntegerv(GL_BLEND, &blend_enabled);
    if (!blend_enabled) {
        glEnable(GL_BLEND);
    }

    int gl_blend_src, gl_blend_dst;
    glGetIntegerv(GL_BLEND_SRC, &gl_blend_src);
    glGetIntegerv(GL_BLEND_DST, &gl_blend_dst);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    int vertex_array_enabled;
    glGetIntegerv(GL_VERTEX_ARRAY, &vertex_array_enabled);
    if (!vertex_array_enabled) {
        glEnableClientState(GL_VERTEX_ARRAY);
    }

    int texture_array_enabled;
    glGetIntegerv(GL_TEXTURE_COORD_ARRAY, &texture_array_enabled);
    if (!texture_array_enabled) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    font->vertices = (GLfloat*)fast_malloc(font->vertices, sizeof(GLfloat) * 8 * msglen, &font->vcap);

    font->texture_coords = (GLfloat*)fast_malloc(font->texture_coords, sizeof(GLfloat) * 8 * msglen, &font->tcap);

    font->indices = (GLshort*)fast_malloc(font->indices, sizeof(GLshort) * 6 * msglen, &font->icap);
    
    font_t* f = font->fp;
    int prev = 0, i;
    
    for (i = 0; i < msglen; ++i) {
        c = msg[i];
        GLfloat *v = font->vertices + 8 * i + 0;
                
        v[0] = x + pen_x + f->offset_x[c];
        v[1] = y + f->offset_y[c];
        v[2] = v[0] + f->width[c];
        v[3] = v[1];
        v[4] = v[0];
        v[5] = v[1] + f->height[c];
        v[6] = v[2];
        v[7] = v[5];

        GLfloat* tc = font->texture_coords + 8 * i;
        tc[0] = f->tex_x1[c];
        tc[1] = f->tex_y2[c];
        tc[2] = f->tex_x2[c];
        tc[3] = f->tex_y2[c];
        tc[4] = f->tex_x1[c];
        tc[5] = f->tex_y1[c];
        tc[6] = f->tex_x2[c];
        tc[7] = f->tex_y1[c];

        GLshort * idx = font->indices + i * 6, k = 4 * i;
        idx[0] = k + 0;
        idx[1] = k + 1;
        idx[2] = k + 2;
        idx[3] = k + 2;
        idx[4] = k + 1;
        idx[5] = k + 3;

        /* Assume we are only working with typewriter fonts */
        pen_x += f->advance[c];
#if HAVE_KERNING
        if (f->use_kerning && prev != 0) {
            int kx = 0;            
            get_kerning(f, prev, c, &kx);
            pen_x += kx;
        }
        prev = c;
#endif        
    }

    EGL_TEST(glVertexPointer(2, GL_FLOAT, 0, font->vertices));

    EGL_TEST(glTexCoordPointer(2, GL_FLOAT, 0, font->texture_coords));

    EGL_TEST(glBindTexture(GL_TEXTURE_2D, f->font_texture));

    EGL_TEST(glDrawElements(GL_TRIANGLES, 6 * strlen(msg), GL_UNSIGNED_SHORT, font->indices));

    err = glGetError();

    if (!texture_array_enabled) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (!vertex_array_enabled) {
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if (!texture_enabled) {
        glDisable(GL_TEXTURE_2D);
    }

    glBlendFunc(gl_blend_src, gl_blend_dst);

    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }

    if (err != 0) {
        fprintf(stderr, "DRAW GL Error 0x%x", err);
        f->initialized = 0;
    }
}

void measureText(const char* msg, float* width, float* height, int msglen) {
    int c;
    unsigned i;
    
    if (!msg || !current_font) {
        return;
    }
    font_t* font = current_font->fp;
    if (msglen < 0)
    	msglen = strlen(msg);

    if (width) {
        /* Width of a text rectangle is a sum advances for every glyph in a string */
        *width = 0.0f;

        for (i = 0; i < msglen; ++i) {
            c = msg[i];
            *width += font->advance[c];
        }
    }

    if (height) {
        /* Height of a text rectangle is a high of a tallest glyph in a string */
        *height = 0.0f;

        for (i = 0; i < msglen; ++i) {
            c = msg[i];
            if (*height < font->height[c]) {
                *height = font->height[c];
            }
        }
    }
}
