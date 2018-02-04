#ifndef __FONT_H__
#define __FONT_H__

#define FONT_DIR "fonts/"
#define MAIN_FONT "geomlight.otf"

int setFont(const char* fontname, float pointsize);

void renderText(const char* msg, float x, float y, int msglen);

void measureText(const char* msg, float* w, float* h, int msglen);

#endif // __FONT_H__
