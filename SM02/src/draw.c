#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "draw.h"
#include "figure.h"
#include "image.h"
#include "font.h"
#include "common.h"

#define VIRT_W 1280
#define VIRT_H 720

void init(struct draw_state_t* s) {

    glScalef(1.0 * 2 / VIRT_W, 1.0 * 2 / VIRT_H, 0);
    glClearColor(2 / 255.0, 40 / 255.0, 76 / 255.0, 1);

    s->rectangles[WHITE_LEFT] = makeRect2(-560, 10, -195, 220, 250, 250, 250, 255);
    s->rectangles[WHITE_RIGTH] = makeRect2(195, 10, 560, 220, 250, 250, 250, 255);

    s->rectangles[RED_LEFT] = makeRect2(-560, 10, -195, 220, 238, 49, 53, 255);
    s->rectangles[GREEN_RIGHT] = makeRect2(195, 10, 560, 220, 0, 168, 90, 255);

    s->rectangles[YELLOW_LEFT] = makeRect2(-560, 260, -195, 310, 255, 242, 16, 255);
    s->rectangles[YELLOW_RIGHT] = makeRect2(195, 260, 560, 310, 255, 242, 16, 255);

    //s->circ = makeCirc(200);
    s->logo = loadImage("images/logo.png");
    s->logo2 = loadImage("images/logo2.png");

    //setFont(MAIN_FONT, 30);
    //setFont(MAIN_FONT, 50);
}

void drawTest(struct draw_state_t* s) {
    if (!s->pause) {
        s->angle += 360 / 2000.0;
        if (s->angle >= 360)
            s->angle -= 360;
        s->cnt++;
    }
    if (s->cnt % 60 == 5) {
        s->mask = rand() % 64;
    }
    glPushMatrix();
    int k;
    if ((s->mask & 3) > 0) {
        k = s->mask & 3;
        if (k != 0 || k != 3)
            drawFigure(s->rectangles[k - 1]);
    }
    if (((s->mask >> 2) & 3) > 0) {
        k = ((s->mask >> 2) & 3);
        if (k != 0 || k != 3)
            drawFigure(s->rectangles[k - 1 + 3]);
    }
    if (s->mask & (1 << 4)) {
        drawFigure(s->rectangles[YELLOW_LEFT]);
    }
    if (s->mask & (1 << 5)) {
        drawFigure(s->rectangles[YELLOW_RIGHT]);
    }

    setFont(MAIN_FONT, 30);
    float tw, th, tw2;
    const char* title = "Foil";
    measureText(title, &tw, &th, -1);

    glColor4ub(220, 220, 220, 248);
    renderText(title, -tw / 2, VIRT_H * 9 / 20 - th, -1);

    char text[32];
    if (s->fight->duration < 10 * 10000) {
        glColor4ub(220, 49, 53, 250);
        if (s->fight->duration >= 0)
            sprintf(text, "%02ld:%02ld", s->fight->duration / 10000, (s->fight->duration / 100) % 100);
        else
            strcpy(text, "00:00");
    } else {
        glColor4ub(220, 220, 220, 250);
        sprintf(text, "%02ld:%02ld", s->fight->duration / 10000 / 60, (s->fight->duration / 10000) % 60);
    }
    setFont(MAIN_FONT, 36);
    glScalef(2, 2, 1);
    measureText(text, &tw, &th, 2);
    measureText(":", &tw2, &th, 1);
    renderText(text, -tw - tw2 / 2, - th / 2 + 115 / 2, -1);
    glScalef(1.0 / 2, 1.0 / 2, 1);

    if (s->logo2) {
        glTranslatef(0, - (VIRT_H * 9 / 20), 0);
        glRotatef(s->angle, 0, 0, 1);
        drawImage(s->logo2);
        glRotatef(-s->angle, 0, 0, 1);
    }
    //glScalef(1.0 / 4, 1.0 / 4, 1);

    //glRotatef(angle, 0, 0, 1);
    //glTranslatef(200, 200, 0);

    glPopMatrix();
}

void draw(struct draw_state_t* s) {

    if (s->fight->mode == MODE_TEST) {
        drawTest(s);
    }

}
