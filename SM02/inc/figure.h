#ifndef __FIGURE_H__
#define __FIGURE_H__

// forward declaration
struct figure_t;

struct figure_t* makeRect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);

struct figure_t* makeRect2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat r, GLfloat g, GLfloat b, GLfloat a);

void drawFigure(const struct figure_t* fig);

struct figure_t* makeCirc(GLfloat radius);

#endif // __FIGURE_H__
