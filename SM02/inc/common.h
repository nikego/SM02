/*****************************************************************************
 * common.h
 *
 ****************************************************************************/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <GLES/gl.h>
#include "EGL/egl.h"
#include "EGL/eglext.h"

#define PI 3.1415926535897932384626433832795

#define TRUE         1
#define FALSE        0

typedef struct egl_state_t {
	uint32_t screen_width;
	uint32_t screen_height;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	EGLDisplay dpy;
	EGLSurface surface;
	EGLContext context;
} egl_state_t;

int initEGL(egl_state_t *state);

void deInitEGL(egl_state_t *state);

void updateScreen(egl_state_t *state);

void prepareEGL(egl_state_t* es);

#define FTOX(fp) (GLfixed)((fp) * (1 << 16))

extern int gQuit;

void signalHandler(int signum);

int get_disp_resolution(int *w, int *h);

#define put_error(...) fprintf(stderr, __VA_ARGS__)

#define put_message(...) fprintf(stderr, __VA_ARGS__)

#define pabort(s) do { perror(s);  goto abort; } while(0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int nextp2(int x);

unsigned char read_console();

static inline void* safe_malloc(int size) {
    void* ptr = malloc(size);
    if (!ptr) { fprintf(stderr, "Out of memory\n"); exit(-1); }
    return ptr;
}

static inline void safe_free(void** pp) {
    if (pp && *pp) {
        free(*pp);
        *pp = 0;
    }
}

long long current_timestamp();

// вычисляет значение канала цвета
#ifdef SIXBITSCOLOR

#define C(c) ((c) * 0.249)

#else

#define C(c) (c)

#endif

// установить значение (X,Y,Z) для вершины I в плоском массиве P
#define SET_VERT(P, I, X, Y, Z) (P)[(I)*3]=(X);(P)[(I)*3+1]=(Y);(P)[(I)*3+2]=(Z)

// установить значение цвета (R,G,B,A) для вершины I в плоском массиве P
#define SET_COL(P, I, R, G, B, A) (P)[(I)*4]=(R);(P)[(I)*4+1]=(G);(P)[(I)*4+2]=(B);(P)[(I)*4+3]=(A)

#endif /* __COMMON_H__ */
