#include "common.h"
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <png.h>

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

typedef struct image_t {
    int size;            // size of the struct
    GLuint tex;          //
    GLfloat vertices[8]; //
    GLfloat tex_coord[8];//
    int w, h;
} image_t;

enum { DI_NONE, DI_FLOYD_STEINBERG, DI_UNIFORM };

/* dithering */

/*
static inline void add_clip8(uint8_t* p, int v) {
    int nv = *p + v;
    if (nv < 0) *p = 0;
    else if (nv > 255) *p = 255;
    else *p = nv;
}

static void dithering(png_byte* l, int lines, int rowbytes, int dith, int param) {
	uint32_t x,y;
    if (dith == DI_UNIFORM) {
        int bound = param;
        int mod = bound * 2 + 1;
        
        for (y = 0; y < lines; y++) {
            for (x = 0; x < rowbytes; x += 4) {
                png_byte* p = l + x + 3;
                if (*p > 0 && *p < 255)
                    add_clip8(p, rand() % mod - bound);            
            }
            l += rowbytes;
        }    
    }
    else if (dith == DI_FLOYD_STEINBERG) {    
        int mask = param;
        for (y = 0; y < lines; y++) {
            for (x = 0; x < rowbytes; x += 4) {
                int p = l[x + 3];            
                if (p > 0 && p < 255) {
                    const int right = x < (rowbytes - 4), down = y < (lines - 1), left = x > 0;
                    int np = p & -mask; // quantification
                    int qe = p - np;
                    if (qe != 0) {
                        l[x + 3] = np;
                        if (right) add_clip8(l + x + 4 + 3, qe * 7 / 16);
                        if (left && down) add_clip8(l + x + rowbytes - 4 + 3, qe * 3 / 16);
                        if (down) add_clip8(l + x + rowbytes + 0 + 3, qe * 5 / 16);
                        if (right && down) add_clip8(l + x + rowbytes + 4 + 3, qe * 1 / 16);                
                    }
                }
            }
            l += rowbytes;
        }
    }
}
*/

int load_texture(const char* filename, int* width, int* height, float* tex_x, float* tex_y, unsigned int *tex, int dith, int param) {
    int i;
    GLuint format;
    png_byte header[8];    /* header for testing if it is a png */

    if (!tex) {
        return EXIT_FAILURE;
    }

    /* Open file as binary */
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        put_error("No such file!\n");
        return EXIT_FAILURE;
    }

    /* Read the header */
    fread(header, 1, 8, fp);

    /* Test if png */
    int is_png = !png_sig_cmp(header, 0, 8);
    if (!is_png) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Create png struct */
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Create png info struct */
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Create png info struct */
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Set up error handling (required without using custom error handlers above) */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Initialize png reading */
    png_init_io(png_ptr, fp);

    /* Let libpng know you already read the first 8 bytes */
    png_set_sig_bytes(png_ptr, 8);

    /* Read all the info up to the image data */
    png_read_info(png_ptr, info_ptr);

    /* Variables to pass to get info */
    int bit_depth, color_type;
    png_uint_32 image_width, image_height;

    /* Get info about png */
    png_get_IHDR(png_ptr, info_ptr, &image_width, &image_height, &bit_depth, &color_type, NULL, NULL, NULL);

    switch (color_type)
    {
        case PNG_COLOR_TYPE_RGBA:
            format = GL_RGBA;            
            break;

        case PNG_COLOR_TYPE_RGB:
            format = GL_RGB;            
            break;

        default:
            put_error("Unsupported PNG color type (%d) for texture: %s", (int)color_type, filename);
            fclose(fp);
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
            return -1;
    }

    /* Update the png info struct. */
    png_read_update_info(png_ptr, info_ptr);

    /* Row size in bytes. */
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    /* Allocate the image_data as a big block, to be given to opengl */
    png_byte *image_data = (png_byte*) malloc(sizeof(png_byte) * rowbytes * image_height);

    if (!image_data) {
        /* clean up memory and close file */
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Row_pointers is for pointing to image_data for reading the png with libpng */
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * image_height);
    if (!row_pointers) {
        /* clean up memory and close stuff */
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        free(image_data);
        fclose(fp);
        return EXIT_FAILURE;
    }

    /* Set the individual row_pointers to point at the correct offsets of image_data */
    for (i = 0; i < (int)image_height; i++) {
        row_pointers[image_height - 1 - i] = image_data + i * rowbytes;
    }

    /* Read the png into image_data through row_pointers */
    png_read_image(png_ptr, row_pointers);
    
