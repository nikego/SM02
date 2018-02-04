#include "common.h"
#include "figure.h"

typedef struct figure_t {
    int size;       // size of the struct
    int nv;         // number of vertices
    GLfloat * pv;   // array of vertices
    GLfloat * pc;   // colors
    int alloc_v, alloc_c; // flags is array allocated
    int mode;           // type of primitive to draw
    int fillmode;       // type of fill primitive
    int bordermode;     // type of border primitive - unused now
    struct figure_t* next;
} figure_t;


figure_t* alloc_figure(int num, void* pv, void* pc)
{
    figure_t* f = (figure_t*)safe_malloc(sizeof(figure_t));
    f->size = sizeof(figure_t);
    f->nv = num;
    f->alloc_c = f->alloc_v = 0;
    f->next = 0;
    if (!pv) {
        f->pv = (GLfloat*)safe_malloc(sizeof(GLfloat) * num * 3);
        f->alloc_v = 1;
    } else
        f->pv = pv;

    if (!pc) {
        f->pc = (GLfloat*)safe_malloc(sizeof(GLfloat) * num * 4);
        f->alloc_c = 1;
    } else
        f->pc = pc;

    return f;
}

void free_figure(figure_t* f)
{
    if (!f)
        return;
    if (f->alloc_v)
        safe_free((void**)&(f->pv));
    if (f->alloc_c)
        safe_free((void**)&(f->pc));
    if (f->next)
        free_figure(f->next);
    safe_free((void**)&f);
}

//void print_figure(figure_t* fig);

figure_t* makeCirc(GLfloat radius)
{    
    const float C_RED = C(0.75);
    const float C_GREEN = C(0.75);
    const float C_BLUE = C(0.75); 

    const float C2_RED = C(1);
    const float C2_GREEN = C(1);
    const float C2_BLUE = C(1);
    
    const int SEGM = 36 * 2;    
    const int VERT = 1 + SEGM;
    
    figure_t* f = alloc_figure(VERT, 0, 0);
    f->mode = GL_TRIANGLE_FAN;
    
    figure_t* f1 = f->next = alloc_figure(VERT, 0, 0);
    f1->mode = GL_LINE_LOOP;
   
    int k = 0, j = 0, i;
    for (i = 0; i <= SEGM; i++){
        GLfloat a = i * 2 * PI / SEGM;
        SET_VERT(f->pv, k, cos(a) * radius, sin(a) * radius, 0);
        SET_COL(f->pc, j,  C_RED,  C_GREEN,  C_BLUE, 1.0);
        SET_VERT(f1->pv, k, cos(a) * radius, sin(a) * radius, 0);        
        SET_COL(f1->pc, j, C2_RED, C2_GREEN, C2_BLUE, 1.0);
        k += 1;
        j += 1;
    }
#if 0    
    const int RLINES = 24;
    figure_t* f2 = f->next->next = alloc_figure(RLINES * 2, 0, 0);        
    f2->mode = GL_LINES;
    
    for (int i = 0; i < RLINES; i++) {
        GLfloat a = i * 2 * PI / RLINES;        
        SET_VERT(f2->pv, i * 2 + 0, cos(a) * radius, sin(a) * radius, 0);
        SET_COL(f2->pc, i * 2 + 0, C2_RED, C2_GREEN, C2_BLUE, 1.0);
        
        float r = (i & 1) ? radius * 0.8 : radius * 0.9;
        SET_VERT(f2->pv, i * 2 + 1, cos(a) * r, sin(a) * r, 0);        
        SET_COL(f2->pc, i * 2 + 1, C2_RED, C2_GREEN, C2_BLUE, 1.0);
    }
#endif    
    return f;
}

#if 0
void drawCirc(int vertices, GLfloat angle) {
    //glGetFloatv(GL_MODELVIEW_MATRIX, mtx);
    //printf("matrix after translace:\n");
    //print_matrixf(mtx);

    //glRotatex(FTOX(angle), FTOX(0), FTOX(0), FTOX(1));

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /*  Set pointers to the vertices and color arrays */
    glVertexPointer(3, GL_FLOAT, 0, circ_vertices);
    glColorPointer(4, GL_FLOAT, 0, circ_colors);

    for (int i = 0; i < vertices; i++) {
        SET_COL(circ_colors, i,  C_RED, C_GREEN, C_BLUE, 1);
    }
    glDrawArrays(GL_TRIANGLE_FAN, 0, vertices);

    for (int i = 0; i < vertices; i++) {
        SET_COL(circ_colors, i,  C2_RED, C2_GREEN, C2_BLUE, 1);
    }
    glDrawArrays(GL_LINE_LOOP, 1, vertices - 1);

    glRotatex(FTOX(angle), FTOX(0), FTOX(0), FTOX(1));
    glVertexPointer(3, GL_FLOAT, 0, scale_v);

    float s_angle = 360.0 / SCALES;
    char text[128];

    for (int i = 0; i < SCALES; i++) {
        glRotatex(FTOX(s_angle), FTOX(0), FTOX(0), FTOX(1));
        SET_VERT(scale_v, 1, 0, RADIUS * ((i & 1) ? 0.8 : 0.9), 0);
        glDrawArrays(GL_LINES, 0, 2);
    }
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    float text_w, text_h;
    measureText(font, "000", &text_w, &text_h);

    for (int i = 0; i < SCALES; i++) {
        sprintf(text, "%03d", (int)s_angle * (i));
        renderText(font, text, -text_w / 2, RADIUS * ((i & 1) ? 0.9 : 0.8) - text_h * 1.5);
        glRotatex(FTOX(-s_angle), FTOX(0), FTOX(0), FTOX(1));
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glPopMatrix();
}

#endif

void setColor(figure_t* fig, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
	int i;
	for (i = 0; i < fig->nv; i++) {
		fig->pc[i * 4 + 0] = r;
		fig->pc[i * 4 + 1] = g;
		fig->pc[i * 4 + 2] = b;
		fig->pc[i * 4 + 3] = a;
    }
}

figure_t* makeRect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
	figure_t* rect = alloc_figure(4, 0, 0);
	int v = 0;
	rect->pv[v] = x1; rect->pv[v + 1] = y1;     rect->pv[v + 2] = 0; v+=3;
	rect->pv[v] = x2; rect->pv[v + 1] = y1; 	rect->pv[v + 2] = 0; v+=3;
	rect->pv[v] = x2; rect->pv[v + 1] = y2; 	rect->pv[v + 2] = 0; v+=3;
	rect->pv[v] = x1; rect->pv[v + 1] = y2; 	rect->pv[v + 2] = 0; v+=3;

	rect->fillmode = rect->mode = GL_TRIANGLE_FAN;
	rect->bordermode = GL_LINE_LOOP;
	return rect;
}

figure_t* makeRect2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
	figure_t* fig = makeRect(x1, y1, x2, y2);
	setColor(fig, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
	return fig;
}

void drawFigure(const figure_t* fig)
{   
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);    
    // glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    do {
        glVertexPointer(3, GL_FLOAT, 0, fig->pv);    
        glColorPointer(4, GL_FLOAT, 0, fig->pc);
        
        glDrawArrays(fig->mode, 0, fig->nv);
        
        fig = fig->next;
    } while(fig);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}
