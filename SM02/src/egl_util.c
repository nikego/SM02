#include <fcntl.h>
#include <linux/fb.h>
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "common.h"
#include "bcm_host.h"

int get_disp_resolution(int* w, int* h)
{
    int fb_fd, ret = -1;
    struct fb_var_screeninfo vinfo;

    if ((fb_fd = open("/dev/fb0", O_RDONLY)) < 0) {
        printf("failed to open fb0 device\n");
        return ret;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOGET_VSCREENINFO");
        goto exit;
    }

    *w = vinfo.xres;
    *h = vinfo.yres;

    printf(" \n Display Resolution: %dx%d : %d\n\n", *w, *h, vinfo.bits_per_pixel);
    if (*w && *h)
        ret = 0;

exit:
    close(fb_fd);
    return ret;
}

void egl_print_err(char* name)
{
    char* err_str[] = { "EGL_SUCCESS",       "EGL_NOT_INITIALIZED", "EGL_BAD_ACCESS",        "EGL_BAD_ALLOC",
                        "EGL_BAD_ATTRIBUTE", "EGL_BAD_CONFIG",      "EGL_BAD_CONTEXT",       "EGL_BAD_CURRENT_SURFACE",
                        "EGL_BAD_DISPLAY",   "EGL_BAD_MATCH",       "EGL_BAD_NATIVE_PIXMAP", "EGL_BAD_NATIVE_WINDOW",
                        "EGL_BAD_PARAMETER", "EGL_BAD_SURFACE" };

    EGLint ecode = eglGetError();

    fprintf(stderr, "'%s': egl error '%s' (0x%x)\n", name, err_str[ecode - EGL_SUCCESS], ecode);
}


void updateScreen(egl_state_t* s) {
    eglSwapBuffers(s->dpy, s->surface);
}

void deInitEGL(egl_state_t* s)
{
    eglMakeCurrent(s->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (s->context != EGL_NO_CONTEXT) {
        eglDestroyContext(s->dpy, s->context);
    }
    if (s->surface != EGL_NO_SURFACE) {
        eglDestroySurface(s->dpy, s->surface);
    }
    eglTerminate(s->dpy);
}

#define SMOOTH 1

int initEGL(egl_state_t* s)
{
   bcm_host_init();

   int32_t success = 0;

   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 	1,
      EGL_GREEN_SIZE, 	1,
      EGL_BLUE_SIZE, 	1,
      EGL_ALPHA_SIZE, 	1,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
#ifdef SMOOTH
	  EGL_DEPTH_SIZE,   EGL_DONT_CARE,
	  EGL_BUFFER_SIZE,   EGL_DONT_CARE,
	  EGL_SAMPLE_BUFFERS, 1,
	  EGL_SAMPLES,      4,
#endif
      EGL_NONE
   };

   EGLConfig config;

   // get an EGL display connection
   s->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(s->dpy!=EGL_NO_DISPLAY);

   // initialize the EGL display connection
   result = eglInitialize(s->dpy, NULL, NULL);
   assert(EGL_FALSE != result);

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(s->dpy, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);

   // create an EGL rendering context
   s->context = eglCreateContext(s->dpy, config, EGL_NO_CONTEXT, NULL);
   assert(s->context!=EGL_NO_CONTEXT);

   // create an EGL window surface
   success = graphics_get_display_size(0 /* LCD */, &s->screen_width, &s->screen_height);
   assert( success >= 0 );

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = s->screen_width;
   dst_rect.height = s->screen_height;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = s->screen_width << 16;
   src_rect.height = s->screen_height << 16;

   s->dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );

   s->dispman_element = vc_dispmanx_element_add ( dispman_update, s->dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

   nativewindow.element = s->dispman_element;
   nativewindow.width = s->screen_width;
   nativewindow.height = s->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );

   s->surface = eglCreateWindowSurface( s->dpy, config, &nativewindow, NULL );
   assert(s->surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(s->dpy, s->surface, s->surface, s->context);
   assert(EGL_FALSE != result);

   // Enable back face culling.
   glEnable(GL_CULL_FACE);

   glMatrixMode(GL_MODELVIEW);

   return 0;
}

void prepareEGL(egl_state_t* es) {
	glShadeModel(GL_SMOOTH);

    glDepthMask(GL_TRUE);
    glStencilMask(0);

    glEnable(GL_BLEND);
    glEnable(GL_NORMALIZE);

#ifdef SMOOTH
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
#endif

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glOrthof(-1, 1, -1, 1, -10, 10);
    glViewport(0, 0, es->screen_width, es->screen_height);

    glGetError();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLfloat widths[2];
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, widths);

    put_message("Aliased line width: %.1f - %.1f\n", widths[0], widths[1]);
}

#if 0

int initEGL(int* surf_w, int* surf_h)
{
    typedef NativeDisplayType EGLNativeDisplayType;

    EGLint disp_w, disp_h;
    EGLNativeDisplayType disp_type;
    EGLConfig cfgs[2];
    EGLint n_cfgs;

    static EGL_DISPMANX_WINDOW_T nativewindow;

    EGLint egl_attr[] = {   //EGL_BUFFER_SIZE,    EGL_DONT_CARE,
                            EGL_RED_SIZE,       8,
                            EGL_GREEN_SIZE,     8,
                            EGL_BLUE_SIZE,      8,
                            EGL_ALPHA_SIZE,     8,
                            //EGL_DEPTH_SIZE,     EGL_DONT_CARE,
                            EGL_SURFACE_TYPE,   EGL_WINDOW_BIT, // EGL_PIXMAP_BIT, //
                            //EGL_SAMPLE_BUFFERS, 1,
                            //EGL_SAMPLES,        4,
                            EGL_NONE };

    if (get_disp_resolution(&disp_w, &disp_h)) {
        printf("ERROR: get display resolution failed\n");
        return -1;
    }

    disp_type = (EGLNativeDisplayType)EGL_DEFAULT_DISPLAY;

    dpy = eglGetDisplay(disp_type);

    if (eglInitialize(dpy, NULL, NULL) != EGL_TRUE) {
        egl_print_err("eglInitialize");
        return -1;
    }

    if (eglGetConfigs(dpy, cfgs, 2, &n_cfgs) != EGL_TRUE) {
        egl_print_err("eglGetConfigs");
        goto cleanup;
    }

    if (eglChooseConfig(dpy, egl_attr, cfgs, 2, &n_cfgs) != EGL_TRUE) {
        egl_print_err("eglChooseConfig");
        goto cleanup;
    }

    nativewindow.element = 1;
    nativewindow.width = disp_w;
    nativewindow.height = disp_h;

    surface = eglCreateWindowSurface(dpy, cfgs[0], &nativewindow, NULL);

    if (surf_w && surf_h) {
        *surf_w = disp_w;
        *surf_h = disp_h;
    }

    context = eglCreateContext(dpy, cfgs[0], EGL_NO_CONTEXT, NULL);

    if (context == EGL_NO_CONTEXT) {
        egl_print_err("eglCreateContext");
        goto cleanup;
    }

    if (eglMakeCurrent(dpy, surface, surface, context) != EGL_TRUE) {
        egl_print_err("eglMakeCurrent");
        goto cleanup;
    }

    if (eglSurfaceAttrib(dpy, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED) != EGL_TRUE) {
        egl_print_err("eglSurfaceAttrib");
        goto cleanup;
    }

    return 0;

cleanup:
    deInitEGL();
    return -1;
}

#endif