#ifdef SIXBITSCOLOR
    for (uint32_t ii = 0; ii < rowbytes * image_height; ii += 4) {
        image_data[ii + 0] >>= 2;
        image_data[ii + 1] >>= 2;
        image_data[ii + 2] >>= 2;        
    }
#endif

    //dithering(image_data, image_height, rowbytes, dith, param);
    
    int tex_width, tex_height;

    tex_width = nextp2(image_width);
    tex_height = nextp2(image_height);

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, (*tex));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
#if 0    
    int rgbformat = GL_UNSIGNED_SHORT_5_5_5_1; // GL_UNSIGNED_BYTE
#else
    int rgbformat = GL_UNSIGNED_BYTE;
#endif    
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, tex_width, tex_height, 0, format, rgbformat, NULL);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width, image_height, format, rgbformat, image_data);

    GLint err = glGetError();

    /* Clean up memory and close file */
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    free(image_data);
    free(row_pointers);
    fclose(fp);

    if (err == 0) {
        /* Return physical with and height of texture if pointers are not null */
        if(width) {
            *width = image_width;
        }
        if (height) {
            *height = image_height;
        }
        /* Return modified texture coordinates if pointers are not null */
        if(tex_x) {
            *tex_x = ((float) image_width - 0.5f) / ((float)tex_width);
        }
        if(tex_y) {
            *tex_y = ((float) image_height - 0.5f) / ((float)tex_height);
        }
        return EXIT_SUCCESS;
    } else {
        put_error("GL error %i \n", err);
        return EXIT_FAILURE;
    }
}

image_t* loadImage(const char* filename) {
    float tex_x, tex_y;
    int width, height;
    GLuint tex;
    if (EXIT_SUCCESS != load_texture(filename, &width, &height, &tex_x, &tex_y, &tex, DI_UNIFORM, 4)) {
        put_error("Unable to load texture\n");
        return NULL;
    }
    image_t* img = (image_t*)malloc(sizeof(image_t));
    img->size = sizeof(image_t);
    img->tex = tex;
    img->w = width;
    img->h = height;
    
    img->vertices[0] = -width / 2.0f;
    img->vertices[1] = -height / 2.0f;
    
    img->vertices[2] = width / 2.0f;
    img->vertices[3] = -height / 2.0f;
    
    img->vertices[4] = -width / 2.0f;
    img->vertices[5] = height / 2.0f;
    
    img->vertices[6] = width / 2.0f;
    img->vertices[7] = height / 2.0f;

    img->tex_coord[0] = 0.0f;
    img->tex_coord[1] = 0.0f;
    img->tex_coord[2] = tex_x;
    img->tex_coord[3] = 0.0f;
    img->tex_coord[4] = 0.0f;
    img->tex_coord[5] = tex_y;
    img->tex_coord[6] = tex_x;
    img->tex_coord[7] = tex_y;
    
    return img;
}

void drawImage(const image_t* img) {
    if (!img)
        return;
        
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
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //GL_ONE_MINUS_SRC_ALPHA

    glVertexPointer(2, GL_FLOAT, 0, img->vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, img->tex_coord);
    glBindTexture(GL_TEXTURE_2D, img->tex);

    glColor4ub(250, 250, 250, 250);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
}
