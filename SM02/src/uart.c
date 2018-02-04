#include "common.h"

#ifdef CANBUS

typedef struct can_data_t {
    int fd;
    struct parser_state_t* parser;
} can_data_t;

void trace(const char* msg, int size, const uint8_t* data) {
    if (size != 0) {
        printf("%s", msg);
        for (int i = 0; i < size; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    } else {
        printf("%s : no data\n", msg);
    }
}

/***********************************************************************/

static int send_frame2(controller_t* c, int size, const uint8_t* frame) {
    // trace(" < ", size, frame);
    can_data_t* d = c->userdata;
    uint8_t buf[128];
    int bufsize = make_uart_frame(frame, size, buf);
    write(d->fd, buf, bufsize);
    return 0;
}

static void on_state_changed(void* usrdata, int type, int chan, uint64_t state);

static struct controller_t* init_can(const char* devname, lua_State* L) {

    int tty = open(devname, O_RDWR | O_NOCTTY/* O_NONBLOCK|  | O_NDELAY*/);
    if (tty <= 0) {
        err("can't open %s\n", devname);
        return NULL;
    }
    uint32_t serial = 0x02010005;

    struct controller_t* ret = init_controller(serial, on_state_changed, L);

    struct can_data_t* d = malloc(sizeof(can_data_t));
    d->fd = tty;

    ret->userdata = d;
    ret->send_frame = send_frame2;

    struct termios options;
    //set opt to 115200-8n1
    cfsetspeed(&options, B500000);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    options.c_lflag = 0;
    options.c_oflag &= ~OPOST;
    options.c_iflag = 0;

    tcsetattr(d->fd, TCSANOW, &options);

    fcntl(d->fd, F_SETFL, FNDELAY);

    d->parser = parser_init();

    return ret;
}

static void handle_can(struct controller_t* c) {
    if (c == NULL) return;

    can_data_t* d = c->userdata;

    uint8_t buf[512];

    fd_set fds;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(d->fd, &fds);

    int retval = select(d->fd + 1, &fds, NULL, NULL, &tv);

    if (retval < 0) {
        err("select() failed\n");
        return;
    }
    else if (retval == 0) {
         //log("timeout...\n");
    }
    else {
        int bytes;
        int r = ioctl(d->fd, FIONREAD, &bytes);
        if (r < 0) {
            log("FIONREAD error %d\n", r);
            return;
        }
        if (bytes > 0) {
            int n = read(d->fd, buf, bytes);
            //trace("read \n", n, buf);
            if (n < 0) {
               err("error while reading serial %d\n", errno);
            }
            else {
               //log("available %d bytes, read %d\n", bytes, n);
                for (int j = 0; j < n; j++) {
                    uint8_t ch = buf[j];
                    int e = parser_next(d->parser, ch);
                    if (e == errOK) {
                        //trace("read CAN frame ", s->len, s->frame);
                        c->on_frame(c, d->parser->len, d->parser->frame);
                    }
                    else if (e == errCRC) {
                        err("CRC error\n");
                    }
                    else if (e == errUnexpectedSeq) {
                        err("unexpected symbol:%02x\n", ch);
                    }
                    else if (e == errNone) {
                       //log("need more %02x\n", ch);
                    }
                }
            }
        }
    }
}

static void idle_can(struct controller_t* c) {
    if (c == NULL) return;

    // call idle fun
    c->on_frame(c, 0, NULL);
}

static void close_can(struct controller_t* c) {
    if (c == NULL) return;
    can_data_t* d = c->userdata;
    parser_close(d->parser);
    close(d->fd);
    free(d);
    free(c);
}

const char* helpString = "Usage: program lua_script_name\n";

static void on_state_changed(void* usrdata, int type, int chan, uint64_t state) {
    const char* name = NULL;
    double val = NAN;
    lua_State* L = usrdata;
    if (L != NULL) {
        if (type == cAnalogOut16) {
            switch (chan) {
                case DAC16_SCALE:
                    name = "scale";
                    break;
                case DAC16_HEADING:
                    name = "true_heading";
                    break;
                case DAC16_MAGHEADING:
                    name = "mag_heading";
                    break;
                case DAC16_SPEED:
                    name = "speed";
                    val = (double)state / 10;
                    break;
                case DAC16_ROT:
                    name = "rot";
                    val = ((double)state - 32768) / 10;
                    break;
            }
            if (name) {
                if (isnan(val))
                    val = (double)state / 100;
                lua_pushstring(L, name);
                lua_pushnumber(L, val);
                callLuaFunction(L, "setAnalog", 2);
            }
        }
        else if (type == cDiscreteOut) {
            switch (chan) {
                case LAMP_NODATA:
                    name = "no_data";
                    break;
                case LAMP_SOURCE:
                    name = "source";
                    break;
                case LAMP_CONFIG:
                    name = "config";
                    break;
                case LAMP_DESYNC:
                    name = "de_sync";
                    break;
                case LAMP_DAYNIGHT:
                    name = "day_night";
                    break;
                case LAMP_KNOTS_KPH:
                    name = "knots_kph";
                    break;
            }
            if (name) {
                lua_pushstring(L, name);
                lua_pushboolean(L, state != 0);
                callLuaFunction(L, "setDigital", 2);
            }
        }
    }
    //log("-- on_state_changed(%d,%d,%lld)\n",type, chan, state);
}
#endif
