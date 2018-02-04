/*****************************************************************************
 * main.c
 ****************************************************************************/

#include "common.h"
#include "draw.h"

int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler);

    egl_state_t es = {0};

    if (initEGL(&es)) {
        put_error("ERROR: init EGL failed\n");
        goto err_ret;
    }
    prepareEGL(&es);

    fight_state_t f = {duration:30 * 10000, mode:MODE_TEST};

    draw_state_t s = {w:es.screen_width,h:es.screen_height,fight:&f};

    init(&s);

    printf("Start!\n");
    
    long long begint = current_timestamp();

    long long endt = begint + f.duration;

    int counter = 0;

    while (!gQuit && f.duration > -5 * 10000) {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw(&s);

        updateScreen(&es);

        int rb = read_console();
        if (rb) {
            switch (rb) {
                case 'q': gQuit = 1; break;
                case 'p': s.pause = !s.pause;break;
            }
            printf("read:'%c' %02x\n",rb,rb);
        }
        f.duration = endt - current_timestamp();
        ++counter;
    }
    long long timeout = current_timestamp() - begint;
    printf("Stop! %.1lf frames per sec\n", counter * 10000.0 / timeout);
    
err_ret:
    deInitEGL(&es);
    return 0;
}
